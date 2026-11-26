#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <algorithm>


namespace Scrapper {
class IgnoreBodies final : public JPH::BodyFilter {
public:
    explicit IgnoreBodies(const JPH::Array<JPH::BodyID>& ids) : ids(ids) {}
    bool ShouldCollide(const JPH::BodyID& id) const override { return std::find(ids.begin(), ids.end(), id) == ids.end(); }

private:
    const JPH::Array<JPH::BodyID>& ids;
};

}
