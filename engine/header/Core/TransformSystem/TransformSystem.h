#pragma once

#include <cstdint>
#include <vector>

#include <raylib.h>

namespace ScrapperEngine
{
    static constexpr uint32_t INVALID_TRANSFORM_INDEX = UINT32_MAX;
    static constexpr uint32_t ROOT_NODE_INDEX = 0;
    static constexpr uint32_t MAX_TRANSFORM_DEPTH = 32;

    enum class OrphanPolicy
    {
        DetachChildren,
        DestroyChildren,
        ReparentToGrandparent
    };


    
    #pragma region Handles
    struct Transform2DHandle
    {

    };
    #pragma endregion




    static constexpr size_t CHUNK_SIZE = 256;
    struct Transform2DChunk
    {
        //default is public.
        float PosX[CHUNK_SIZE];
        float PosY[CHUNK_SIZE];

        uint32_t Depth[CHUNK_SIZE];
        uint32_t ParentIndex[CHUNK_SIZE];

        size_t Count = 0;
        bool IsFull() const { return Count >= CHUNK_SIZE; }
    };
    class TransformSystem
    {
    public:
        TransformSystem() = default;
        ~TransformSystem() = default;

    private:
    };
}
