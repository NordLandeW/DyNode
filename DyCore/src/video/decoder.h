#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

struct IMFSourceReader;

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

    bool is_loaded() const {
        return m_isLoaded;
    }

    bool is_playing() const {
        return m_isPlaying;
    }

    bool is_finished() const {
        return m_isFinished;
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

    IMFSourceReader* m_pReader =
        nullptr;  // Media Foundation source reader providing video samples.
    std::thread
        m_decodeThread;       // Dedicated worker thread running decode_loop().
    std::mutex m_frameMutex;  // Guards concurrent access to the pixel buffer.

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