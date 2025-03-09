#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys_devpkey.h>
#include <initguid.h>
#include <vector>

DEFINE_GUID(IID_IAudioClient,
            0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioCaptureClient,
            0xC8ADBD64, 0xE71E, 0x48A0, 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17);
DEFINE_GUID(CLSID_MMDeviceEnumerator,
            0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator,
            0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);

class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();
    bool Initialize();
    bool GetAudioData(std::vector<float> &audioData);
    void SetRandomFallback(bool enable) { useRandomFallback = enable; }
    bool IsUsingRandomFallback() const { return useRandomFallback; }

private:
    IMMDeviceEnumerator *pEnumerator = nullptr;    // Finds audio devices
    IMMDevice *pDevice = nullptr;                  // Represents the audio device
    IAudioClient *pAudioClient = nullptr;          // Manages audio session
    IAudioCaptureClient *pCaptureClient = nullptr; // Captures the audio data
    WAVEFORMATEX *pwfx = nullptr;                  // Audio format information
    bool useRandomFallback = false;
};