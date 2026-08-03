#ifndef HELGA_FLIGHT_RECORDER_H
#define HELGA_FLIGHT_RECORDER_H

// HelgaFlightRecorder -- records the aircraft's transform and control
// inputs every physics frame during a flight, and can save/load that
// recording to disk so a "mission" can be replayed later.
//
// This only owns *what the aircraft did*. It knows nothing about
// cameras -- playback (see godot/scripts/replay_controller.gd) drives
// the aircraft's transform directly from recorded samples while leaving
// every camera completely free to switch/move independently, which is
// what makes a DCS/Tacview-style cinematic replay (fly the camera
// wherever you want over a fixed, repeatable flight, then record the
// result in OBS) possible.

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>

namespace godot {

struct FlightRecordSample {
    float time = 0.0f;
    Vector3 position;
    Quaternion rotation;
    float elevator = 0.0f;
    float aileron = 0.0f;
    float rudder = 0.0f;
    float throttle = 0.0f;
    int32_t flight_state = 0;
};

class HelgaFlightRecorder : public Node {
    GDCLASS(HelgaFlightRecorder, Node)

private:
    std::vector<FlightRecordSample> samples;
    bool recording = false;
    float elapsed = 0.0f;

    static Dictionary sample_to_dictionary(const FlightRecordSample &s);

protected:
    static void _bind_methods();

public:
    HelgaFlightRecorder();
    ~HelgaFlightRecorder() override;

    void start_recording();
    void stop_recording();
    bool is_recording() const { return recording; }
    void clear();

    void record_sample(const Vector3 &position, const Quaternion &rotation,
                        float elevator, float aileron, float rudder, float throttle,
                        int flight_state, float delta);

    int get_sample_count() const;
    float get_duration() const;

    // Interpolated lookup for playback -- Dictionary keys: position,
    // rotation, elevator, aileron, rudder, throttle, flight_state. Empty
    // if nothing has been recorded yet.
    Dictionary sample_at_time(float t) const;

    bool save_to_file(const String &path) const;
    bool load_from_file(const String &path);
};

}

#endif // HELGA_FLIGHT_RECORDER_H
