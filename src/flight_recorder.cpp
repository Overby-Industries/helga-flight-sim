#include "flight_recorder.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace {
constexpr uint32_t FILE_MAGIC = 0x484C4746; // "HFLG"
constexpr uint32_t FILE_VERSION = 1;

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}
}

HelgaFlightRecorder::HelgaFlightRecorder() {}

HelgaFlightRecorder::~HelgaFlightRecorder() {}

void HelgaFlightRecorder::start_recording() {
    samples.clear();
    elapsed = 0.0f;
    recording = true;
}

void HelgaFlightRecorder::stop_recording() {
    recording = false;
}

void HelgaFlightRecorder::clear() {
    samples.clear();
    elapsed = 0.0f;
}

void HelgaFlightRecorder::record_sample(const Vector3 &position, const Quaternion &rotation,
                                         float elevator, float aileron, float rudder, float throttle,
                                         int flight_state, float delta) {
    if (!recording) {
        return;
    }
    elapsed += delta;

    FlightRecordSample s;
    s.time = elapsed;
    s.position = position;
    s.rotation = rotation;
    s.elevator = elevator;
    s.aileron = aileron;
    s.rudder = rudder;
    s.throttle = throttle;
    s.flight_state = flight_state;
    samples.push_back(s);
}

int HelgaFlightRecorder::get_sample_count() const {
    return static_cast<int>(samples.size());
}

float HelgaFlightRecorder::get_duration() const {
    return samples.empty() ? 0.0f : samples.back().time;
}

Dictionary HelgaFlightRecorder::sample_to_dictionary(const FlightRecordSample &s) {
    Dictionary result;
    result["position"] = s.position;
    result["rotation"] = s.rotation;
    result["elevator"] = s.elevator;
    result["aileron"] = s.aileron;
    result["rudder"] = s.rudder;
    result["throttle"] = s.throttle;
    result["flight_state"] = s.flight_state;
    return result;
}

Dictionary HelgaFlightRecorder::sample_at_time(float t) const {
    if (samples.empty()) {
        return Dictionary();
    }
    if (t <= samples.front().time) {
        return sample_to_dictionary(samples.front());
    }
    if (t >= samples.back().time) {
        return sample_to_dictionary(samples.back());
    }

    // Binary search for the first sample with time > t.
    size_t lo = 0;
    size_t hi = samples.size() - 1;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (samples[mid].time < t) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    const FlightRecordSample &b = samples[lo];
    const FlightRecordSample &a = samples[lo - 1];
    float span = b.time - a.time;
    float alpha = span > 0.0001f ? (t - a.time) / span : 0.0f;

    Dictionary result;
    result["position"] = a.position.lerp(b.position, alpha);
    result["rotation"] = a.rotation.slerp(b.rotation, alpha);
    result["elevator"] = lerpf(a.elevator, b.elevator, alpha);
    result["aileron"] = lerpf(a.aileron, b.aileron, alpha);
    result["rudder"] = lerpf(a.rudder, b.rudder, alpha);
    result["throttle"] = lerpf(a.throttle, b.throttle, alpha);
    result["flight_state"] = a.flight_state; // discrete -- no interpolation
    return result;
}

bool HelgaFlightRecorder::save_to_file(const String &path) const {
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null()) {
        return false;
    }
    file->store_32(FILE_MAGIC);
    file->store_32(FILE_VERSION);
    file->store_32(static_cast<uint32_t>(samples.size()));
    for (const FlightRecordSample &s : samples) {
        file->store_float(s.time);
        file->store_float(s.position.x);
        file->store_float(s.position.y);
        file->store_float(s.position.z);
        file->store_float(s.rotation.x);
        file->store_float(s.rotation.y);
        file->store_float(s.rotation.z);
        file->store_float(s.rotation.w);
        file->store_float(s.elevator);
        file->store_float(s.aileron);
        file->store_float(s.rudder);
        file->store_float(s.throttle);
        file->store_32(static_cast<uint32_t>(s.flight_state));
    }
    return true;
}

bool HelgaFlightRecorder::load_from_file(const String &path) {
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null()) {
        return false;
    }
    uint32_t magic = file->get_32();
    uint32_t version = file->get_32();
    if (magic != FILE_MAGIC || version != FILE_VERSION) {
        return false;
    }
    uint32_t count = file->get_32();

    std::vector<FlightRecordSample> loaded;
    loaded.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        FlightRecordSample s;
        s.time = file->get_float();
        s.position.x = file->get_float();
        s.position.y = file->get_float();
        s.position.z = file->get_float();
        s.rotation.x = file->get_float();
        s.rotation.y = file->get_float();
        s.rotation.z = file->get_float();
        s.rotation.w = file->get_float();
        s.elevator = file->get_float();
        s.aileron = file->get_float();
        s.rudder = file->get_float();
        s.throttle = file->get_float();
        s.flight_state = static_cast<int32_t>(file->get_32());
        loaded.push_back(s);
    }

    samples = std::move(loaded);
    elapsed = get_duration();
    recording = false;
    return true;
}

void HelgaFlightRecorder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_recording"), &HelgaFlightRecorder::start_recording);
    ClassDB::bind_method(D_METHOD("stop_recording"), &HelgaFlightRecorder::stop_recording);
    ClassDB::bind_method(D_METHOD("is_recording"), &HelgaFlightRecorder::is_recording);
    ClassDB::bind_method(D_METHOD("clear"), &HelgaFlightRecorder::clear);
    ClassDB::bind_method(D_METHOD("record_sample", "position", "rotation", "elevator", "aileron", "rudder", "throttle", "flight_state", "delta"), &HelgaFlightRecorder::record_sample);
    ClassDB::bind_method(D_METHOD("get_sample_count"), &HelgaFlightRecorder::get_sample_count);
    ClassDB::bind_method(D_METHOD("get_duration"), &HelgaFlightRecorder::get_duration);
    ClassDB::bind_method(D_METHOD("sample_at_time", "t"), &HelgaFlightRecorder::sample_at_time);
    ClassDB::bind_method(D_METHOD("save_to_file", "path"), &HelgaFlightRecorder::save_to_file);
    ClassDB::bind_method(D_METHOD("load_from_file", "path"), &HelgaFlightRecorder::load_from_file);
}
