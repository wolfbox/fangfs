#pragma once

#include <dirent.h>
#include <unistd.h>
#include <sys/sysctl.h>

/// Broken operating systems like OSX don't support this
#ifndef HAVE_FDOPENDIR
DIR* fdopendir(int fd);
#endif

inline size_t get_memory_size() {
#ifdef HAVE_SC_PHYS_PAGES
    {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        if(pages < 0 || page_size < 0) { return 0; }
        return pages * page_size;
    }
#elif defined (HAVE_HW_MEMSIZE)
    {
        int mib[2];
        mib[0] = CTL_HW;
        mib[1] = HW_MEMSIZE;
        size_t result;
        size_t len = sizeof(result);
        if(sysctl(mib, 2, &result, &len, nullptr, 0) < 0) {
            return 0;
        }
        return result;
    }
#else
#error "Cannot determine amount of memory on this platform"
#endif
}
