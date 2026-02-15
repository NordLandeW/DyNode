#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

struct IMFSourceReader;
struct IMFSample;

/**
 * @brief Thin wrapper around Media Foundation video decoding.
 *
 * The decoder owns a background thread that continuously pulls frames from the
 * Media Foundation source reader into an internal pixel buffer. Public methods
 * control the lifetime, playback state, and data transfer into the host
 * engine's memory.
 */
class VideoDecoder {
   public:
    static VideoDecoder& get_instance() {
        static VideoDecoder instance;
        return instance;
    }

    VideoDecoder();
    ~VideoDecoder();

    /**
     * @brief Opens a video file and starts the decode loop.
     *
     * This call initializes the Media Foundation source reader for the given
     * file and spawns a background thread that begins decoding frames
     * immediately. If a video is already loaded it will be closed first.
     *
     * @param filename UTF-16 path to the video file.
     * @return true when the reader is created successfully, false otherwise.
     */
    bool open(const wchar_t* filename);

    /**
     * @brief Stops decoding and releases all Media Foundation resources.
     *
     * This will signal the decode thread to exit and block until it has
     * terminated. It is safe to call multiple times; subsequent calls become
     * no-ops once resources are already released.
     */
    void close(bool cleanup = false);

    /**
     * @brief Pauses or resumes playback without tearing down the reader.
     *
     * When paused, the decode thread remains alive but simply sleeps instead of
     * pulling new samples. This avoids the cost of repeatedly recreating
     * Media Foundation objects when toggling playback.
     *
     * @param pause true to pause, false to resume.
     */
    void set_pause(bool pause);

    /**
     * @brief Schedules a seek to the specified playback time.
     *
     * The actual seek is performed on the decode thread before the next read.
     * This keeps COM interaction confined to a single thread and avoids
     * cross-thread calls into Media Foundation.
     *
     * @param seconds Target timestamp in seconds from the beginning of the
     *                stream.
     */
    void seek(double seconds);

    /**
     * @brief Enables or disables the synchronous, clock-driven decode mode.
     *
     * This mode exists to support deterministic consumers (e.g. a recorder)
     * that drive presentation with an external timestep. When enabled, the
     * decode thread runs as fast as possible and pushes frames into an internal
     * queue for get_frame_sync() to consume.
     *
     * Important: get_frame() is intentionally not allowed while sync mode is
     * enabled so the legacy "latest frame" semantics cannot interfere.
     */
    void set_sync_mode(bool enable);

    bool is_sync_mode() const {
        return m_isSyncMode.load(std::memory_order_acquire);
    }

    /**
     * @brief Copies the latest decoded frame into a caller-provided buffer.
     *
     * If a fresh frame is available and the buffer is large enough, the pixels
     * are copied and the method returns 1.0. The internal "frame ready" flag is
     * then cleared so subsequent calls will not return the same frame again.
     *
     * @param gm_buffer_ptr Destination buffer pointer owned by the caller.
     * @param buffer_size   Size of the destination buffer in bytes.
     * @return 1.0 when a frame was copied successfully, 0.0 otherwise
     *         (for example when no frame is ready or the buffer is too small).
     */
    double get_frame(void* gm_buffer_ptr, double buffer_size);

    /**
     * @brief Sync-mode frame fetch driven by an external timestep.
     *
     * This API is meaningful only when sync mode is enabled via
     * set_sync_mode(true).
     *
     * Semantics:
     * - The decoder maintains an internal clock ("clock B") advanced by
     *   @p delta_time on every call.
     * - If the next frame is not due yet (next frame timestamp > clock B), the
     *   function returns 0.0 immediately.
     * - If the next frame is due but the decode thread has not produced it yet
     *   (queue is empty / behind), the function blocks until either:
     *     - a new decoded frame becomes available, or
     *     - playback stops/ends, or
     *     - sync mode is disabled.
     * - When a frame whose timestamp <= clock B is available, it is copied into
     *   @p gm_buffer_ptr and the function returns 1.0.
     *
     * @param gm_buffer_ptr Destination buffer pointer owned by the caller.
     * @param buffer_size   Size of the destination buffer in bytes.
     * @param delta_time    External timestep in seconds; used to advance clock
     * B.
     * @return 1.0 when a frame was copied successfully, 0.0 otherwise.
     */
    double get_frame_sync(void* gm_buffer_ptr, double buffer_size,
                          double delta_time);

