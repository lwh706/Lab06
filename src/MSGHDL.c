#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

void signal_handler(int);
//為了讓signal handler收到而設定為全域變數
int step;
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
    
//建立送出的Message Queue
    key_t key;
    key = (key_t) 0x66660706;
    ms_snd_qid = msgget(key, 0666 | IPC_CREAT);
    step = 1;
//送出訊息MQ
    //要傳送的訊息內容
    char buf[512];
    char tmp[512];
    unsigned char hex1;
    unsigned char hex2;
    unsigned char msg_txt[512];
    strcpy(buf, "ISO00600006002003238C48128A1800A0000000000000077000601012345000706082345060406044871010590388820374245001751006=4912000000860706A1B2C3D4E5F6G7H8ABCDEFGHIJKLMNOPQRSTUVNEWTAIPEICITYTW TW004WXYZ9010190706PRO200000000000046& 0000200046! C000024 1881              0  1 1");
    hex2 = 0x00 + strlen(buf)%256;
    hex1 = 0x00 + (strlen(buf)-hex2)/256;
    //訊息處理
    msg_txt[0] = hex1;
    msg_txt[1] = hex2;
    strcat((msg_txt+2), buf);
    //msgsnd()送出訊息
    if(msgsnd(ms_snd_qid, &msg_txt, sizeof(msg_txt), 0) < 0){
        printf("Send message queue failed\n");
        msgctl(ms_snd_qid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    //顯示送出畫面
    printf("Send Message queue : %s;\n", buf);
    printf("__________________________________________________\n");
    //清除MQ
    msgctl(ms_snd_qid, IPC_RMID, NULL);
//建立接收的Message Queue
    key = (key_t) 0x77770706;
    ms_rcv_qid = msgget(key, 0666 | IPC_CREAT);
    step = 2;
//接收MQ
    //msgrcv()接收訊息
    printf("Waiting message...\n");
    memset(buf, 0, sizeof(buf));
    if(msgrcv(ms_rcv_qid, &buf, sizeof(buf), 0, 0) < 0){
        printf("Recieve message queue failed");
        exit(EXIT_FAILURE);
    }
    //顯示接收畫面
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, &buf[2], sizeof(buf)-2);
    printf("Recieve Message Queue : %s;\n", tmp);
//處理訊息
    int position = 2;
    //Header
    char header[13];
    memset(header, 0, sizeof(header));
    memcpy(header, &buf[position], 12);
    printf("Header : %s;\n", header);
    position += 12;//改變讀取起始位置
    //MTI
    char mti[5];
    memset(mti, 0, sizeof(mti));
    memcpy(mti, &buf[position], 4);
    printf("MTI    : %s;\n", mti);
    position += 4;//改變讀取起始位置
    //轉換bitmap
    char bitmap_16[20];
    char bitmap_2[257];
    char O_I[5];
    char O_I_str[5];
    unsigned char bin;
    //16進位轉成2進位
    memset(bitmap_2, 0, sizeof(bitmap_2));
    memcpy(bitmap_16, &buf[18], 16);
    int i;
    for(i=0; i<16; i++){
        if(bitmap_16[i]<58){
            bin = bitmap_16[i] - 48;
        }
        else{            
            bin = bitmap_16[i] - 55;
        }
        O_I[0] = (bin - bin%8)/8;
        O_I[1] = (bin - O_I[0]*8 - bin%4)/4;
        O_I[2] = (bin - O_I[0]*8 - O_I[1]*4 - bin%2)/2;
        O_I[3] = (bin - O_I[0]*8 - O_I[1]*4 - O_I[2]*2);
        sprintf(O_I_str, "%d%d%d%d", O_I[0], O_I[1], O_I[2], O_I[3]);
        strcat(bitmap_2, O_I_str);
    }
    printf("BitMap : %s;\n", bitmap_2);
    //bitmap為1的欄位填入p_name
    int p_name[40];
    int l = 0;
    int m = 1;
    for(i=0; i<64; i++){
        if(bitmap_2[i]==48){
            m += 1;
            continue;
        }
        else if(bitmap_2[i]==49){
            p_name[l] = m;
            l += 1;
            m += 1;
        }
        else{
            printf("position %d is NOT 0 or 1!\n", i);
            exit(EXIT_FAILURE);
        }
    }
    position += 16;//改變讀取起始位置
    //處理各欄位
    // int p_name[16] = {3,4,7,11,12,13,15,17,32,35,37,38,39,41,49,63}; 改由上面處理
    int p_len[16] = {6,12,10,6,6,4,4,4,0,0,12,6,2,16,3,0};
    int szofp;
    char flex_char[5];
    szofp = sizeof(p_len)/sizeof(p_len[0]);
    for(i=0; i<szofp; i++){
        //針對變動長欄位進行處裡
        if(p_name[i]==32||p_name[i]==35){   
            memset(flex_char, 0, sizeof(flex_char));         
            memcpy(flex_char, &buf[position], 2);
            p_len[i] = atoi(flex_char) + 2;
        }
        if(p_name[i]==63){
            memset(flex_char, 0, sizeof(flex_char));  
            memcpy(flex_char, &buf[position], 3);
            p_len[i] = atoi(flex_char) + 3;
        }
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, &buf[position], p_len[i]);
        //輸出欄位內容
        if(p_name[i]<10){
            printf(" P-%d   : %s;\n", p_name[i], tmp);
        }
        else{
            printf(" P-%d  : %s;\n", p_name[i], tmp);
        }
        position += p_len[i];//改變讀取起始位置
    }
    printf("__________________________________________________\n");
    //清除MQ
    msgctl(ms_rcv_qid, IPC_RMID, NULL);

    return 0;
}
//如果強制中斷馬上清除MQ
void signal_handler(int singo){
    printf("\nSIGNAL Caught!\n");
    //清除MQ
    if(step == 1){
        msgctl(ms_snd_qid, IPC_RMID, NULL);
    }
    else{ 
        msgctl(ms_rcv_qid, IPC_RMID, NULL);
    }
    exit(0);
}