#include <errno.h>

/* VEX's newlib has no hard-link or POSIX unlink support. */
int _link(char* old_path, char* new_path) {
    (void)old_path;
    (void)new_path;
    errno = EMLINK;
    return -1;
}

int _unlink(char* path) {
    (void)path;
    errno = ENOENT;
    return -1;
}
