#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

int main(int argc, char *argv[]) {
    int fd;

    if(argc < 2)
        return 0;
    
    fd = open("/proc/MacroManager_proc", O_RDWR);
    lseek(fd, 0, SEEK_SET);
    write(fd, argv[1], strlen(argv[1]) + 1);
    return 0;
}