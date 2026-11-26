#include "Scrapper/Jolt/JoltRuntime.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include <cstdarg>
#include <cstdio>


namespace Scrapper {
namespace {
int users = 0;

void TraceToStderr(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::fprintf(stderr, "[Jolt] ");
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");
    va_end(args);
}
}

JoltRuntime::JoltRuntime() {
    if (users++ > 0) return;
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceToStderr;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

JoltRuntime::~JoltRuntime() {
    if (--users > 0) return;
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

}
