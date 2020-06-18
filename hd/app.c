#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define PIR_MAJOR_NUMBER 505
#define PIR_MINOR_NUMBER 100
#define PIR_DEV_PATH_NAME "/dev/pir_ioctl"

#define IOCTL_MAGIC_NUMBER 'j'
#define IOCTL_CMD_GET_STATUS _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
// #define IOCTL_CMD_BLINK _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

// static volatile pthread_t threads [5] ;
#define PORT 55000
#define ADDR "192.168.219.110"

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(0);
}
static int rec;
static void * soc (void *arg){
    int serv_sock,str_len,sock;
    char message[30];
    socklen_t clnt_addr_size;
    struct sockaddr_in serv_addr = *((struct sockaddr_in *)arg);
    sock=socket(PF_INET, SOCK_STREAM,0);
    if(sock == -1) error_handling("socket error");
    if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
        error_handling("connect error");
    // printf("soc\n");
    while(1){
        str_len=read(sock,message,sizeof(message)-1);
        if(str_len) rec = atoi(message);
        else rec = 0;
        // if(rec == 1) sleep(10);
    }
    close(sock);
}

int keep=1;
void intHandler(int sig){keep=0;}
struct sockaddr_in *serv_addr;
int main(void){
    dev_t pir_dev;
    int fd1,res;
    pthread_t th;
    char buffer[4];
    struct sigaction act;
    act.sa_handler = intHandler;
    sigaction(SIGINT, &act, NULL);
    pir_dev = makedev(PIR_MAJOR_NUMBER, PIR_MINOR_NUMBER);
    mknod(PIR_DEV_PATH_NAME, S_IFCHR|0666, pir_dev);

    fd1 = open(PIR_DEV_PATH_NAME, O_RDWR);

    if(fd1<0){
        printf("fail to open pir\n");
        return -1;
    }
    serv_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    // memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr->sin_family=AF_INET;
    serv_addr->sin_addr.s_addr=inet_addr(ADDR);
    serv_addr->sin_port=htons(PORT);
    pthread_create(&th, NULL, soc, (void *) serv_addr);
    while(keep){
        sleep(1);
        if(rec){
            ioctl(fd1, IOCTL_CMD_GET_STATUS, buffer);
            printf("%s\n",buffer);
        }
        
    }
    int state=0;
    // ioctl(fd1, IOCTL_CMD, &state);
    close(fd1);
    // close(fd2);

    return 0;
}