    /**
     * @brief Returns the decoded video width in pixels.
     *
     * The value is derived from the Media Foundation frame size attribute after
     * the source reader has been configured.
     */
    double get_width();

    /**
     * @brief Returns the decoded video height in pixels.
     *
     * The value is derived from the Media Foundation frame size attribute after
     * the source reader has been configured.
     */
    double get_height();

    /**
     * @brief Returns the video duration in seconds.
     */
    double get_duration();

    /**
     * @brief Returns whether a source is currently open.
     *
     * Uses an explicit atomic load to avoid relying on implicit conversions
     * (which are deprecated/removed in newer standard library implementations)
     * and to make the cross-thread intent clear.
     */
    bool is_loaded() const {
        return m_isLoaded.load(std::memory_order_acquire);
    }

    /**
     * @brief Returns whether playback is currently running.
     */
    bool is_playing() const {
        return m_isPlaying.load(std::memory_order_acquire);
    }

    /**
     * @brief Returns whether the decoder has reached end-of-stream.
     */
    bool is_finished() const {
        return m_isFinished.load(std::memory_order_acquire);
    }

    /**
     * @brief Sets playback speed multiplier.
     *
     * Speed affects the pacing of presentation timestamps in the decode thread.
     * It does not alter the underlying decode format.
     *
     * @param speed Playback speed multiplier. Values <= 0 (or NaN) fall back to
     *              1.0. Values are clamped to a reasonable range.
     * @return The effective speed that was applied.
     */
    double set_speed(double speed) {
        if (!(speed > 0.0)) {
            speed = 1.0;
        }

        constexpr double kMinSpeed = 0.01;
        constexpr double kMaxSpeed = 16.0;

        if (speed < kMinSpeed) {
            speed = kMinSpeed;
        } else if (speed > kMaxSpeed) {
            speed = kMaxSpeed;
        }

        videoSpeed.store(speed, std::memory_order_relaxed);
        return speed;
    }

    /**
     * @brief Gets current playback speed multiplier.
     */
    double get_speed() const {
        return videoSpeed.load(std::memory_order_relaxed);
    }

   private:
    /**
     * @brief Background loop that pulls samples from Media Foundation.
     *
     * This method runs on a dedicated thread created by open(). It handles
     * seeking, pause state, and conversion of samples into a contiguous RGB32
     * pixel buffer protected by a mutex.
     */
    void decode_loop();

    struct SyncFrame {
        std::vector<BYTE> pixels;
        long long timestampTicks = 0;
    };

    static constexpr size_t kMaxSyncQueueFrames = 3;

