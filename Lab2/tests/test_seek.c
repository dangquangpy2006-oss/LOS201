#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(void)
{
    char buf[3] = {0};
    int fd = open("/dev/lab2", O_RDWR);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (write(fd, "ABCDEFGH", 8) != 8) {
        perror("write");
        close(fd);
        return 1;
    }

    if (lseek(fd, 3, SEEK_SET) != 3 ||
        read(fd, buf, 2) != 2 ||
        strcmp(buf, "DE") != 0) {
        puts("[FAIL] Valid seek");
        close(fd);
        return 1;
    }
    puts("[PASS] Valid seek");

    errno = 0;
    if (lseek(fd, 20, SEEK_SET) != -1 ||
        errno != EINVAL ||
        lseek(fd, 0, SEEK_CUR) != 5) {
        puts("[FAIL] Invalid seek");
        close(fd);
        return 1;
    }
    puts("[PASS] Invalid seek preserves offset");

    if (lseek(fd, -1, SEEK_END) != 7) {
        puts("[FAIL] SEEK_END");
        close(fd);
        return 1;
    }
    puts("[PASS] SEEK_END");

    close(fd);
    return 0;
}
