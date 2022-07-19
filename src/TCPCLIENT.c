#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define PORT 7777
#define setIP "0.0.0.0"

void signal_handler(int);
//為了讓signal handler收到而設定為全域變數
int step;
int sockfd;
int ms_rcv_qid;
int ms_snd_qid;

int main(){
    //sigaction
    struct sigaction act;
    act.sa_handler=signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGTSTP,&act,NULL);
    sigaction(SIGQUIT,&act,NULL);

//建立TCP
    //創建套接字
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket error");
        return -1;
    }
    printf("socket successed....\n");
    //TCP連線
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(PORT);
    cli_addr.sin_addr.s_addr = inet_addr(setIP);
    //等候連線，最多等候15秒
    int cnctfd;
    int i;
    for(i=0; i<15; i++){
        printf("Wait %d sec...\n", i+1);
        sleep(1);
        cnctfd = connect(sockfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr));
        if(cnctfd == 0){
            break;
        }
    }
    if(cnctfd < 0){
        perror("connect error");
        return -1;
    }
    printf("connect successed...\n");
//建立接收的Message Queue
    key_t key;
    key = (key_t) 0x66660706;
    ms_rcv_qid = msgget(key, 0666 | IPC_CREAT);
    step = 1;
//接收MQ
    int snd_ck_int;
    int rcv_ck_int;
    char buf[512];
    char tmp[512];
    char ck_ch[5];
    unsigned char hex1;
    unsigned char hex2;
    unsigned char msg_txt[512];
    //msgrcv()接收訊息
    printf("Waiting message...\n");
    memset(buf, 0, sizeof(buf));
    if(msgrcv(ms_rcv_qid, &buf, sizeof(buf), 0, 0) < 0){
        printf("Recieve message queue failed");
        msgctl(ms_rcv_qid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    //顯示接收畫面
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, &buf[2], sizeof(buf)-2);
    printf("Recieve Message Queue : %s;\n", tmp);
    hex1 = buf[0];
    hex2 = buf[1];
    //送出MTI
    memcpy(ck_ch, &buf[14], 4);
    snd_ck_int = atoi(ck_ch);
    printf("__________________________________________________\n");
    //清除MQ
    msgctl(ms_rcv_qid, IPC_RMID, NULL);
    //處理訊息
    msg_txt[0] = hex1;
    msg_txt[1] = hex2;
    strcat((msg_txt+2), tmp);
//傳送TCP
    int cli_snd;
    cli_snd = send(sockfd, &msg_txt, sizeof(msg_txt), 0);
    if(cli_snd < 0){
        perror("send error");
        exit(EXIT_FAILURE);
    }
    printf("send : %s;\n", tmp);
    printf("__________________________________________________\n");
//接收TCP
    //讀取消息隊列中的訊息
    memset(buf, 0, sizeof(buf));
    int cli_read = read(sockfd, &buf, sizeof(buf));
    if(cli_read < 0){
        perror("read error");
        return -1;
    }
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, &buf[2], sizeof(buf)-2);
    printf("rcv : %s;\n", tmp);
    hex1 = buf[0];
    hex2 = buf[1];
    //判斷回傳MTI是否為送出MTI+10
    memcpy(ck_ch, &buf[14], 4);
    rcv_ck_int = atoi(ck_ch);
    if(rcv_ck_int - snd_ck_int != 10){
        printf("***************\n");
        printf("*check fail...*\n");
        printf("***************\n");
    }
    else{        
        printf("***********\n");
        printf("*check ok!*\n");
        printf("***********\n");
    }
    printf("__________________________________________________\n");
    //關閉套接字
    close(sockfd);
    //處理訊息
    memset(msg_txt, 0, sizeof(msg_txt));
    msg_txt[0] = hex1;
    msg_txt[1] = hex2;
    strcat((msg_txt+2), tmp);
//建立回覆的Message Queue
    key = (key_t) 0x77770706;
    ms_snd_qid = msgget(key, 0666 | IPC_CREAT);
    step = 2;
//回覆MQ
    //msgsnd()送出訊息
    if(msgsnd(ms_snd_qid, &msg_txt, sizeof(msg_txt), 0) < 0){
        printf("Send message queue failed\n");
        msgctl(ms_snd_qid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    //顯示送出畫面
    printf("Send Message queue : %s;\n", tmp);
    printf("__________________________________________________\n");

    //清除MQ
    msgctl(ms_snd_qid, IPC_RMID, NULL);

    return 0;
}
//如果強制中斷馬上清除MQ
void signal_handler(int singo){
    printf("\nSIGNAL Caught!\n");
    //清除MQ
    if(step == 1){
        msgctl(ms_rcv_qid, IPC_RMID, NULL);
        close(sockfd);
    }
    else{
        msgctl(ms_snd_qid, IPC_RMID, NULL);
    }
    exit(0);
}