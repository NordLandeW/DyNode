#include "decoder.h"

#include <d3d11.h>
#include <d3d11_4.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <minwindef.h>
#include <windows.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <cassert>
#include <chrono>
#include <cmath>
#include <format>
#include <string>

#include "utils.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

void VideoDecoder::handle_pending_seek(
    bool isSyncMode, long long& skipUntilTime,
    std::chrono::high_resolution_clock::time_point& skipStartTime) {
    const long long seekPos = m_seekTarget.exchange(-1);
    if (seekPos < 0) {
        return;
    }

    auto set_position = [this](long long pos) {
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = pos;
        HRESULT seekHr = m_pReader->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);
        return seekHr;
    };

    if (isSyncMode) {
        const HRESULT seekHr = set_position(seekPos);
        if (FAILED(seekHr)) {
            print_debug_message(std::format(
                "VideoDecoder::decode_loop SetCurrentPosition failed, "
                "hr=0x{:08X}",
                static_cast<unsigned long>(seekHr)));
            skipUntilTime = -1;
        } else {
            skipUntilTime = seekPos;
        }

        m_lastPresentationTime = 0;
        return;
    }

    bool useSmartSeek = false;
    const long long SMART_SEEK_THRESHOLD = 10 * 10000000LL;

    if (m_lastPresentationTime > 0 && seekPos > m_lastPresentationTime) {
        const long long diff = seekPos - m_lastPresentationTime;
        useSmartSeek = diff < SMART_SEEK_THRESHOLD;
    }

    if (useSmartSeek) {
        skipUntilTime = seekPos;
        skipStartTime = std::chrono::high_resolution_clock::now();
        print_debug_message(
            std::format("VideoDecoder::decode_loop performing smart "
                        "seek to {} ticks (diff: {}).",
                        seekPos, seekPos - m_lastPresentationTime));
    } else {
        const HRESULT seekHr = set_position(seekPos);
        if (FAILED(seekHr)) {
            print_debug_message(std::format(
                "VideoDecoder::decode_loop SetCurrentPosition failed, "
                "hr=0x{:08X}",
                static_cast<unsigned long>(seekHr)));
            skipUntilTime = -1;
        } else {
            skipUntilTime = seekPos;
            skipStartTime = std::chrono::high_resolution_clock::now();
        }
    }

    m_lastPresentationTime = 0;
}

bool VideoDecoder::should_drop_sample(
    LONGLONG timestamp, bool isSyncMode, long long& skipUntilTime,
    const std::chrono::high_resolution_clock::time_point& skipStartTime) {
    if (skipUntilTime < 0) {
        return false;
    }

    if (isSyncMode) {
        if (timestamp < skipUntilTime) {
            return true;
        }

        skipUntilTime = -1;
        return false;
    }

    const auto now = std::chrono::high_resolution_clock::now();
    const long long elapsedTicks =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now -
                                                             skipStartTime)
            .count() /
        100;

    const double speed = videoSpeed.load(std::memory_order_relaxed);
    const double effectiveSpeed = (speed > 0.0) ? speed : 1.0;

    const long long timeFlowTicks =
        static_cast<long long>(static_cast<long double>(elapsedTicks) *
                               static_cast<long double>(effectiveSpeed));

    if (timestamp < skipUntilTime + timeFlowTicks) {
        return true;
    }

    skipUntilTime = -1;
    return false;
}

