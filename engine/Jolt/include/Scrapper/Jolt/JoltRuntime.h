#pragma once


namespace Scrapper {
// Jolt's allocator, factory and type registry are process-wide. Whatever holds Jolt objects
// (a JoltWorld, a loaded asset) holds one of these, so the registry outlives them all.
class JoltRuntime {
public:
    JoltRuntime();
    JoltRuntime(const JoltRuntime&) : JoltRuntime() {}
    JoltRuntime& operator=(const JoltRuntime&) { return *this; }
    ~JoltRuntime();
};

}
