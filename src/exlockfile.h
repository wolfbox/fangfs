#pragma once

#include <stdbool.h>

/// Try to obtain an exclusive lock via the filesystem immediately, or else
/// return. Errors are returned via errno.
bool exlock_try_obtain(const char* path);

/// Obtain an exclusive lock via the filesystem, and blocks until it can be
/// obtained. Returns 0 if successful.
int exlock_obtain(const char* path);

/// Release an exclusive lock via the filesystem. Returns 0 if successful.
int exlock_release(const char* path);