bool VideoDecoder::copy_pixels_from_sample(IMFSample* pSample, BYTE* dstBase,
                                           size_t expectedSize) {
    IMFMediaBuffer* pBuffer = nullptr;
    HRESULT hr = pSample->GetBufferByIndex(0, &pBuffer);
    if (FAILED(hr) || !pBuffer) {
        if (pBuffer) {
            pBuffer->Release();
        }
        print_debug_message(std::format(
            "VideoDecoder::decode_loop GetBufferByIndex failed, hr=0x{:08X}",
            static_cast<unsigned long>(hr)));
        return false;
    }

    BYTE* pData = nullptr;
    DWORD currLen = 0;
    LONG lStride = 0;
    IMF2DBuffer* p2DBuffer = nullptr;
    bool locked2D = false;
    BYTE* pScanline0 = nullptr;

    if (SUCCEEDED(pBuffer->QueryInterface(IID_PPV_ARGS(&p2DBuffer)))) {
        if (SUCCEEDED(p2DBuffer->Lock2D(&pScanline0, &lStride))) {
            locked2D = true;
        }
    }

    if (!locked2D) {
        if (p2DBuffer) {
            p2DBuffer->Release();
            p2DBuffer = nullptr;
        }
        hr = pBuffer->Lock(&pData, NULL, &currLen);
        if (FAILED(hr)) {
            pBuffer->Release();
            print_debug_message(std::format(
                "VideoDecoder::decode_loop buffer lock failed, hr=0x{:08X}",
                static_cast<unsigned long>(hr)));
            return false;
        }
    }

    const size_t rowBytes = m_width * 4;

    if (locked2D) {
        BYTE* src = pScanline0;
        for (UINT i = 0; i < m_height; ++i) {
            memcpy(dstBase + i * rowBytes,
                   src + (static_cast<LONG>(i) * lStride), rowBytes);
        }

        p2DBuffer->Unlock2D();
        p2DBuffer->Release();
        pBuffer->Release();
        return true;
    }

    const LONG lSrcStride = m_stride;

    BYTE* src = pData;
    BYTE* dst = dstBase;

    for (UINT i = 0; i < m_height; ++i) {
        if (src + rowBytes > pData + currLen) {
            break;
        }

        memcpy(dst, src, rowBytes);

        dst += rowBytes;
        src += lSrcStride;
    }

    pBuffer->Unlock();
    pBuffer->Release();
    return true;
}

bool VideoDecoder::write_latest_frame(IMFSample* pSample, size_t expectedSize) {
    std::lock_guard<std::mutex> lock(m_frameMutex);

    if (m_pixelBuffer.size() != expectedSize) {
        m_pixelBuffer.resize(expectedSize);
        print_debug_message(std::format(
            "VideoDecoder::decode_loop resized pixel buffer to {} bytes "
            "({}x{}x4).",
            static_cast<unsigned long>(expectedSize), m_width, m_height));
    }

    if (!copy_pixels_from_sample(pSample, m_pixelBuffer.data(), expectedSize)) {
        return false;
    }

    m_isFrameReady = true;
    return true;
}

bool VideoDecoder::make_sync_frame(IMFSample* pSample, LONGLONG timestamp,
                                   SyncFrame& outFrame, size_t expectedSize) {
    outFrame.timestampTicks = timestamp;
    outFrame.pixels.resize(expectedSize);
    return copy_pixels_from_sample(pSample, outFrame.pixels.data(),
                                   expectedSize);
}

void VideoDecoder::update_decoded_timing(LONGLONG timestamp) {
    m_lastDecodedTimestampTicks.store(timestamp, std::memory_order_relaxed);

    if (m_lastPresentationTime > 0 && timestamp > m_lastPresentationTime) {
        const long long diffTicks = timestamp - m_lastPresentationTime;
        if (diffTicks > 0) {
            m_syncEstimatedFrameDurationTicks.store(diffTicks,
                                                    std::memory_order_relaxed);
        }
    }
}

void VideoDecoder::enqueue_sync_frame(SyncFrame&& frame) {
    std::unique_lock<std::mutex> qlock(m_syncMutex);
    m_syncCvQueueSpace.wait(qlock, [this]() {
        return m_stopRequested || !m_isPlaying ||
               !m_isSyncMode.load(std::memory_order_relaxed) ||
               m_syncQueue.size() < kMaxSyncQueueFrames;
    });

    if (!m_stopRequested && m_isPlaying &&
        m_isSyncMode.load(std::memory_order_relaxed)) {
        m_syncQueue.push_back(std::move(frame));
        qlock.unlock();
        m_syncCvFrameAvailable.notify_one();
    }
}

void VideoDecoder::pace_for_timestamp(LONGLONG timestamp) {
    if (m_lastPresentationTime > 0 && timestamp > m_lastPresentationTime) {
        const long long diffTicks = timestamp - m_lastPresentationTime;

        const double speed = videoSpeed.load(std::memory_order_relaxed);
        const double effectiveSpeed = (speed > 0.0) ? speed : 1.0;

        const long long scaledNs = static_cast<long long>(
            (static_cast<long double>(diffTicks) * 100.0L) /
            static_cast<long double>(effectiveSpeed));

        m_nextFrameTargetTime += std::chrono::nanoseconds(scaledNs);

        std::this_thread::sleep_until(m_nextFrameTargetTime);
        return;
    }

    m_nextFrameTargetTime = std::chrono::high_resolution_clock::now();
}

