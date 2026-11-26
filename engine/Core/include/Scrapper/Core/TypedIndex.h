#pragma once

#include <cstdint>


namespace Scrapper {
// An index into one particular array. Every Tag makes its own type, so an index into one array
// can't be passed where another's is expected.
template <typename Tag>
struct TypedIndex {
    std::uint16_t value = 0;

    friend constexpr bool operator==(TypedIndex a, TypedIndex b) { return a.value == b.value; }
    friend constexpr bool operator!=(TypedIndex a, TypedIndex b) { return a.value != b.value; }
};

}