    void handle_pending_seek(
        bool isSyncMode, long long& skipUntilTime,
        std::chrono::high_resolution_clock::time_point& skipStartTime);
    bool should_drop_sample(
        LONGLONG timestamp, bool isSyncMode, long long& skipUntilTime,
        const std::chrono::high_resolution_clock::time_point& skipStartTime);
    bool copy_pixels_from_sample(IMFSample* pSample, BYTE* dstBase,
                                 size_t expectedSize);
    bool write_latest_frame(IMFSample* pSample, size_t expectedSize);
    bool make_sync_frame(IMFSample* pSample, LONGLONG timestamp,
                         SyncFrame& outFrame, size_t expectedSize);
    void update_decoded_timing(LONGLONG timestamp);
    void enqueue_sync_frame(SyncFrame&& frame);
    void pace_for_timestamp(LONGLONG timestamp);
    bool ensure_reader_available_for_decode();
    bool wait_if_paused();
    bool read_sample_from_reader(IMFSample*& pSample, DWORD& flags,
                                 LONGLONG& timestamp);
    void handle_end_of_stream_flag(DWORD flags, IMFSample*& pSample,
                                   long long& skipUntilTime);
    bool process_sample_for_output(
        IMFSample* pSample, LONGLONG timestamp, bool isSyncMode,
        long long& skipUntilTime,
        const std::chrono::high_resolution_clock::time_point& skipStartTime);
    long long compute_next_due_ticks_locked() const;
    bool wait_for_due_sync_frame_locked(std::unique_lock<std::mutex>& lock);
    bool can_copy_front_sync_frame_locked(long long clockTicks,
                                          double buffer_size) const;
    double copy_front_sync_frame_locked(void* gm_buffer_ptr,
                                        std::unique_lock<std::mutex>& lock);
    double get_frame_sync_locked(void* gm_buffer_ptr, double buffer_size,
                                 long long clockTicks,
                                 std::unique_lock<std::mutex>& lock);

    IMFSourceReader* m_pReader =
        nullptr;  // Media Foundation source reader providing video samples.
    std::thread
        m_decodeThread;       // Dedicated worker thread running decode_loop().
    std::mutex m_frameMutex;  // Guards concurrent access to the pixel buffer.

    std::mutex m_syncMutex;
    std::condition_variable m_syncCvFrameAvailable;
    std::condition_variable m_syncCvQueueSpace;
    std::deque<SyncFrame> m_syncQueue;
    double m_syncClockSeconds = 0.0;
    long long m_syncLastPresentedTimestampTicks = -1;

    // Timestamp of the last decoded sample (100ns ticks). Used by sync mode to
    // decide whether decoding is behind the consumer clock.
    std::atomic<long long> m_lastDecodedTimestampTicks{0};

    // Nominal per-frame duration derived from the stream metadata (100ns
    // ticks). This is a hint for sync mode when deciding whether the next frame
    // is due.
    std::atomic<long long> m_nominalFrameDurationTicks{0};

    // Observed per-frame duration derived from timestamps (100ns ticks). Used
    // as a fallback when metadata does not provide a frame rate.
    std::atomic<long long> m_syncEstimatedFrameDurationTicks{0};

    std::atomic<bool> m_isSyncMode = false;

    std::vector<BYTE> m_pixelBuffer;  // Latest decoded frame in RGB32, consumed
                                      // by the host engine.
    UINT m_width = 0;   // Cached frame width queried from Media Foundation.
    UINT m_height = 0;  // Cached frame height queried from Media Foundation.
    LONG m_stride = 0;  // Stride (step) for the video frame.
    double m_duration = 0.0;  // Cached video duration in seconds.

    // Playback speed multiplier. Affects decode pacing (sleep) only.
    std::atomic<double> videoSpeed{1.0};

    // Playback state flag observed by the decode thread.
    std::atomic<bool> m_isPlaying = false;
    // Signals the decode thread to exit its main loop.
    std::atomic<bool> m_stopRequested = false;

    // Last presentation timestamp (in 100ns units) processed.
    long long m_lastPresentationTime = 0;
    // Target time for the next frame to be displayed (for drift correction).
    std::chrono::high_resolution_clock::time_point m_nextFrameTargetTime;
    // Indicates that m_pixelBuffer holds a fresh frame to consume.
    std::atomic<bool> m_isFrameReady = false;
    // Pending seek target in 100-nanosecond units, -1 when idle.
    std::atomic<long long> m_seekTarget = -1;
    // Reserved flag for end-of-stream handling.
    std::atomic<bool> m_isFinished = false;
    // Tracks whether a video source is currently open.
    std::atomic<bool> m_isLoaded = false;
};