bool VideoDecoder::ensure_reader_available_for_decode() {
    if (m_pReader) {
        return true;
    }

    print_debug_message(
        "VideoDecoder::decode_loop exiting because m_pReader is null.");
    return false;
}

bool VideoDecoder::wait_if_paused() {
    if (m_isPlaying) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return true;
}

bool VideoDecoder::read_sample_from_reader(IMFSample*& pSample, DWORD& flags,
                                           LONGLONG& timestamp) {
    DWORD streamIndex = 0;
    flags = 0;
    timestamp = 0;

    const HRESULT hr =
        m_pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                              &streamIndex, &flags, &timestamp, &pSample);

    if (SUCCEEDED(hr)) {
        return true;
    }

    if (pSample) {
        pSample->Release();
        pSample = nullptr;
    }

    print_debug_message(
        std::format("VideoDecoder::decode_loop ReadSample failed, hr=0x{:08X}",
                    static_cast<unsigned long>(hr)));
    return false;
}

void VideoDecoder::handle_end_of_stream_flag(DWORD flags, IMFSample*& pSample,
                                             long long& skipUntilTime) {
    if (!(flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
        return;
    }

    if (pSample) {
        pSample->Release();
        pSample = nullptr;
    }

    m_isFinished = true;
    m_isPlaying = false;
    skipUntilTime = -1;

    m_syncCvFrameAvailable.notify_all();
    m_syncCvQueueSpace.notify_all();

    print_debug_message("VideoDecoder::decode_loop reached end of stream.");
}

bool VideoDecoder::process_sample_for_output(
    IMFSample* pSample, LONGLONG timestamp, bool isSyncMode,
    long long& skipUntilTime,
    const std::chrono::high_resolution_clock::time_point& skipStartTime) {
    if (should_drop_sample(timestamp, isSyncMode, skipUntilTime,
                           skipStartTime)) {
        pSample->Release();
        return false;
    }

    const size_t expectedSize = m_width * m_height * 4;
    SyncFrame syncFrame;
    const bool copied = isSyncMode ? make_sync_frame(pSample, timestamp,
                                                     syncFrame, expectedSize)
                                   : write_latest_frame(pSample, expectedSize);

    pSample->Release();

    if (!copied) {
        return false;
    }

    update_decoded_timing(timestamp);

    if (isSyncMode) {
        if (m_seekTarget.load(std::memory_order_relaxed) >= 0) {
            m_lastPresentationTime = timestamp;
            return true;
        }

        enqueue_sync_frame(std::move(syncFrame));
        m_lastPresentationTime = timestamp;
        return true;
    }

    pace_for_timestamp(timestamp);
    m_lastPresentationTime = timestamp;
    return true;
}

void VideoDecoder::decode_loop() {
    // Initialize COM on the worker thread since Media Foundation objects are
    // apartment-affine.
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        // Decoding cannot proceed on this thread without COM; log and bail out.
        print_debug_message(std::format(
            "VideoDecoder::decode_loop CoInitializeEx failed, hr=0x{:08X}",
            static_cast<unsigned long>(hr)));
        return;
    }

    print_debug_message("VideoDecoder::decode_loop started.");

    long long skipUntilTime = -1;
    auto skipStartTime = std::chrono::high_resolution_clock::now();

    while (!m_stopRequested) {
        if (!ensure_reader_available_for_decode()) {
            break;
        }

        if (wait_if_paused()) {
            continue;
        }

        const bool isSyncMode = m_isSyncMode.load(std::memory_order_relaxed);
        handle_pending_seek(isSyncMode, skipUntilTime, skipStartTime);

        IMFSample* pSample = nullptr;
        DWORD flags = 0;
        LONGLONG llTimeStamp = 0;

        if (!read_sample_from_reader(pSample, flags, llTimeStamp)) {
            break;
        }

        handle_end_of_stream_flag(flags, pSample, skipUntilTime);

        if (!pSample) {
            continue;
        }

        process_sample_for_output(pSample, llTimeStamp, isSyncMode,
                                  skipUntilTime, skipStartTime);
    }

    print_debug_message("VideoDecoder::decode_loop exiting.");

    CoUninitialize();
}

