#pragma once

#include "Network/NetTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>


namespace ScrapperEngine {
// Raw byte serialization of trivially copyable values. Every target (x64, arm64) is little-endian.
class ByteWriter {
public:
    template <typename T>
    ByteWriter& Write(const T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "ByteWriter only writes trivially copyable types");
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
        return *this;
    }

    const std::uint8_t* Data() const { return buffer.data(); }
    std::size_t Size() const { return buffer.size(); }

private:
    std::vector<std::uint8_t> buffer;
};

class ByteReader {
public:
    ByteReader(const std::uint8_t* data, std::size_t size) : data(data), size(size) {}

    // False (and `out` untouched) when the message is shorter than expected.
    template <typename T>
    bool Read(T& out) {
        static_assert(std::is_trivially_copyable<T>::value, "ByteReader only reads trivially copyable types");
        if (size - offset < sizeof(T)) {
            return false;
        }
        std::memcpy(&out, data + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }

    std::size_t Remaining() const { return size - offset; }

private:
    const std::uint8_t* data;
    std::size_t size;
    std::size_t offset = 0;
};

// Wire format of every message: [MessageType][payload].
inline std::vector<std::uint8_t> FrameMessage(MessageType type, const ByteWriter& payload) {
    std::vector<std::uint8_t> bytes(sizeof(MessageType) + payload.Size());
    std::memcpy(bytes.data(), &type, sizeof(MessageType));
    if (payload.Size() > 0) {
        std::memcpy(bytes.data() + sizeof(MessageType), payload.Data(), payload.Size());
    }
    return bytes;
}

}
