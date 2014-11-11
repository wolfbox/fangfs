#include <fcntl.h>
#include "compat.h"

DIR* fdopendir(int fd) {
    char path[MAXPATHLEN];
    DIR* dir = NULL;

    if(fcntl(fd, F_GETPATH, path) < 0) {
        return NULL;
    }

    dir = opendir(path);
    return dir;
}
