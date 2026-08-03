#include "audio_controller.h"
#include "aerodynamics.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace {
constexpr float TWO_PI = 6.283185307179586f;

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}
}

HelgaAudioController::HelgaAudioController() {}

HelgaAudioController::~HelgaAudioController() {}

void HelgaAudioController::find_aerodynamics_sibling() {
    Node *parent = get_parent();
    if (parent == nullptr) {
        return;
    }
    for (int i = 0; i < parent->get_child_count(); i++) {
        HelgaAerodynamics *candidate = Object::cast_to<HelgaAerodynamics>(parent->get_child(i));
        if (candidate != nullptr) {
            aerodynamics = candidate;
            return;
        }
    }
}

void HelgaAudioController::_ready() {
    find_aerodynamics_sibling();

    Ref<AudioStreamGenerator> wind_gen;
    wind_gen.instantiate();
    wind_gen->set_mix_rate(mix_rate);
    wind_gen->set_buffer_length(0.3);

    wind_player = memnew(AudioStreamPlayer);
    add_child(wind_player);
    wind_player->set_stream(wind_gen);
    wind_player->set_volume_db(-80.0);
    wind_player->play();

    Ref<AudioStreamGenerator> engine_gen;
    engine_gen.instantiate();
    engine_gen->set_mix_rate(mix_rate);
    engine_gen->set_buffer_length(0.3);

    engine_player = memnew(AudioStreamPlayer);
    add_child(engine_player);
    engine_player->set_stream(engine_gen);
    engine_player->set_volume_db(-80.0);
    engine_player->play();
}

float HelgaAudioController::next_noise() {
    // xorshift32 -- fast, self-contained, no external noise library needed
    // (same self-contained-procedural-generation philosophy as Aevoria
    // Simulator's noise-based texture generator).
    noise_state ^= noise_state << 13;
    noise_state ^= noise_state >> 17;
    noise_state ^= noise_state << 5;
    return (static_cast<float>(noise_state) / static_cast<float>(UINT32_MAX)) * 2.0f - 1.0f;
}

void HelgaAudioController::fill_wind_buffer(int frames) {
    double airspeed = aerodynamics != nullptr ? aerodynamics->get_airspeed() : 0.0;
    // Cutoff rises with airspeed: a low rumble at low speed, a brighter
    // hiss at high speed, via a simple one-pole low-pass filter over
    // white noise.
    float cutoff = 0.02f + static_cast<float>(std::min(airspeed, 300.0) / 300.0) * 0.35f;

    PackedVector2Array buffer;
    buffer.resize(frames);
    for (int i = 0; i < frames; i++) {
        float white = next_noise();
        wind_filter_state += cutoff * (white - wind_filter_state);
        buffer[i] = Vector2(wind_filter_state, wind_filter_state);
    }
    if (wind_playback.is_valid()) {
        wind_playback->push_buffer(buffer);
    }
}

void HelgaAudioController::fill_engine_buffer(int frames) {
    double airspeed = aerodynamics != nullptr ? aerodynamics->get_airspeed() : 0.0;
    float base_freq = 60.0f + static_cast<float>(throttle) * 120.0f; // 60-180 Hz fundamental
    // Blend toward a noisier, higher-character "scramjet roar" as airspeed
    // climbs past ~150 m/s. There's no Mach-number/atmosphere model yet,
    // so raw airspeed stands in for now -- revisit once one exists.
    float scramjet_mix = static_cast<float>(std::min(std::max((airspeed - 150.0) / 150.0, 0.0), 1.0));

    PackedVector2Array buffer;
    buffer.resize(frames);
    float phase_step = TWO_PI * base_freq / static_cast<float>(mix_rate);
    float harmonic_step = TWO_PI * (base_freq * 2.01f) / static_cast<float>(mix_rate);

    for (int i = 0; i < frames; i++) {
        engine_phase += phase_step;
        if (engine_phase > TWO_PI) engine_phase -= TWO_PI;
        engine_harmonic_phase += harmonic_step;
        if (engine_harmonic_phase > TWO_PI) engine_harmonic_phase -= TWO_PI;

        float tone = std::sin(engine_phase) * 0.6f + std::sin(engine_harmonic_phase) * 0.25f;
        float roar = next_noise() * 0.5f;
        float sample = lerpf(tone, roar, scramjet_mix);
        sample = std::max(-1.0f, std::min(1.0f, sample * static_cast<float>(throttle)));
        buffer[i] = Vector2(sample, sample);
    }
    if (engine_playback.is_valid()) {
        engine_playback->push_buffer(buffer);
    }
}

void HelgaAudioController::_process(double p_delta) {
    // Lazily acquire the generator playbacks -- get_stream_playback()
    // can return null for the first frame or two after play() until the
    // audio thread actually starts consuming the stream.
    if (wind_playback.is_null() && wind_player != nullptr) {
        Ref<AudioStreamPlayback> generic = wind_player->get_stream_playback();
        wind_playback = Ref<AudioStreamGeneratorPlayback>(Object::cast_to<AudioStreamGeneratorPlayback>(generic.ptr()));
    }
    if (engine_playback.is_null() && engine_player != nullptr) {
        Ref<AudioStreamPlayback> generic = engine_player->get_stream_playback();
        engine_playback = Ref<AudioStreamGeneratorPlayback>(Object::cast_to<AudioStreamGeneratorPlayback>(generic.ptr()));
    }

    double airspeed = aerodynamics != nullptr ? aerodynamics->get_airspeed() : 0.0;

    double wind_volume_db = -80.0 + std::min(airspeed, 300.0) / 300.0 * 74.0; // -80..-6 dB
    if (wind_player != nullptr) {
        wind_player->set_volume_db(wind_volume_db);
    }

    double engine_volume_db = -80.0 + throttle * 68.0; // -80..-12 dB
    if (engine_player != nullptr) {
        engine_player->set_volume_db(engine_volume_db);
    }

    if (wind_playback.is_valid()) {
        int frames = wind_playback->get_frames_available();
        if (frames > 0) fill_wind_buffer(frames);
    }
    if (engine_playback.is_valid()) {
        int frames = engine_playback->get_frames_available();
        if (frames > 0) fill_engine_buffer(frames);
    }
}

void HelgaAudioController::_exit_tree() {
    // Explicitly release these Refs rather than relying on this object's
    // destructor running before Godot's own leak-check pass -- otherwise
    // AudioStreamGeneratorPlayback shows up as leaked at shutdown.
    wind_playback = Ref<AudioStreamGeneratorPlayback>();
    engine_playback = Ref<AudioStreamGeneratorPlayback>();
}

void HelgaAudioController::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_throttle"), &HelgaAudioController::get_throttle);
    ClassDB::bind_method(D_METHOD("set_throttle", "value"), &HelgaAudioController::set_throttle);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "throttle", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_throttle", "get_throttle");
}
