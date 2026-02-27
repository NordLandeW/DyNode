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
 * @brief Lightweight wrapper for Media Foundation video decoding.
 *
 * Purpose:
 * - Manages video open/close, background decoding, and frame output.
 * - Exposes two frame-consumption models:
 *   1) latest-frame mode via `get_frame`
 *   2) external-clock sync mode via `set_sync_mode` and `get_frame_sync`
 *
 * Notes:
 * - Singleton instance; prefer using it with an "open -> consume -> close"
 *   lifecycle.
 * - `open` starts the worker thread in paused state; call `set_pause(false)`
 *   to begin producing frames.
 * - When sync mode is enabled, use `get_frame_sync`; do not mix with
 *   `get_frame`.
 * - Output memory is caller-owned; ensure buffer capacity and validity.
 * - Decoding runs on a worker thread. State accessors are cross-thread safe,
 *   but frequent mode/state toggles should be rate-limited by upper layers.
 *
 * Time synchronization:
 * - Asynchronous/latest-frame mode via `get_frame`:
 *   - No external clock is required.
 *   - The worker thread paces decode against sample timestamps scaled by
 *     playback speed, then updates a single latest-frame snapshot.
 *   - `get_frame` is non-blocking: it returns `1.0` only when a fresh snapshot
 *     is ready and clears the ready flag after copy; otherwise returns `0.0`.
 *   - If the consumer is slower than decode, intermediate frames may be
 *     replaced by newer snapshots.
 *
 * - External-clock sync mode via `set_sync_mode(true)` and `get_frame_sync`:
 *   - `get_frame_sync` advances an internal consumer clock by `delta_time`
 *     seconds each call. Non-positive values are treated as 0.
 *   - Decoded frames carry Media Foundation timestamps in 100ns ticks.
 *   - A frame is considered due when its timestamp is less than or equal to
 *     the consumer clock.
 *   - If the next frame is not due yet, `get_frame_sync` returns `0.0`
 *     immediately without copying.
 *   - If the frame is due but decode output is temporarily behind,
 *     `get_frame_sync` may block until a frame arrives, playback stops, sync
 *     mode is disabled, decoder is paused, or EOS is reached.
 *   - This design keeps presentation cadence controlled by the external clock,
 *     while decode throughput remains asynchronous.
 *
 * Usage examples:
 * - Latest-frame mode:
 * @code
 *   auto& dec = VideoDecoder::get_instance();
 *   if (dec.open(L"example.mp4")) {
 *       dec.set_pause(false);
 *
 *       const double bytes = dec.get_width() * dec.get_height() * 4.0;
 *       std::vector<unsigned char> frame(static_cast<size_t>(bytes));
 *
 *       // Poll in your update loop.
 *       if (dec.get_frame(frame.data(), static_cast<double>(frame.size())) ==
 *           1.0) {
 *           // Upload/use RGB32 pixels in frame.
 *       }
 *
 *       dec.close();
 *   }
 * @endcode
 *
 * - External-clock sync mode:
 * @code
 *   auto& dec = VideoDecoder::get_instance();
 *   if (dec.open(L"example.mp4")) {
 *       dec.set_sync_mode(true);
 *       dec.set_pause(false);
 *
 *       const double bytes = dec.get_width() * dec.get_height() * 4.0;
 *       std::vector<unsigned char> frame(static_cast<size_t>(bytes));
 *
 *       // Call once per tick; deltaSec comes from your fixed/update step.
 *       const double copied = dec.get_frame_sync(
 *           frame.data(), static_cast<double>(frame.size()), deltaSec);
 *       if (copied == 1.0) {
 *           // Consume due frame.
 *       }
 *
 *       dec.set_sync_mode(false);
 *       dec.close();
 *   }
 * @endcode
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
     * file and spawns a background decode thread. The decoder starts paused;
     * call set_pause(false) to begin pulling frames. If a video is already
     * loaded it will be closed first.
     *
     * @param filename UTF-16 path to the video file.
     * @return true when the reader is created successfully, false otherwise.
     */
    bool open(const wchar_t* filename);

    /**
     * @brief Stops decoding and releases active runtime decode resources.
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
     * This mode exists to support deterministic consumers like a recorder
     * that drive presentation with an external timestep. When enabled, the
     * decode thread runs as fast as possible and pushes frames into an internal
     * queue for get_frame_sync to consume.
     *
     * Important: get_frame is intentionally not allowed while sync mode is
     * enabled so the legacy latest frame semantics cannot interfere.
     */
    void set_sync_mode(bool enable);

    bool is_sync_mode() const {
        return m_isSyncMode.load(std::memory_order_acquire);
    }

    /**
     * @brief Copies the latest decoded frame into a caller-provided buffer.
     *
     * If a fresh frame is available and the buffer is large enough, the pixels
     * are copied and the method returns 1.0. The internal frame ready flag is
     * then cleared so subsequent calls will not return the same frame again.
     * The destination pointer must be valid.
     *
     * @param gm_buffer_ptr Destination buffer pointer owned by the caller.
     * @param buffer_size   Size of the destination buffer in bytes.
     * @return 1.0 when a frame was copied successfully, 0.0 otherwise.
     */
    double get_frame(void* gm_buffer_ptr, double buffer_size);

    /**
     * @brief Sync-mode frame fetch driven by an external timestep.
     *
     * This API is meaningful only when sync mode is enabled via
     * set_sync_mode(true).
     *
     * Semantics:
     * - The decoder maintains an internal clock advanced by delta_time on every
     *   call. Non-positive values are clamped to 0.
     * - If the next frame is not due yet, meaning the next frame timestamp is
     *   greater than the internal clock, the function returns 0.0 immediately.
     * - If the next frame is due but the decode thread has not produced it yet,
     *   meaning the queue is empty or behind, the function blocks until either
     *   a new decoded frame becomes available, playback is paused, stopped, or
     *   ended, or sync mode is disabled.
     * - When a frame whose timestamp is less than or equal to the internal
     *   clock is available, it is copied into gm_buffer_ptr and the function
     *   returns 1.0.
     *
     * @param gm_buffer_ptr Destination buffer pointer owned by the caller.
     * @param buffer_size   Size of the destination buffer in bytes.
     * @param delta_time    External timestep in seconds; used to advance the
     *                      internal clock.
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
     * @brief Returns the current decoded frame position in microseconds.
     *
     * The value is derived from the last decoded Media Foundation timestamp
     * (100ns ticks) and converted to microseconds.
     */
    double get_position();

    /**
     * @brief Returns whether a source is currently open.
     *
     * Uses an explicit atomic load to avoid relying on implicit conversions
     * which are deprecated or removed in newer standard library implementations
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
     * @param speed Playback speed multiplier. Values less than or equal to 0 or
     *              NaN fall back to 1.0. Values are clamped to a reasonable
     *              range.
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

    /**
     * @brief Applies a pending seek request on the decode thread.
     *
     * Consumes m_seekTarget, performs reader seek, and initializes temporary
     * frame-skip state so stale frames prior to the target time are ignored.
     */
    void handle_pending_seek(
        bool isSyncMode, long long& skipUntilTime,
        std::chrono::high_resolution_clock::time_point& skipStartTime);

    /**
     * @brief Determines whether a decoded sample should be dropped.
     *
     * Used after seek to discard frames earlier than skipUntilTime, with a
     * time-based escape hatch to avoid stalling if timestamps are irregular.
     */
    bool should_drop_sample(
        LONGLONG timestamp, bool isSyncMode, long long& skipUntilTime,
        const std::chrono::high_resolution_clock::time_point& skipStartTime);

    /**
     * @brief Copies RGB32 pixels from a Media Foundation sample into memory.
     *
     * @param pSample Source sample to read from.
     * @param dstBase Destination pixel buffer base address.
     * @param expectedSize Expected byte size for the copied frame.
     * @return true when conversion/copy succeeds and size matches.
     */
    bool copy_pixels_from_sample(IMFSample* pSample, BYTE* dstBase,
                                 size_t expectedSize);

    /**
     * @brief Writes the decoded sample as the latest frame snapshot.
     *
     * Thread-safe helper that updates m_pixelBuffer and frame-ready state for
     * get_frame().
     */
    bool write_latest_frame(IMFSample* pSample, size_t expectedSize);

    /**
     * @brief Builds a sync-queue frame from a decoded sample.
     *
     * Populates outFrame with copied pixels and the sample timestamp.
     */
    bool make_sync_frame(IMFSample* pSample, LONGLONG timestamp,
                         SyncFrame& outFrame, size_t expectedSize);

    /**
     * @brief Updates decoder timing hints from the latest timestamp.
     *
     * Refreshes last-decoded and estimated frame-duration atomics used by
     * sync-mode pacing/availability checks.
     */
    void update_decoded_timing(LONGLONG timestamp);

    /**
     * @brief Enqueues a frame for sync-mode consumers.
     *
     * Applies bounded-queue policy and notifies waiters that frame data is
     * available.
     */
    void enqueue_sync_frame(SyncFrame&& frame);

    /**
     * @brief Applies real-time pacing against presentation timestamp.
     *
     * Used in non-sync mode to avoid decoding too far ahead.
     */
    void pace_for_timestamp(LONGLONG timestamp);

    /**
     * @brief Checks whether the source reader is valid for decoding.
     *
     * @return true when decoding may proceed; false when caller should abort
     *         current loop iteration.
     */
    bool ensure_reader_available_for_decode();

    /**
     * @brief Blocks while paused and exits early on stop/close.
     *
     * @return true when decoding should continue, false when loop should end.
     */
    bool wait_if_paused();

    /**
     * @brief Reads one sample from the source reader.
     *
     * @param pSample [out] Returned sample pointer, may be null on EOS.
     * @param flags [out] Media Foundation reader flags.
     * @param timestamp [out] Sample timestamp in 100ns ticks.
     * @return true on successful reader call, false on read failure.
     */
    bool read_sample_from_reader(IMFSample*& pSample, DWORD& flags,
                                 LONGLONG& timestamp);

    /**
     * @brief Handles end-of-stream flags and related decoder state updates.
     */
    void handle_end_of_stream_flag(DWORD flags, IMFSample*& pSample,
                                   long long& skipUntilTime);

    /**
     * @brief Processes one decoded sample for the active output mode.
     *
     * Routes sample to latest-frame or sync-queue path and performs timestamp
     * bookkeeping.
     */
    bool process_sample_for_output(
        IMFSample* pSample, LONGLONG timestamp, bool isSyncMode,
        long long& skipUntilTime,
        const std::chrono::high_resolution_clock::time_point& skipStartTime);

    /**
     * @brief Computes the timestamp threshold for the next sync frame.
     *
     * Requires m_syncMutex held by caller.
     */
    long long compute_next_due_ticks_locked() const;

    /**
     * @brief Waits until a due sync frame may be consumed.
     *
     * Requires m_syncMutex held; may block on condition variables.
     */
    bool wait_for_due_sync_frame_locked(std::unique_lock<std::mutex>& lock);

    /**
     * @brief Checks whether the front queued frame can be copied now.
     *
     * Requires m_syncMutex held.
     */
    bool can_copy_front_sync_frame_locked(long long clockTicks,
                                          double buffer_size) const;

    /**
     * @brief Copies and pops the front sync frame.
     *
     * Requires m_syncMutex held; returns 1.0 on successful copy.
     */
    double copy_front_sync_frame_locked(void* gm_buffer_ptr,
                                        std::unique_lock<std::mutex>& lock);

    /**
     * @brief Core implementation for sync-mode frame acquisition.
     *
     * Requires m_syncMutex held by caller.
     */
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