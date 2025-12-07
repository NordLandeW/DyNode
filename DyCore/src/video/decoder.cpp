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
#include <format>
#include <string>

#include "utils.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

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
        // If the reader has been released, there is nothing left to decode.
        if (!m_pReader) {
            print_debug_message(
                "VideoDecoder::decode_loop exiting because m_pReader is null.");
            break;
        }

        // Respect pause state without destroying decoder resources so toggling
        // play/pause stays cheap.
        if (!m_isPlaying) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Perform any pending seek on the decode thread to keep MF calls
        // confined to a single COM apartment.
        long long seekPos = m_seekTarget.exchange(-1);
        if (seekPos >= 0) {
            // Check if we can perform a "smart seek" (skip forward) without
            // resetting to a keyframe. This is faster if the target is close to
            // the current position.
            bool useSmartSeek = false;
            // 10 secs
            const long long SMART_SEEK_THRESHOLD = 10 * 10000000LL;

            if (m_lastPresentationTime > 0 &&
                seekPos > m_lastPresentationTime) {
                long long diff = seekPos - m_lastPresentationTime;
                if (diff < SMART_SEEK_THRESHOLD) {
                    useSmartSeek = true;
                }
            }

            if (useSmartSeek) {
                skipUntilTime = seekPos;
                skipStartTime = std::chrono::high_resolution_clock::now();
                print_debug_message(
                    std::format("VideoDecoder::decode_loop performing smart "
                                "seek to {} ticks (diff: {}).",
                                seekPos, seekPos - m_lastPresentationTime));
            } else {
                PROPVARIANT var;
                PropVariantInit(&var);
                var.vt = VT_I8;
                var.hVal.QuadPart = seekPos;
                HRESULT seekHr = m_pReader->SetCurrentPosition(GUID_NULL, var);
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
                PropVariantClear(&var);
            }

            // Reset timing logic so we don't sleep for the seek difference.
            // Even for smart seek, we want to display the target frame
            // immediately.
            m_lastPresentationTime = 0;
        }

        // Read next frame from the source.
        IMFSample* pSample = nullptr;
        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG llTimeStamp = 0;

        hr =
            m_pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                                  &streamIndex, &flags, &llTimeStamp, &pSample);

        if (FAILED(hr)) {
            if (pSample) {
                pSample->Release();
            }
            // Abort the loop on hard read errors to avoid a busy retry spin.
            print_debug_message(std::format(
                "VideoDecoder::decode_loop ReadSample failed, hr=0x{:08X}",
                static_cast<unsigned long>(hr)));
            break;
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            if (pSample) {
                pSample->Release();
            }
            // Mark end of stream so callers can differentiate EOS from pause.
            m_isFinished = true;
            m_isPlaying = false;
            skipUntilTime = -1;
            print_debug_message(
                "VideoDecoder::decode_loop reached end of stream.");
        }

        if (pSample) {
            if (skipUntilTime >= 0) {
                if (llTimeStamp <
                    skipUntilTime +
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::high_resolution_clock::now() -
                            skipStartTime)
                                .count() /
                            100) {
                    pSample->Release();
                    continue;
                }
                skipUntilTime = -1;
            }

            IMFMediaBuffer* pBuffer = nullptr;
            hr = pSample->GetBufferByIndex(0, &pBuffer);
            if (FAILED(hr) || !pBuffer) {
                if (pBuffer) {
                    pBuffer->Release();
                }
                pSample->Release();
                print_debug_message(std::format(
                    "VideoDecoder::decode_loop GetBufferByIndex failed, "
                    "hr=0x{:08X}",
                    static_cast<unsigned long>(hr)));
                continue;
            }

            BYTE* pData = nullptr;
            DWORD currLen = 0;
            LONG lStride = 0;
            IMF2DBuffer* p2DBuffer = nullptr;
            bool locked2D = false;
            BYTE* pScanline0 = nullptr;

            // Try to get IMF2DBuffer interface for stride info
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
                    pSample->Release();
                    print_debug_message(std::format(
                        "VideoDecoder::decode_loop buffer lock failed, "
                        "hr=0x{:08X}",
                        static_cast<unsigned long>(hr)));
                    continue;
                }
            }

            {
                // Protect the shared pixel buffer when swapping in new frame
                // data.
                std::lock_guard<std::mutex> lock(m_frameMutex);

                // Ensure pixel buffer is packed (width * height * 4)
                size_t expectedSize = m_width * m_height * 4;
                if (m_pixelBuffer.size() != expectedSize) {
                    m_pixelBuffer.resize(expectedSize);
                    print_debug_message(std::format(
                        "VideoDecoder::decode_loop resized pixel buffer to "
                        "{} bytes ({}x{}x4).",
                        static_cast<unsigned long>(expectedSize), m_width,
                        m_height));
                }

                if (locked2D) {
                    // Copy row-by-row to handle stride
                    BYTE* dst = m_pixelBuffer.data();
                    BYTE* src = pScanline0;
                    size_t rowBytes = m_width * 4;

                    for (UINT i = 0; i < m_height; ++i) {
                        memcpy(dst + i * rowBytes,
                               src + (static_cast<LONG>(i) * lStride),
                               rowBytes);
                    }

                    p2DBuffer->Unlock2D();
                    p2DBuffer->Release();
                } else {
                    // Fallback to Lock()
                    // Use the stride calculated during open()
                    LONG lSrcStride = m_stride;

                    BYTE* dst = m_pixelBuffer.data();
                    BYTE* src = pData;
                    size_t rowBytes = m_width * 4;

                    for (UINT i = 0; i < m_height; ++i) {
                        // Ensure we don't read past the locked buffer
                        if (src + rowBytes > pData + currLen)
                            break;

                        memcpy(dst, src, rowBytes);

                        dst += rowBytes;    // Destination is packed
                        src += lSrcStride;  // Source has stride
                    }

                    pBuffer->Unlock();
                }

                m_isFrameReady = true;
            }

            pBuffer->Release();
            pSample->Release();

            // Calculate delay based on presentation timestamps
            long long durationMs = 0;
            if (m_lastPresentationTime > 0 &&
                llTimeStamp > m_lastPresentationTime) {
                long long diff = llTimeStamp - m_lastPresentationTime;

                // Prevent precision loss
                m_nextFrameTargetTime += std::chrono::nanoseconds(diff * 100);

                std::this_thread::sleep_until(m_nextFrameTargetTime);
            } else {
                // First frame after start or seek, or invalid timestamp order.
                // Reset the target clock to now.
                m_nextFrameTargetTime =
                    std::chrono::high_resolution_clock::now();
            }

            m_lastPresentationTime = llTimeStamp;
        }
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

    IMFAttributes* pAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);

    // [Hardware Acceleration] Start ------------------------------------------
    if (SUCCEEDED(hr)) {
        // Create D3D11 device with video and BGRA support so MF can leverage
        // DXVA.
        ID3D11Device* pD3DDevice = nullptr;
        ID3D11DeviceContext* pD3DContext = nullptr;
        UINT creationFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT |
                             D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        // creationFlags |= D3D11_CREATE_DEVICE_DEBUG; // enable for D3D
        // debugging

        D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1,
                                             D3D_FEATURE_LEVEL_11_0};
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

        HRESULT d3dHr =
            D3D11CreateDevice(nullptr,                   // Default adapter
                              D3D_DRIVER_TYPE_HARDWARE,  // HW driver
                              nullptr,        // No software rasterizer
                              creationFlags,  // Flags
                              featureLevels,  // Requested feature levels
                              ARRAYSIZE(featureLevels),  // Count
                              D3D11_SDK_VERSION,         // SDK
                              &pD3DDevice,               // Out: device
                              &featureLevel,  // Out: negotiated feature level
                              &pD3DContext    // Out: context
            );

        if (SUCCEEDED(d3dHr)) {
            // Ensure thread safety as MF may call into D3D from different
            // threads.
            ID3D11Multithread* pMultithread = nullptr;
            if (SUCCEEDED(
                    pD3DDevice->QueryInterface(IID_PPV_ARGS(&pMultithread)))) {
                pMultithread->SetMultithreadProtected(TRUE);
                pMultithread->Release();
            }

            // Create DXGI Device Manager and associate the D3D device.
            UINT resetToken = 0;
            IMFDXGIDeviceManager* pDXGIManager = nullptr;
            d3dHr = MFCreateDXGIDeviceManager(&resetToken, &pDXGIManager);
            if (SUCCEEDED(d3dHr)) {
                d3dHr = pDXGIManager->ResetDevice(pD3DDevice, resetToken);
                if (SUCCEEDED(d3dHr)) {
                    // Provide the DXGI manager to the source reader for DXVA.
                    pAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER,
                                            pDXGIManager);
                    pAttributes->SetUINT32(
                        MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
                    print_debug_message(
                        "VideoDecoder::open Hardware Acceleration enabled "
                        "(D3D11).");
                }
                // Attribute holds a reference; release our local one.
                pDXGIManager->Release();
            }

            if (pD3DContext)
                pD3DContext->Release();
            if (pD3DDevice)
                pD3DDevice->Release();
        } else {
            print_debug_message(std::format(
                "VideoDecoder::open D3D11CreateDevice failed, hr=0x{:08X}",
                static_cast<unsigned long>(d3dHr)));
        }
    }

    hr = pAttributes->SetUINT32(
        MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);

    if (FAILED(hr)) {
        print_debug_message(
            "VideoDecoder::open failed to configure attributes.");
        if (pAttributes)
            pAttributes->Release();
        return false;
    }

    hr = MFCreateSourceReaderFromURL(filename, pAttributes, &m_pReader);
    if (pAttributes) {
        pAttributes->Release();
        pAttributes = nullptr;
    }

    if (FAILED(hr)) {
        m_pReader = nullptr;
        print_debug_message(std::format(
            "VideoDecoder::open MFCreateSourceReaderFromURL failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        return false;
    }

    IMFMediaType* pType = nullptr;
    hr = MFCreateMediaType(&pType);
    if (FAILED(hr)) {
        print_debug_message(
            std::format("VideoDecoder::open MFCreateMediaType failed for '{}', "
                        "hr=0x{:08X}",
                        file_utf8, static_cast<unsigned long>(hr)));
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open SetGUID(MF_MT_MAJOR_TYPE) failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        pType->Release();
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open SetGUID(MF_MT_SUBTYPE=RGB32) failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        pType->Release();
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    hr = m_pReader->SetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);

    pType->Release();
    pType = nullptr;

    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open SetCurrentMediaType RGB32 failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    IMFMediaType* pCurrentType = nullptr;
    hr = m_pReader->GetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open GetCurrentMediaType failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    hr =
        MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &m_width, &m_height);

    if (SUCCEEDED(hr)) {
        // Try to get stride from Media Type
        HRESULT strideHr =
            pCurrentType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&m_stride);

        // If failed, calculate manually
        if (FAILED(strideHr)) {
            strideHr = MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1,
                                                      m_width, &m_stride);
            if (FAILED(strideHr)) {
                // Fallback: assume packed
                m_stride = m_width * 4;
            }
        }

        if (m_stride < 0)
            m_stride = -m_stride;

        print_debug_message(
            std::format("VideoDecoder::open Stride set to {}.", m_stride));
    }

    pCurrentType->Release();
    if (FAILED(hr)) {
        print_debug_message(std::format(
            "VideoDecoder::open MFGetAttributeSize failed for '{}', "
            "hr=0x{:08X}",
            file_utf8, static_cast<unsigned long>(hr)));
        m_pReader->Release();
        m_pReader = nullptr;
        return false;
    }

    PROPVARIANT var;
    PropVariantInit(&var);
    hr = m_pReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,
                                             MF_PD_DURATION, &var);
    if (SUCCEEDED(hr)) {
        long long durationVal = 0;
        if (var.vt == VT_UI8) {
            durationVal = var.uhVal.QuadPart;
        } else if (var.vt == VT_I8) {
            durationVal = var.hVal.QuadPart;
        }
        m_duration = static_cast<double>(durationVal) / 10000000.0;
        PropVariantClear(&var);
    } else {
        m_duration = 0.0;
        print_debug_message(
            "VideoDecoder::open warning: could not determine duration.");
    }

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

void VideoDecoder::seek(double seconds) {
    if (m_duration > 0.0 && seconds >= m_duration) {
        m_isFinished = true;
        set_pause(true);
        print_debug_message(std::format(
            "VideoDecoder::seek target {} exceeds duration {}, finishing.",
            seconds, m_duration));
        return;
    }

    // Seek target is expressed in 100-nanosecond units as required by MF.
    long long target = static_cast<long long>(seconds * 10000000.0);
    m_seekTarget = target;
    print_debug_message(
        std::format("VideoDecoder::seek scheduled to {} seconds ({} ticks).",
                    seconds, target));
}

double VideoDecoder::get_duration() {
    return m_duration;
}

double VideoDecoder::get_frame(void* gm_buffer_ptr, double buffer_size) {
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

double VideoDecoder::get_width() {
    return static_cast<double>(m_width);
}

double VideoDecoder::get_height() {
    return static_cast<double>(m_height);
}
