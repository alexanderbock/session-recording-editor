#pragma once

#include <filesystem>
#include <variant>
#include <vector>

struct Keyframe {
#ifdef _DEBUG
    virtual ~Keyframe() = default;
#endif

    enum class Type { Camera, Script };
    Type type;

    double startupTime;
    double recordingTime;
    double ingameTime;
};

struct KeyframeCamera : public Keyframe {
    KeyframeCamera() { type = Type::Camera; }

    double posX;
    double posY;
    double posZ;
    double orientationW;
    double orientationX;
    double orientationY;
    double orientationZ;
    double scale;
    bool shouldFollow;
    std::string followNode;
};

struct KeyframeScript : public Keyframe {
    KeyframeScript() { type = Type::Script; }
    std::string script;
};

struct ScaleInfo {
    double x;
    double y;
    KeyframeCamera* kf;
};

struct SessionRecording {
    std::vector<Keyframe*> keyframes;

    double recordingLength = 0.0;
    std::pair<double, double> minMaxScale;

    std::vector<ScaleInfo> originalNormalizedScale;
    std::vector<ScaleInfo> normalizedLinearizedScale;
};

SessionRecording* loadSessionRecording(std::filesystem::path path);
void saveSessionRecording(SessionRecording* session, std::filesystem::path path);