VideoDecoder::VideoDecoder() {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder constructor MFStartup failed, hr=0x{:08X}",
            static_cast<unsigned long>(hr)));
    } else {
        print_debug_message("VideoDecoder constructor MFStartup succeeded.");
    }
}

VideoDecoder::~VideoDecoder() {
    close(true);
    HRESULT hr = MFShutdown();
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder destructor MFShutdown failed, hr=0x{:08X}",
            static_cast<unsigned long>(hr)));
    } else {
        print_debug_message("VideoDecoder destructor MFShutdown succeeded.");
    }
}

void enable_hardware_acceleration(IMFAttributes* pAttributes) {
    ID3D11Device* pD3DDevice = nullptr;
    ID3D11DeviceContext* pD3DContext = nullptr;
    UINT creationFlags =
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1,
                                         D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

    HRESULT d3dHr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
        featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pD3DDevice,
        &featureLevel, &pD3DContext);

    if (SUCCEEDED(d3dHr)) {
        ID3D11Multithread* pMultithread = nullptr;
        if (SUCCEEDED(
                pD3DDevice->QueryInterface(IID_PPV_ARGS(&pMultithread)))) {
            pMultithread->SetMultithreadProtected(TRUE);
            pMultithread->Release();
        }

        UINT resetToken = 0;
        IMFDXGIDeviceManager* pDXGIManager = nullptr;
        d3dHr = MFCreateDXGIDeviceManager(&resetToken, &pDXGIManager);
        if (SUCCEEDED(d3dHr)) {
            d3dHr = pDXGIManager->ResetDevice(pD3DDevice, resetToken);
            if (SUCCEEDED(d3dHr)) {
                pAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER,
                                        pDXGIManager);
                pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS,
                                       TRUE);
                print_debug_message(
                    "VideoDecoder::open Hardware Acceleration enabled "
                    "(D3D11).");
            }
            pDXGIManager->Release();
        }

        if (pD3DContext) {
            pD3DContext->Release();
        }
        if (pD3DDevice) {
            pD3DDevice->Release();
        }
        return;
    }

    print_debug_message(
        std::format("VideoDecoder::open D3D11CreateDevice failed, hr=0x{:08X}",
                    static_cast<unsigned long>(d3dHr)));
}

bool configure_reader_rgb32(IMFSourceReader* reader,
                            const std::string& file_utf8) {
    IMFMediaType* pType = nullptr;
    HRESULT hr = MFCreateMediaType(&pType);
    if (FAILED(hr)) {
        print_debug_message(
            std::format("VideoDecoder::open MFCreateMediaType failed for '{}', "
                        "hr=0x{:08X}",
                        file_utf8, static_cast<unsigned long>(hr)));
        return false;
    }

    hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open SetGUID(MF_MT_MAJOR_TYPE) failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        pType->Release();
        return false;
    }

    hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open SetGUID(MF_MT_SUBTYPE=RGB32) failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        pType->Release();
        return false;
    }

    hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                     NULL, pType);
    pType->Release();

    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open SetCurrentMediaType RGB32 failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        return false;
    }

    return true;
}

