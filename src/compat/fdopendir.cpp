#include <fcntl.h>
#include <sys/param.h>
#include "compat.h"

DIR* fdopendir(int fd) {
    char path[MAXPATHLEN];
    DIR* dir = nullptr;

    if(fcntl(fd, F_GETPATH, path) < 0) {
        return nullptr;
    }

    dir = opendir(path);
    return dir;
}
