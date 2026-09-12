#include <doctest/doctest.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <array>
#include <atomic>
#include <future>

#include "decoder.h"

// This seam only supplies a device-free reader and synchronizes entry into the
// real wait. No decoding, terminal transition or wait predicate is duplicated.
struct VideoDecoderTestAccess {
    static void prepare(VideoDecoder& decoder,
                        IMFSourceReader* reader = nullptr) {
        decoder.m_pReader = reader;
        decoder.m_isLoaded = true;
        decoder.m_isPlaying = true;
        decoder.m_isSyncMode = true;
    }
    static void start(VideoDecoder& decoder) {
        decoder.m_decodeThread =
            std::thread([&decoder] { decoder.decode_loop(); });
    }
    static void join(VideoDecoder& decoder) {
        decoder.m_decodeThread.join();
    }
    static std::future<double> consumer(VideoDecoder& decoder,
                                        std::array<BYTE, 4>& pixels,
                                        std::promise<void>& entered) {
        return std::async(std::launch::async, [&decoder, &pixels, &entered] {
            std::unique_lock<std::mutex> lock(decoder.m_syncMutex);
            entered.set_value();
            return decoder.get_frame_sync_locked(pixels.data(), pixels.size(),
                                                 0, lock);
        });
    }
    static void frame(VideoDecoder& decoder) {
        decoder.enqueue_sync_frame({{1, 2, 3, 4}, 0});
    }
    static void eof(VideoDecoder& decoder) {
        IMFSample* sample = nullptr;
        long long skipUntil = -1;
        decoder.handle_end_of_stream_flag(MF_SOURCE_READERF_ENDOFSTREAM, sample,
                                          skipUntil);
    }
    static void run_with_conflicting_com(VideoDecoder& decoder,
                                         std::promise<HRESULT>& initialized) {
        decoder.m_decodeThread = std::thread([&decoder, &initialized] {
            const HRESULT hr =
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            initialized.set_value(hr);
            if (SUCCEEDED(hr)) {
                decoder.decode_loop();
                CoUninitialize();
            }
        });
    }
};

namespace {
class FailingReader final : public IMFSourceReader {
    std::atomic<ULONG> refs{1};
    bool errorFlag;