bool query_video_format(IMFSourceReader* reader, UINT& width, UINT& height,
                        LONG& stride,
                        std::atomic<long long>& nominalFrameDurationTicks,
                        const std::string& file_utf8) {
    IMFMediaType* pCurrentType = nullptr;
    HRESULT hr = reader->GetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open GetCurrentMediaType failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        return false;
    }

    hr = MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &width, &height);

    if (SUCCEEDED(hr)) {
        HRESULT strideHr =
            pCurrentType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&stride);

        if (FAILED(strideHr)) {
            strideHr = MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1,
                                                      width, &stride);
            if (FAILED(strideHr)) {
                stride = width * 4;
            }
        }

        if (stride < 0) {
            stride = -stride;
        }

        print_debug_message(
            std::format("VideoDecoder::open Stride set to {}.", stride));

        UINT32 frameRateNumerator = 0;
        UINT32 frameRateDenominator = 0;
        if (SUCCEEDED(MFGetAttributeRatio(pCurrentType, MF_MT_FRAME_RATE,
                                          &frameRateNumerator,
                                          &frameRateDenominator)) &&
            frameRateNumerator > 0 && frameRateDenominator > 0) {
            const long long frameDurationTicks =
                (10000000LL * static_cast<long long>(frameRateDenominator)) /
                static_cast<long long>(frameRateNumerator);
            if (frameDurationTicks > 0) {
                nominalFrameDurationTicks.store(frameDurationTicks,
                                                std::memory_order_relaxed);
            }
        }
    }

    pCurrentType->Release();

    if (FAILED(hr)) {
        print_debug_message(
            std::format("VideoDecoder::open MFGetAttributeSize failed for "
                        "'{}', hr=0x{:08X}",
                        file_utf8, static_cast<unsigned long>(hr)));
        return false;
    }

    return true;
}

double query_duration_seconds(IMFSourceReader* reader) {
    PROPVARIANT var;
    PropVariantInit(&var);

    HRESULT hr = reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,
                                                  MF_PD_DURATION, &var);
    double duration = 0.0;
    if (SUCCEEDED(hr)) {
        long long durationVal = 0;
        if (var.vt == VT_UI8) {
            durationVal = var.uhVal.QuadPart;
        } else if (var.vt == VT_I8) {
            durationVal = var.hVal.QuadPart;
        }
        duration = static_cast<double>(durationVal) / 10000000.0;
    } else {
        print_debug_message(
            "VideoDecoder::open warning: could not determine duration.");
    }

    PropVariantClear(&var);
    return duration;
}

bool VideoDecoder::open(const wchar_t* filename) {
    if (m_isLoaded) {
        print_debug_message(
            "VideoDecoder::open called while a video is already loaded; "
            "closing existing source.");
        close();
    }

    std::string file_utf8 = wstringToUtf8(std::wstring(filename));
    print_debug_message(
        std::format("VideoDecoder::open requested for '{}'.", file_utf8));

    m_isFinished = false;
    m_isFrameReady = false;
    m_stopRequested = false;
    m_seekTarget = -1;
    m_lastPresentationTime = 0;

    m_lastDecodedTimestampTicks.store(0, std::memory_order_relaxed);
    m_nominalFrameDurationTicks.store(0, std::memory_order_relaxed);
    m_syncEstimatedFrameDurationTicks.store(0, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_syncQueue.clear();
        m_syncClockSeconds = 0.0;
        m_syncLastPresentedTimestampTicks = -1;
    }

    IMFAttributes* pAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr) || !pAttributes) {
        print_debug_message(
            "VideoDecoder::open failed to configure attributes.");
        if (pAttributes) {
            pAttributes->Release();
        }
        return false;
    }

    enable_hardware_acceleration(pAttributes);

    hr = pAttributes->SetUINT32(
        MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);

    if (FAILED(hr)) {
        print_debug_message(
            "VideoDecoder::open failed to configure attributes.");
        pAttributes->Release();
        return false;
    }

    hr = MFCreateSourceReaderFromURL(filename, pAttributes, &m_pReader);
    pAttributes->Release();

    if (FAILED(hr)) {
        m_pReader = nullptr;
        print_debug_message(std::format(
            "VideoDecoder::open MFCreateSourceReaderFromURL failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        return false;
    }

    if (!configure_reader_rgb32(m_pReader, file_utf8) ||
        !query_video_format(m_pReader, m_width, m_height, m_stride,
                            m_nominalFrameDurationTicks, file_utf8)) {
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    m_duration = query_duration_seconds(m_pReader);

    m_isPlaying = false;
    m_decodeThread = std::thread(&VideoDecoder::decode_loop, this);

    m_isLoaded = true;
    print_debug_message(std::format(
        "VideoDecoder::open successfully opened '{}', width={}, height={}.",
        file_utf8, static_cast<unsigned long>(m_width),
        static_cast<unsigned long>(m_height)));
    return true;
}

void VideoDecoder::close(bool cleanup) {
    if (!m_isLoaded && !m_pReader && !m_decodeThread.joinable()) {
        // Avoid noisy logs when close() is called on an already-closed decoder.
        return;
    }

    print_debug_message("VideoDecoder::close requested.");

    m_stopRequested = true;
    m_isPlaying = false;

    {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_syncQueue.clear();
    }
    m_syncCvFrameAvailable.notify_all();
    m_syncCvQueueSpace.notify_all();

    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
        print_debug_message("VideoDecoder::close joined decode thread.");
    }

    if (!cleanup && m_pReader) {
        m_pReader->Release();
        m_pReader = nullptr;
        print_debug_message("VideoDecoder::close released source reader.");
    }

    m_isLoaded = false;
    m_isFrameReady = false;
    m_isFinished = false;
    m_seekTarget = -1;

    print_debug_message("VideoDecoder::close completed.");
}

void VideoDecoder::set_pause(bool pause) {
    m_isPlaying = !pause;

    // Wake sync consumers/producers so they can re-evaluate their wait
    // predicates (pause may stop new frames from arriving).
    m_syncCvFrameAvailable.notify_all();
    m_syncCvQueueSpace.notify_all();

    if (pause) {
        print_debug_message("VideoDecoder::set_pause -> paused.");
    } else {
        print_debug_message("VideoDecoder::set_pause -> playing.");

        if (m_isFinished) {
            // If we were at the end, seek back to start when resuming.
            seek(0.0);
            m_isFinished = false;
            print_debug_message(
                "VideoDecoder::set_pause resumed from end of stream; seeking "
                "to start.");
        }
    }
}

void VideoDecoder::set_sync_mode(bool enable) {
    const bool wasEnabled =
        m_isSyncMode.exchange(enable, std::memory_order_relaxed);
    if (wasEnabled == enable) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_syncQueue.clear();
        m_syncClockSeconds = 0.0;
        m_syncLastPresentedTimestampTicks = -1;
    }

    // Prevent stale frames from being observable through the legacy API when
    // the caller accidentally mixes modes.
    if (enable) {
        m_isFrameReady = false;
    }

    m_syncCvFrameAvailable.notify_all();
    m_syncCvQueueSpace.notify_all();
}

