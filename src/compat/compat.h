#pragma once

#include <unistd.h>
#include <dirent.h>
#include <sys/sysctl.h>

/// Broken operating systems like OSX don't support this
#ifndef HAVE_FDOPENDIR
DIR* fdopendir(int fd);
#endif

inline size_t get_memory_size() {
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        if(pages < 0 || page_size < 0) { return 0; }
        return pages * page_size;
    }
#elif defined(CTL_HW)
    {
        int mib[2];
        mib[0] = CTL_HW;
        mib[1] = HW_PHYSMEM;
        size_t result;
        size_t len = sizeof(result);
        if(sysctl(mib, 2, &result, &len, NULL, 0) < 0) {
            return 0;
        }
        return result;
    }
#else
#error "Cannot determine amount of memory on this platform"
#endif
}