   public:
    explicit FailingReader(bool flag = false) : errorFlag(flag) {
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
                                             void** object) override {
        if (!object)
            return E_POINTER;
        *object = nullptr;
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IMFSourceReader)) {
            *object = static_cast<IMFSourceReader*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++refs;
    }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG count = --refs;
        if (count == 0)
            delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE ReadSample(DWORD, DWORD, DWORD* stream,
                                         DWORD* flags, LONGLONG* timestamp,
                                         IMFSample** sample) override {
        if (stream)
            *stream = 0;
        if (flags)
            *flags = errorFlag ? MF_SOURCE_READERF_ERROR : 0;
        if (timestamp)
            *timestamp = 0;
        if (sample)
            *sample = nullptr;
        return errorFlag ? S_OK : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE GetStreamSelection(DWORD, BOOL*) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetStreamSelection(DWORD, BOOL) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetNativeMediaType(DWORD, DWORD,
                                                 IMFMediaType**) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetCurrentMediaType(DWORD,
                                                  IMFMediaType**) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetCurrentMediaType(DWORD, DWORD*,
                                                  IMFMediaType*) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetCurrentPosition(REFGUID,
                                                 REFPROPVARIANT) override {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Flush(DWORD) override {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetServiceForStream(DWORD, REFGUID, REFIID,
                                                  LPVOID*) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetPresentationAttribute(DWORD, REFGUID,
                                                       PROPVARIANT*) override {
        return E_NOTIMPL;
    }
};

struct DecoderFixture {
    VideoDecoder decoder;
    std::array<BYTE, 4> pixels{};
    ~DecoderFixture() {
        decoder.close();
    }
};

void check_failed_decoder(VideoDecoder& decoder, std::array<BYTE, 4>& pixels) {
    CHECK(decoder.has_failed());
    CHECK_FALSE(decoder.is_playing());
    CHECK_FALSE(decoder.is_finished());
    // Later consumers and the GML play/seek retry pattern must not resurrect
    // a worker that has exited, or enter another unbounded wait.
    decoder.set_pause(false);
    decoder.seek(0.0);
    CHECK_FALSE(decoder.is_playing());
    CHECK(decoder.get_frame_sync(pixels.data(), pixels.size(), 1.0) == 0.0);
}
}  // namespace

TEST_CASE("VideoReadFailureReleasesExistingAndFutureConsumers") {
    for (bool flag : {false, true}) {
        DecoderFixture fixture;
        VideoDecoderTestAccess::prepare(fixture.decoder,
                                        new FailingReader(flag));
        std::promise<void> entered;
        auto ready = entered.get_future();
        auto consumer = VideoDecoderTestAccess::consumer(
            fixture.decoder, fixture.pixels, entered);
        ready.get();
        VideoDecoderTestAccess::start(fixture.decoder);
        CHECK(consumer.get() == 0.0);
        VideoDecoderTestAccess::join(fixture.decoder);
        check_failed_decoder(fixture.decoder, fixture.pixels);
    }
}

TEST_CASE("VideoMissingReaderReleasesSyncConsumer") {
    DecoderFixture fixture;
    VideoDecoderTestAccess::prepare(fixture.decoder);
    std::promise<void> entered;
    auto ready = entered.get_future();
    auto consumer = VideoDecoderTestAccess::consumer(fixture.decoder,
                                                     fixture.pixels, entered);
    ready.get();
    VideoDecoderTestAccess::start(fixture.decoder);
    CHECK(consumer.get() == 0.0);
    VideoDecoderTestAccess::join(fixture.decoder);
    check_failed_decoder(fixture.decoder, fixture.pixels);
}

TEST_CASE("VideoComInitializationFailureReleasesSyncConsumer") {
    DecoderFixture fixture;
    VideoDecoderTestAccess::prepare(fixture.decoder);
    std::promise<void> entered;
    auto ready = entered.get_future();
    auto consumer = VideoDecoderTestAccess::consumer(fixture.decoder,
                                                     fixture.pixels, entered);
    ready.get();
    std::promise<HRESULT> initialized;
    auto result = initialized.get_future();
    VideoDecoderTestAccess::run_with_conflicting_com(fixture.decoder,
                                                     initialized);
    REQUIRE(SUCCEEDED(result.get()));
    CHECK(consumer.get() == 0.0);
    VideoDecoderTestAccess::join(fixture.decoder);
    check_failed_decoder(fixture.decoder, fixture.pixels);
}

TEST_CASE("VideoSyncWaitStillDeliversFramesAndStopsOnPause") {
    DecoderFixture fixture;
    VideoDecoderTestAccess::prepare(fixture.decoder);
    std::promise<void> entered;
    auto ready = entered.get_future();
    auto consumer = VideoDecoderTestAccess::consumer(fixture.decoder,
                                                     fixture.pixels, entered);
    ready.get();
    VideoDecoderTestAccess::frame(fixture.decoder);
    CHECK(consumer.get() == 1.0);
    CHECK(fixture.pixels == std::array<BYTE, 4>{1, 2, 3, 4});
    CHECK_FALSE(fixture.decoder.has_failed());
    // No guessed frame duration is needed for this new wait.
    fixture.decoder.set_sync_mode(false);
    fixture.decoder.set_sync_mode(true);
    std::promise<void> enteredAgain;
    auto readyAgain = enteredAgain.get_future();
    auto waitingAgain = VideoDecoderTestAccess::consumer(
        fixture.decoder, fixture.pixels, enteredAgain);
    readyAgain.get();
    fixture.decoder.set_pause(true);
    CHECK(waitingAgain.get() == 0.0);
    CHECK_FALSE(fixture.decoder.has_failed());
}

TEST_CASE("VideoEofIsNotWorkerFailureAndCanResume") {
    DecoderFixture fixture;
    VideoDecoderTestAccess::prepare(fixture.decoder);
    std::promise<void> entered;
    auto ready = entered.get_future();
    auto consumer = VideoDecoderTestAccess::consumer(fixture.decoder,
                                                     fixture.pixels, entered);
    ready.get();
    VideoDecoderTestAccess::eof(fixture.decoder);
    CHECK(consumer.get() == 0.0);
    CHECK(fixture.decoder.is_finished());
    CHECK_FALSE(fixture.decoder.has_failed());
    fixture.decoder.set_pause(false);
    CHECK(fixture.decoder.is_playing());
    CHECK_FALSE(fixture.decoder.is_finished());
    CHECK_FALSE(fixture.decoder.has_failed());
}

TEST_CASE("VideoCloseReleasesSyncConsumerWithoutFailure") {
    DecoderFixture fixture;
    VideoDecoderTestAccess::prepare(fixture.decoder);
    std::promise<void> entered;
    auto ready = entered.get_future();
    auto consumer = VideoDecoderTestAccess::consumer(fixture.decoder,
                                                     fixture.pixels, entered);
    ready.get();
    fixture.decoder.close();
    CHECK(consumer.get() == 0.0);
    CHECK_FALSE(fixture.decoder.is_loaded());
    CHECK_FALSE(fixture.decoder.has_failed());
}

TEST_CASE("VideoRuntimeShutdownReleasesConsumerAndPreservesSingletonLifetime") {
    auto& decoder = VideoDecoder::get_instance();
    VideoDecoderTestAccess::prepare(decoder);
    std::array<BYTE, 4> pixels{};
    std::promise<void> entered;
    auto ready = entered.get_future();
    auto consumer = VideoDecoderTestAccess::consumer(decoder, pixels, entered);
    ready.get();
    VideoDecoder::shutdown_instance();
    CHECK(consumer.get() == 0.0);
    CHECK_FALSE(decoder.is_loaded());
    CHECK_FALSE(decoder.has_failed());
    CHECK(&VideoDecoder::get_instance() == &decoder);
    CHECK_NOTHROW(VideoDecoder::shutdown_instance());
}
