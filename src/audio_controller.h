#ifndef HELGA_AUDIO_CONTROLLER_H
#define HELGA_AUDIO_CONTROLLER_H

// HelgaAudioController -- procedural wind and engine/scramjet sound.
//
// Both sounds are synthesized directly as PCM samples in C++ via Godot's
// AudioStreamGenerator, driven every frame by real flight parameters
// (airspeed from HelgaAerodynamics, throttle set by aircraft_control.gd)
// rather than played back from pre-recorded loops -- no audio assets
// required for a first pass. Recorded samples can be layered in later
// for extra fidelity; this only owns the procedural half.
//
// Expected scene layout: a child of the aircraft RigidBody3D, sibling to
// a HelgaAerodynamics node (found automatically among its siblings).
// Uses plain (non-positional) AudioStreamPlayers deliberately -- engine/
// wind sound is something the pilot hears regardless of which external
// camera is active, so it shouldn't attenuate with a chase/tracking
// camera's distance the way a 3D positional sound would.

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>

namespace godot {

class HelgaAerodynamics;

class HelgaAudioController : public Node {
    GDCLASS(HelgaAudioController, Node)

private:
    AudioStreamPlayer *wind_player = nullptr;
    AudioStreamPlayer *engine_player = nullptr;
    Ref<AudioStreamGeneratorPlayback> wind_playback;
    Ref<AudioStreamGeneratorPlayback> engine_playback;

    HelgaAerodynamics *aerodynamics = nullptr;

    double throttle = 0.0;
    double mix_rate = 44100.0;

    float wind_filter_state = 0.0f;
    float engine_phase = 0.0f;
    float engine_harmonic_phase = 0.0f;
    uint32_t noise_state = 0x1234567u;

    float next_noise();
    void fill_wind_buffer(int frames);
    void fill_engine_buffer(int frames);
    void find_aerodynamics_sibling();

protected:
    static void _bind_methods();

public:
    HelgaAudioController();
    ~HelgaAudioController() override;

    void _ready() override;
    void _process(double p_delta) override;
    void _exit_tree() override;

    double get_throttle() const { return throttle; }
    void set_throttle(double p_value) { throttle = p_value; }
};

}

#endif // HELGA_AUDIO_CONTROLLER_H
