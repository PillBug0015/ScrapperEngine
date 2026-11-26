#include "Scrapper/Jolt/Ragdoll.h"

#include <cmath>


namespace Scrapper {
Quaternion RagdollAsset::Clip::Sample(PartId part, float time) const {
    const std::vector<RagdollClip::Key>& keys = keysByPart[part.value];
    if (keys.empty()) return QuaternionIdentity();

    time = loop && duration > 0.0f ? std::fmod(time, duration) : Clamp(time, 0.0f, duration);
    Vector3 degrees = keys.back().degrees;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (time > keys[i].time) continue;
        if (i == 0) {
            degrees = keys[0].degrees;
            break;
        }
        const RagdollClip::Key& from = keys[i - 1];
        const RagdollClip::Key& to = keys[i];
        const float span = to.time - from.time;
        const float t = span > 0.0f ? (time - from.time) / span : 1.0f;
        degrees = Vector3Lerp(from.degrees, to.degrees, t * t * (3.0f - 2.0f * t)); // eased in and out
        break;
    }
    return QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
}

}