void VideoDecoder::seek(double seconds) {
    const long long targetTicks = static_cast<long long>(seconds * 10000000.0);

    if (m_isSyncMode.load(std::memory_order_relaxed)) {
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            m_syncQueue.clear();
            m_syncClockSeconds = seconds;
            m_syncLastPresentedTimestampTicks = -1;
        }

        m_syncCvFrameAvailable.notify_all();
        m_syncCvQueueSpace.notify_all();
    }

    if (m_duration > 0.0 && seconds >= m_duration) {
        m_isFinished = true;
        set_pause(true);
        print_debug_message(std::format(
            "VideoDecoder::seek target {} exceeds duration {}, finishing.",
            seconds, m_duration));
        return;
    }

    // Seek target is expressed in 100-nanosecond units as required by MF.
    m_seekTarget = targetTicks;
    print_debug_message(
        std::format("VideoDecoder::seek scheduled to {} seconds ({} ticks).",
                    seconds, targetTicks));
}

double VideoDecoder::get_duration() {
    return m_duration;
}

double VideoDecoder::get_position() {
    const long long timestampTicks =
        m_lastDecodedTimestampTicks.load(std::memory_order_relaxed);

    if (timestampTicks <= 0) {
        return 0.0;
    }

    return static_cast<double>(timestampTicks) / 10.0;
}

double VideoDecoder::get_frame(void* gm_buffer_ptr, double buffer_size) {
    if (m_isSyncMode.load(std::memory_order_relaxed)) {
        // In sync mode frames are consumed via get_frame_sync() only.
        return 0.0;
    }

    if (!m_isFrameReady) {
        return 0.0;
    }

    std::lock_guard<std::mutex> lock(m_frameMutex);

    size_t requiredSize = m_pixelBuffer.size();
    if (requiredSize == 0 || buffer_size < requiredSize) {
        if (requiredSize > 0 && buffer_size < requiredSize) {
            print_debug_message(std::format(
                "VideoDecoder::get_frame buffer too small: have {}, need {}.",
                static_cast<unsigned long long>(buffer_size),
                static_cast<unsigned long long>(requiredSize)));
        }
        return 0.0;
    }

    memcpy(gm_buffer_ptr, m_pixelBuffer.data(), requiredSize);

    m_isFrameReady = false;
    return 1.0;
}

