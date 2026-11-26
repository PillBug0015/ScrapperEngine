#pragma once

#include "Scrapper/Assets/Model.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <memory>
#include <string>


namespace Scrapper {
// The shape's surface as a flat-shaded white mesh in body space, so a body can be drawn as it collides.
std::shared_ptr<const ModelAsset> MakeShapeModel(const JPH::Shape& shape, const std::string& name);

}
