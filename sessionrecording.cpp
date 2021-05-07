#include "sessionrecording.h"

#include <QMessageBox>
#include <fstream>
#include <string>
#include <vector>

namespace {
    std::vector<std::string> tokenizeString(const std::string& input, char separator) {
        size_t separatorPos = input.find(separator);
        if (separatorPos == std::string::npos) {
            return { input };
        }
        else {
            std::vector<std::string> result;
            size_t prevSeparator = 0;
            while (separatorPos != std::string::npos) {
                result.push_back(input.substr(prevSeparator, separatorPos - prevSeparator));
                prevSeparator = separatorPos + 1;
                separatorPos = input.find(separator, separatorPos + 1);
            }
            result.push_back(input.substr(prevSeparator));
            return result;
        }
    }

    void removeEmpty(std::vector<std::string>* list) {
        for (size_t i = 0; i < list->size(); i += 1) {
            if (list->at(i).empty()) {
                list->erase(list->begin() + i);
                i -= 1;
            }
        }
    }
} // namespace

SessionRecording* loadSessionRecording(std::filesystem::path path) {
    SessionRecording* res = new SessionRecording;
    res->minMaxScale = std::pair(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());

    std::ifstream f(path);
    std::string line;
    std::getline(f, line);
    if (line != "OpenSpace_record/playback01.00A") {
        QMessageBox::critical(nullptr, "Error loading session recording",
            "Could not load session recording. "
            "Header is not 'OpenSpace_record/playback01.00A'"
        );
        delete res;
        return nullptr;
    }

    int iLine = 1;
    while (std::getline(f, line)) {
        std::vector<std::string> parts = tokenizeString(line, ' ');
        removeEmpty(&parts);

        std::string type = parts[0];
        if (type != "script" && type != "camera") {
            QMessageBox::critical(nullptr, "Error loading session recording",
                QString::fromStdString("Could not load session recording. "
                "Unknown keyframe type '" + type + "' in line " + std::to_string(iLine))
            );
            delete res;
            return nullptr;
        }


        if (type == "script") {
            KeyframeScript* kf = new KeyframeScript;

            kf->startupTime = std::atof(parts[1].c_str());
            kf->recordingTime = std::atof(parts[2].c_str());
            kf->ingameTime = std::atof(parts[3].c_str());
            if (parts[4] != "1") {
                QMessageBox::critical(nullptr, "Error loading session recording",
                    QString::fromStdString("Could not load session recording. "
                        "Can only understand script keyframes with 1 script, got '" + parts[4] + "' in line" + std::to_string(iLine))
                );
                delete res;
                return nullptr;
            }
            kf->script = std::accumulate(parts.begin() + 5, parts.end(), std::string());
            res->keyframes.push_back(kf);
        }
        else {
            assert(type == "camera");
            KeyframeCamera* kf = new KeyframeCamera;

            kf->startupTime = std::atof(parts[1].c_str());
            kf->recordingTime = std::atof(parts[2].c_str());
            kf->ingameTime = std::atof(parts[3].c_str());
            kf->posX = std::atof(parts[4].c_str());
            kf->posY = std::atof(parts[5].c_str());
            kf->posZ = std::atof(parts[6].c_str());
            kf->orientationW = std::atof(parts[7].c_str());
            kf->orientationX = std::atof(parts[8].c_str());
            kf->orientationY = std::atof(parts[9].c_str());
            kf->orientationZ = std::atof(parts[10].c_str());
            kf->scale = std::atof(parts[11].c_str());
            kf->shouldFollow = parts[12] == "F";
            kf->followNode = parts[13];
            res->keyframes.push_back(kf);

            if (kf->scale < res->minMaxScale.first)  res->minMaxScale.first = kf->scale;
            if (kf->scale > res->minMaxScale.second)  res->minMaxScale.second = kf->scale;
        }
    }

    // Recording length
    res->recordingLength = res->keyframes.back()->recordingTime;

    // create scale normalization
    for (Keyframe* k : res->keyframes) {
        if (k->type != Keyframe::Type::Camera)  continue;

        KeyframeCamera* kf = static_cast<KeyframeCamera*>(k);
        double x = kf->recordingTime / res->recordingLength;
        double y = (kf->scale - res->minMaxScale.first) / (res->minMaxScale.second - res->minMaxScale.first);

        ScaleInfo info;
        info.x = x;
        info.y = y;
        info.kf = kf;
        res->normalizedLinearizedScale.push_back(info);
    }
    res->originalNormalizedLinearizedScale = res->normalizedLinearizedScale;

    // remove keyframes that are represented by linear interpolation
    for (size_t i = 1; i < res->normalizedLinearizedScale.size() - 1; i += 1) {
        const ScaleInfo& before = res->normalizedLinearizedScale[i - 1];
        const ScaleInfo& current = res->normalizedLinearizedScale[i];
        const ScaleInfo& after = res->normalizedLinearizedScale[i + 1];

        double t = (current.x - before.x) / (after.x - before.x);
        double v = before.y + t * (after.y - before.y);

        constexpr const double Epsilon = 1e-4;
        if (abs(v - current.y) <= Epsilon || (after.y == before.y)) {
            res->normalizedLinearizedScale.erase(res->normalizedLinearizedScale.begin() + i);
            i -= 1;
        }
    }

    if (res->normalizedLinearizedScale.size() < 2) {
        QMessageBox::critical(nullptr, "Error loading session recording",
            QString::fromStdString("Could not load session recording. "
                "After normalization, less than two scale values are left")
        );
        delete res;
        return nullptr;
    }

    return res;
}

void saveSessionRecording(SessionRecording* session, std::filesystem::path path) {
    std::ofstream f(path);

    f << "OpenSpace_record/playback01.00A\n";
    for (Keyframe* v : session->keyframes) {
        if (v->type == Keyframe::Type::Camera) {
            KeyframeCamera* kf = static_cast<KeyframeCamera*>(v);

            f << "camera ";
            f << std::to_string(kf->startupTime) << ' ';
            f << std::to_string(kf->recordingTime) << ' ';
            f << std::to_string(kf->ingameTime) << ' ';
            f << std::to_string(kf->posX) << ' ';
            f << std::to_string(kf->posY) << ' ';
            f << std::to_string(kf->posZ) << ' ';
            f << std::to_string(kf->orientationW) << ' ';
            f << std::to_string(kf->orientationX) << ' ';
            f << std::to_string(kf->orientationY) << ' ';
            f << std::to_string(kf->orientationZ) << ' ';
            f << std::to_string(kf->scale) << ' ';
            f << (kf->shouldFollow ? "F" : "-") << ' ';
            f << kf->followNode << '\n';
        }
        else {
            KeyframeScript* kf = static_cast<KeyframeScript*>(v);
            f << "script ";
            f << std::to_string(kf->startupTime) << ' ';
            f << std::to_string(kf->recordingTime) << ' ';
            f << std::to_string(kf->ingameTime) << ' ';
            f << "1 ";
            f << kf->script << '\n';
        }
    }
}
