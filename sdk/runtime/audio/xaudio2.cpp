// mixer output through XAudio2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <xaudio2.h>

#include <thread>
#include <vector>

#include "audio/audio.h"
#include "core/log.h"
#include <mmsystem.h>

namespace audio {

struct VoiceCallback : public IXAudio2VoiceCallback {
    HANDLE buffer_event{nullptr};
    VoiceCallback() { buffer_event = CreateEventW(nullptr, FALSE, FALSE, nullptr); }
    ~VoiceCallback() { if (buffer_event) CloseHandle(buffer_event); }
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnBufferEnd(void*) override { if (buffer_event) SetEvent(buffer_event); }
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
};

void StartOutput(std::function<void(float*, uint32_t)> render) {
    std::thread([render] {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        timeBeginPeriod(1);
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IXAudio2* xa = nullptr;
        IXAudio2MasteringVoice* master = nullptr;
        IXAudio2SourceVoice* voice = nullptr;
        VoiceCallback callback;
        WAVEFORMATEX wf{};
        wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wf.nChannels = 2;
        wf.nSamplesPerSec = kRate;
        wf.wBitsPerSample = 32;
        wf.nBlockAlign = 8;
        wf.nAvgBytesPerSec = kRate * 8;
        if (FAILED(XAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR)) || FAILED(xa->CreateMasteringVoice(&master)) ||
            FAILED(xa->CreateSourceVoice(&voice, &wf, 0, 2.0f, &callback))) {
            LOGE("audio: XAudio2 unavailable, no sound");
            timeEndPeriod(1);
            return;
        }
        voice->Start();
        constexpr uint32_t kFrames = (kRate * 20) / 1000, kSlots = 6;
        std::vector<float> bufs(kSlots * kFrames * 2);
        for (uint32_t n = 0;;) {
            XAUDIO2_VOICE_STATE st;
            voice->GetState(&st, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (st.BuffersQueued >= 3) {
                WaitForSingleObject(callback.buffer_event, 25);
                continue;
            }
            float* b = &bufs[(n % kSlots) * kFrames * 2];
            render(b, kFrames);
            XAUDIO2_BUFFER xb{};
            xb.AudioBytes = kFrames * 8;
            xb.pAudioData = reinterpret_cast<const BYTE*>(b);
            voice->SubmitSourceBuffer(&xb);
            n++;  // only a submitted buffer takes a slot, XAudio2 still reads the queued ones
        }
        timeEndPeriod(1);
    }).detach();
}

}  // namespace audio
