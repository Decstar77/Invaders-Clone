
#include "AttoAsset.h"


namespace atto
{
    bool LeEngine::InitializeAudio() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            ATTOERROR("Could not initialize COM");
            return false;
        }
        
        hr = XAudio2Create(&audio.xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) {
            ATTOERROR("Could not create XAudio2");
            return false;
        }

        hr = audio.xAudio2->CreateMasteringVoice(&audio.masteringVoice);
        if (FAILED(hr)) {
            ATTOERROR("Could not create mastering voice");
            return false;
        }

        WAVEFORMATEXTENSIBLE wfx = { 0 };
        XAUDIO2_BUFFER buffer = { 0 };

        return true;
    }

    void LeEngine::AudioCreate(AudioAsset& audio) {

    }

    //Speaker LeEngine::AudioPlay(AudioAssetId audioAssetId, bool looping, f32 volume /*= 1.0f*/) {
    //    const AudioAsset* audioAsset = LoadAudioAsset(audioAssetId);
    //    if (!audioAsset) {
    //        ATTOERROR("Could not load audio asset");
    //        return {};
    //    }

    //    const i32 speakerCount = speakers.GetCount();
    //    for (i32 speakerIndex = 0; speakerIndex < speakerCount; ++speakerIndex) {
    //        Speaker& speaker = speakers[speakerIndex];
    //        if (speaker.sourceHandle == 0) {
    //            continue;
    //        }

    //        ALint state = {};
    //        alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
    //        if (state == AL_STOPPED) {
    //            alSourcei(speaker.sourceHandle, AL_BUFFER, audioAsset->bufferHandle);
    //            alSourcei(speaker.sourceHandle, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    //            alSourcef(speaker.sourceHandle, AL_GAIN, volume);
    //            alSourcePlay(speaker.sourceHandle);
    //            ALCheckErrors();

    //            return speaker;
    //        }
    //    }

    //    Speaker speaker = {};
    //    speaker.index = speakers.GetCount();
    //    alGenSources(1, &speaker.sourceHandle);
    //    alSourcei(speaker.sourceHandle, AL_BUFFER, audioAsset->bufferHandle);
    //    alSourcei(speaker.sourceHandle, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    //    alSourcef(speaker.sourceHandle, AL_GAIN, volume);
    //    alSourcePlay(speaker.sourceHandle);
    //    ALCheckErrors();
    //    speakers.Add(speaker);

    //    return speaker;
    //}

    //void LeEngine::AudioPause(Speaker speaker) {
    //    if (speaker.sourceHandle != 0) {
    //        alSourcePause(speaker.sourceHandle);
    //    }
    //}

    //void LeEngine::AudioStop(Speaker speaker) {
    //    if (speaker.sourceHandle != 0) {
    //        alSourceStop(speaker.sourceHandle);
    //    }
    //}

    //bool LeEngine::AudioIsSpeakerPlaying(Speaker speaker) {
    //    if (speaker.sourceHandle != 0) {
    //        ALint state = {};
    //        alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
    //        return  state == AL_PLAYING;
    //    }

    //    return false;
    //}

    //bool LeEngine::AudioIsSpeakerAlive(Speaker speaker) {
    //    if (speaker.sourceHandle != 0) {
    //        ALint state = {};
    //        alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
    //        return state != AL_STOPPED;
    //    }

    //    return false;
    //}
}