static long long pick_frame_duration_ticks(
    const std::atomic<long long>& nominal,
    const std::atomic<long long>& estimated) {
    const long long est = estimated.load(std::memory_order_relaxed);
    if (est > 0) {
        return est;
    }

    const long long nom = nominal.load(std::memory_order_relaxed);
    if (nom > 0) {
        return nom;
    }

    return 0;
}

long long VideoDecoder::compute_next_due_ticks_locked() const {
    if (!m_syncQueue.empty()) {
        return m_syncQueue.front().timestampTicks;
    }

    if (m_syncLastPresentedTimestampTicks < 0) {
        return 0;
    }

    const long long frameDurTicks = pick_frame_duration_ticks(
        m_nominalFrameDurationTicks, m_syncEstimatedFrameDurationTicks);
    if (frameDurTicks <= 0) {
        return -1;
    }

    return m_syncLastPresentedTimestampTicks + frameDurTicks;
}

bool VideoDecoder::wait_for_due_sync_frame_locked(
    std::unique_lock<std::mutex>& lock) {
    while (m_syncQueue.empty() &&
           m_isSyncMode.load(std::memory_order_relaxed) && !m_stopRequested &&
           !m_isFinished && m_isPlaying) {
        m_syncCvFrameAvailable.wait(lock);
    }

    if (!m_isSyncMode.load(std::memory_order_relaxed) || m_stopRequested ||
        !m_isPlaying) {
        return false;
    }

    return !m_syncQueue.empty();
}

bool VideoDecoder::can_copy_front_sync_frame_locked(long long clockTicks,
                                                    double buffer_size) const {
    if (m_syncQueue.empty()) {
        return false;
    }

    const SyncFrame& frame = m_syncQueue.front();
    if (frame.timestampTicks > clockTicks) {
        return false;
    }

    const size_t requiredSize = frame.pixels.size();
    if (requiredSize == 0 || buffer_size < requiredSize) {
        return false;
    }

    return true;
}

double VideoDecoder::copy_front_sync_frame_locked(
    void* gm_buffer_ptr, std::unique_lock<std::mutex>& lock) {
    const SyncFrame& frame = m_syncQueue.front();
    memcpy(gm_buffer_ptr, frame.pixels.data(), frame.pixels.size());
    m_syncLastPresentedTimestampTicks = frame.timestampTicks;
    m_syncQueue.pop_front();

    lock.unlock();
    m_syncCvQueueSpace.notify_one();

    return 1.0;
}

double VideoDecoder::get_frame_sync_locked(void* gm_buffer_ptr,
                                           double buffer_size,
                                           long long clockTicks,
                                           std::unique_lock<std::mutex>& lock) {
    const long long nextDueTicks = compute_next_due_ticks_locked();
    if (nextDueTicks < 0) {
        return 0.0;
    }

    if (clockTicks < nextDueTicks) {
        return 0.0;
    }

    if (!wait_for_due_sync_frame_locked(lock)) {
        return 0.0;
    }

    if (!can_copy_front_sync_frame_locked(clockTicks, buffer_size)) {
        return 0.0;
    }

    return copy_front_sync_frame_locked(gm_buffer_ptr, lock);
}

double VideoDecoder::get_frame_sync(void* gm_buffer_ptr, double buffer_size,
                                    double delta_time) {
    if (!m_isSyncMode.load(std::memory_order_relaxed)) {
        return 0.0;
    }

    if (!gm_buffer_ptr) {
        return 0.0;
    }

    if (!(delta_time > 0.0)) {
        delta_time = 0.0;
    }

    std::unique_lock<std::mutex> lock(m_syncMutex);

    m_syncClockSeconds += delta_time;
    const long long clockTicks =
        static_cast<long long>(std::llround(m_syncClockSeconds * 10000000.0));

    if (!m_isPlaying) {
        return 0.0;
    }

    return get_frame_sync_locked(gm_buffer_ptr, buffer_size, clockTicks, lock);
}

double VideoDecoder::get_width() {
    return static_cast<double>(m_width);
}

double VideoDecoder::get_height() {
    return static_cast<double>(m_height);
}
