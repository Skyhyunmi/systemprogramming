#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>

static struct sockaddr_in addr,addr2;
pthread_t snd_th,rcv_th;

static int sock, sock2;
int rec1, rec2, clnt_cnt=0, clnt_socks[5];
pthread_mutex_t mutx;

void error_handling(char *message);
static void * th_read (void *arg);
static void *handle_clnt(void *arg);

void Server(int port){
    int nSockOpt =1;
    sock=socket(PF_INET,SOCK_STREAM,0);
    sock2=socket(PF_INET,SOCK_STREAM,0);
    setsockopt(sock2,SOL_SOCKET,SO_REUSEADDR, &nSockOpt, sizeof(nSockOpt));
    memset(&addr, 0, sizeof(addr));
    socklen_t clnt_addr_size;
    printf("Server\n");
    pthread_mutex_init(&mutx, NULL);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(sock2,(struct sockaddr*) &addr, sizeof(addr)) == -1) error_handling("bind error");
    if(listen(sock2,5)==-1) error_handling("listen error");
    while(1){
        clnt_addr_size=sizeof(addr2);
        sock=accept(sock2, (struct sockaddr*)&addr2, &clnt_addr_size);
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++]=sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&snd_th, NULL, handle_clnt, (void*)&sock);
        pthread_detach(snd_th);
    }
}

void Client(char address[], int port){
    sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&addr, 0, sizeof(addr));
    socklen_t clnt_addr_size;
    printf("Client\n");
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=inet_addr(address);
    if(sock == -1) error_handling("socket error");
    if(connect(sock,(struct sockaddr *)&addr,sizeof(addr))==-1)
        error_handling("connect error");
    pthread_create(&rcv_th, NULL, th_read, (void *) &sock);
    pthread_detach(rcv_th);
}

static void my_write(char *message){
    write(sock,message, sizeof(message));
}

static void * th_read (void *arg){
    char message[30];
    int str_len;
    while(1){
        memset(message,0,sizeof(message));
        str_len=read(sock,message,sizeof(message)-1);
        if(str_len) {
            // printf("%s\n",message);
            if (message[0] =='1') rec1 = atoi(message);
            else rec2 = atoi(message);
        }
    }
}

void error_handling(char *message){
    perror(message);
    exit(1);
}

static void send_msg(int clnt_sock, char* msg, int len)
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i=0; i<clnt_cnt; i++){
        if(clnt_sock != clnt_socks[i])
            write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mutx);
}

static void *handle_clnt(void *arg)
{
    int clnt_sock=*((int*)arg);
    int temp = clnt_sock;
    int str_len=0, i;
    char msg[30];
    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0){
        send_msg(clnt_sock,msg, strlen(msg));
    }
 
    pthread_mutex_lock(&mutx);
    for (i=0; i<clnt_cnt; i++)
    {
        if (clnt_sock==clnt_socks[i])
        {
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

int data_from(int val){
    int from = val/10;
    if(val%10 == 1) return from;
    else return -1;
}