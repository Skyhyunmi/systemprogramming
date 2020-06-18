#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "socket.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>


#define CLIENT 0
#define SERVER 1
#define RW 2


int main(int argc, char *argv[]){

    int fd1,res;
    char buffer[4],buffer2[4];

    Start(atoi(argv[2]),argv[1],SERVER);

    close(sock);

    return 0;
}