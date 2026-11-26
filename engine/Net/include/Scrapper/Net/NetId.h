#pragma once

#include <cstdint>


namespace Scrapper {
// Replicated under this id. Host and clients must give the same object the same id; 0 is not an id.
struct NetId {
    std::uint32_t value = 0;
};

}
