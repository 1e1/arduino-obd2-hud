#pragma once
// Minimal host shim so dependency-light firmware headers (e.g. _can.h) compile
// natively for unit tests, without the real Arduino core.
#include <cstdint>
#include <cstring>
