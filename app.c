#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include "socket.h"
#include "lcd.h"

#define PIR_MAJOR_NUMBER 505
#define PIR_MINOR_NUMBER 100
#define PIR_DEV_PATH_NAME "/dev/pir_ioctl"

#define IOCTL_MAGIC_NUMBER 'j'
#define IOCTL_CMD_GET_STATUS    _IOWR(IOCTL_MAGIC_NUMBER,0, int)
#define IOCTL_CMD_GET_STATUS2    _IOWR(IOCTL_MAGIC_NUMBER,1, int)

// SERVER 0 -> CLIENT
// SERVER 1 -> SERVER
#define SERVER 0

void sig_handler(int signo);

int keep=1;
extern int rec1,rec2,sock;

int main(int argc, char *argv[]){
    dev_t pir_dev;
    char message[128];
    int fd1,cnt,fire=0;
    long pir1, pir2;
    int rc = lcdInit(0x27);

    signal(SIGINT, (void *)sig_handler);
	if (rc){
		printf("Initialization failed; aborting...\n");
		return 0;
	}

    pir_dev = makedev(PIR_MAJOR_NUMBER, PIR_MINOR_NUMBER);
    mknod(PIR_DEV_PATH_NAME, S_IFCHR|0666, pir_dev);

    fd1 = open(PIR_DEV_PATH_NAME, O_RDWR);
    
    if(fd1<0){
        printf("fail to open pir\n");
        return -1;
    }

    Client(argv[1], atoi(argv[2])); // Client -> Need port and ip
    while(keep){
        sleep(1);
        cnt++;

        if(rec1==11) {
            fire=1;
            cnt=0;
            lcdSetCursor(1,4);  
            lcdWriteString("      Time : 0   ");
        }
        else if(cnt >3 && rec1==10 ) {
            fire=0;
        }
                // rec은 실제 센서에서 받아 오는 값(21 21 21 20), fire는 테스트용 임의의 센서 값 (11)

        
        sprintf(message,"  Signal from 1: %d  ",fire);
        lcdSetCursor(1,1);  
        lcdWriteString(message);

        pir1 = ioctl(fd1, IOCTL_CMD_GET_STATUS, NULL);
        pir2 = ioctl(fd1, IOCTL_CMD_GET_STATUS2, NULL);
        int res=20;
        if(fire){
            if(pir1 || pir2) res=21;
            else res=22;
        }
        else res=20;
        sprintf(message," PIR1 : %ld  PIR2 : %ld      ",pir1, pir2);

        lcdSetCursor(1,2);  
        lcdWriteString(message);


        sprintf(message,"     Signal : %d",res%10);
        lcdSetCursor(1,3);  
        lcdWriteString(message);
        sprintf(message,"%d",res);
        my_write(message);

        sprintf(message,"      Time : %d",cnt);
        lcdSetCursor(1,4);  
        lcdWriteString(message);
        
        // fflush(stdout);
    }
    lcdShutdown();
    close(sock);
    close(fd1);

    return 0;
}

void sig_handler(int signo)
{
  keep = 0;
}