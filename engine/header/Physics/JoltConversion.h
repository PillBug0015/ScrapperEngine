#pragma once

#include <Jolt/Jolt.h>
#include <raylib.h>


namespace ScrapperEngine {
inline JPH::Vec3 ToJolt(Vector3 v) { return JPH::Vec3(v.x, v.y, v.z); }
inline JPH::RVec3 ToJoltR(Vector3 v) { return JPH::RVec3(v.x, v.y, v.z); }
inline JPH::Quat ToJolt(Quaternion q) { return JPH::Quat(q.x, q.y, q.z, q.w); }

inline Vector3 ToRaylib(JPH::Vec3Arg v) { return { v.GetX(), v.GetY(), v.GetZ() }; }
inline Quaternion ToRaylib(JPH::QuatArg q) { return { q.GetX(), q.GetY(), q.GetZ(), q.GetW() }; }

}
