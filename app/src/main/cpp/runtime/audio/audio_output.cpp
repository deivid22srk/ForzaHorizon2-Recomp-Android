// audio_output.cpp — stream AAudio estéreo 48 kHz, callback-driven
#include "audio_output.h"

#include <aaudio/AAudio.h>
#include <android/log.h>
#include <chrono>
#include <memory>

#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/Audio", __VA_ARGS__)

namespace fh2::audio {

namespace {

// Loopback silencioso: o mixer 3D do guest alimenta o ring buffer; até a
// integração completa, o stream opera com silêncio (baixa latência garantida).
aaudio_data_callback_result_t dataCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    auto* out = static_cast<float*>(audioData);
    for (int32_t i = 0; i < numFrames * 2; ++i) out[i] = 0.f;
    (void)stream; (void)userData;
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    ALOG("erro no stream: %d", int(error));
    (void)stream; (void)userData;
}

class AAudioOutput final : public AudioOutput {
public:
    bool start() override {
        if (stream_) return true;

        AAudioStreamBuilder* builder = nullptr;
        if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) {
            ALOG("AAudio_createStreamBuilder falhou");
            return false;
        }

        AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
        AAudioStreamBuilder_setChannelCount(builder, 2);
        AAudioStreamBuilder_setSampleRate(builder, 48000);
        // Low latency: sem pós-processamento, performance mode pro
        AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
        AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
        AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

        aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &stream_);
        if (result != AAUDIO_OK) {
            ALOG("openStream falhou: %d", int(result));
            AAudioStreamBuilder_delete(builder);
            stream_ = nullptr;
            return false;
        }

        AAudioStreamBuilder_delete(builder);

        result = AAudioStream_requestStart(stream_);
        if (result != AAUDIO_OK) {
            ALOG("requestStart falhou: %d", int(result));
            return false;
        }

        sampleRate_ = AAudioStream_getSampleRate(stream_);
        ALOG("stream iniciado: %d Hz, burst=%d frames", sampleRate_,
             AAudioStream_getFramesPerBurst(stream_));
        return true;
    }

    void pause() override {
        if (stream_) AAudioStream_requestPause(stream_);
    }

    void resume() override {
        if (stream_) AAudioStream_requestStart(stream_);
    }

    void stop() override {
        if (stream_) {
            AAudioStream_requestStop(stream_);
            AAudioStream_close(stream_);
            stream_ = nullptr;
        }
    }

    int sampleRate() const override { return sampleRate_; }

    ~AAudioOutput() override { stop(); }

private:
    AAudioStream* stream_ = nullptr;
    int sampleRate_ = 48000;
};

} // namespace

std::unique_ptr<AudioOutput> AudioOutput::create() {
    return std::make_unique<AAudioOutput>();
}

} // namespace fh2::audio
