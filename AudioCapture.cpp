#include "AudioCapture.h"
#include <cmath>
#include <algorithm>
#include <iostream>

AudioCapture::AudioCapture()
{
    // Initialize COM in apartment threading mode
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        std::cerr << "Failed to initialize COM: 0x" << std::hex << hr << std::endl;
    }
}

AudioCapture::~AudioCapture()
{
    if (pCaptureClient)
        pCaptureClient->Release();
    if (pAudioClient)
        pAudioClient->Release();
    if (pDevice)
        pDevice->Release();
    if (pEnumerator)
        pEnumerator->Release();
    if (pwfx)
        CoTaskMemFree(pwfx);
    CoUninitialize();
}

bool AudioCapture::Initialize()
{
    HRESULT hr;

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator, nullptr,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void **)&pEnumerator);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create device enumerator: 0x" << std::hex << hr << std::endl;
        return false;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr))
    {
        std::cerr << "Failed to get default audio endpoint: 0x" << std::hex << hr << std::endl;
        return false;
    }

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void **)&pAudioClient);
    if (FAILED(hr))
    {
        std::cerr << "Failed to activate audio client: 0x" << std::hex << hr << std::endl;
        return false;
    }

    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr))
    {
        std::cerr << "Failed to get mix format: 0x" << std::hex << hr << std::endl;
        return false;
    }

    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0, pwfx, 0);
    if (FAILED(hr))
    {
        std::cerr << "Failed to initialize audio client: 0x" << std::hex << hr << std::endl;
        return false;
    }

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void **)&pCaptureClient);
    if (FAILED(hr))
    {
        std::cerr << "Failed to get capture client: 0x" << std::hex << hr << std::endl;
        return false;
    }

    hr = pAudioClient->Start();
    if (FAILED(hr))
    {
        std::cerr << "Failed to start audio client: 0x" << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool AudioCapture::GetAudioData(std::vector<float> &audioData)
{
    if (!pCaptureClient)
        return false;

    UINT32 packetLength = 0;
    HRESULT hr = pCaptureClient->GetNextPacketSize(&packetLength);
    if (FAILED(hr))
        return false;

    audioData.clear();
    std::vector<float> tempData;

    while (packetLength > 0)
    {
        BYTE *data;
        UINT32 numFramesAvailable;
        DWORD flags;

        hr = pCaptureClient->GetBuffer(&data, &numFramesAvailable, &flags, nullptr, nullptr);
        if (FAILED(hr))
            return false;

        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
        {
            float *floatData = (float *)data;
            const int NUM_BANDS = 20;
            std::vector<float> bands(NUM_BANDS, 0.0f);

            for (UINT32 i = 0; i < numFramesAvailable * pwfx->nChannels; i += pwfx->nChannels)
            {
                int band = (i * NUM_BANDS) / (numFramesAvailable * pwfx->nChannels);

                float sum = 0;
                for (UINT16 channel = 0; channel < pwfx->nChannels; channel++)
                {
                    sum += std::abs(floatData[i + channel]);
                }
                bands[band] += sum / pwfx->nChannels;
            }

            for (int i = 0; i < NUM_BANDS; i++)
            {
                tempData.push_back(bands[i] / (numFramesAvailable / NUM_BANDS));
            }
        }

        hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
        if (FAILED(hr))
            return false;

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr))
            return false;
    }

    if (!tempData.empty())
    {
        audioData = std::move(tempData);
    }

    return !audioData.empty();
}