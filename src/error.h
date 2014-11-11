#pragma once

#include <stdexcept>

#define STATUS_OK 0
#define STATUS_ERROR -1
#define STATUS_CHECK_ERRNO -2
#define STATUS_TAMPERING -3

class AllocationError: public std::runtime_error {
public:
    AllocationError(): std::runtime_error("Memory allocation error") {}
};
