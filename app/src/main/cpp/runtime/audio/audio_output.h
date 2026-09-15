// audio_output.h — saída de áudio de baixa latência via AAudio (NDK nativo)
//
// Substitui XAudio2/OpenAL desktop. O mixer 3D (motor, colisões, rádio)
// consome os buffers deste stream (backlog: mixer posicional completo).
#pragma once

#include <atomic>
#include <cstdint>

namespace fh2::audio {

class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    static std::unique_ptr<AudioOutput> create();

    virtual bool start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;

    /** Taxa atual do stream (o guest resampleia para esta taxa). */
    virtual int sampleRate() const = 0;
};

} // namespace fh2::audio
