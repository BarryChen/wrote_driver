

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>

#if 0//def  __ANDROID__
#include <android/log.h>
#define LOG_TAG "MC-TEST"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#include <stdio.h>
#define LOGI(...)  printf(__VA_ARGS__)
#define LOGE(...)  printf(__VA_ARGS__)
#endif

static int port = -1;


static void * reader(void * data) {
    int ret;
    char buf[200];
    for (;;) {
        memset(buf, 0, sizeof(buf));
        ret = read(port, buf, sizeof(buf));
        if (ret == 0) {
            close(port);
            LOGI(" connection close\n");
            return NULL;
        } else if (ret == -1) {
            LOGE(" read failed err:%d\n", errno);
            sleep(1);
        } else {
            LOGI(" read response (%s)\n", buf);
        }
    }

    return NULL;
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0) {
        perror("SetupSerial 1");
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag  |=  CLOCAL | CREAD; //CLOCAL:忽略modem控制线  CREAD：打开接受者
    newtio.c_cflag &= ~CSIZE; //字符长度掩码。取值为：CS5，CS6，CS7或CS8

    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
    case 'O':
        newtio.c_cflag |= PARENB; //允许输出产生奇偶信息以及输入到奇偶校验
        newtio.c_cflag |= PARODD;  //输入和输出是奇及校验
        newtio.c_iflag |= (INPCK | ISTRIP); // INPACK:启用输入奇偶检测；ISTRIP：去掉第八位
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }

    switch( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 19200:
        cfsetispeed(&newtio, B19200);
        cfsetospeed(&newtio, B19200);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }

    if( nStop == 1 )
        newtio.c_cflag &=  ~CSTOPB; //CSTOPB:设置两个停止位，而不是一个
    else if ( nStop == 2 )
        newtio.c_cflag |=  CSTOPB;

    //newtio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);///////////

    newtio.c_cc[VTIME]  = 0; //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
    newtio.c_cc[VMIN] = 0; //VMIN:非canonical模式读到最小字符数
    tcflush(fd,TCIFLUSH); // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
    if((tcsetattr(fd,TCSANOW,&newtio))!=0) //TCSANOW:改变立即发生
    {
        perror("com set error");
        return -1;
    }
    printf("set done!\n\r");
    return 0;
}

int set_opt_ot(int fd)
{
    struct termios  ios;
    tcgetattr( fd, &ios );
    ios.c_cflag = CS8|CREAD|CLOCAL|CRTSCTS; //| CS8|CREAD;//|CLOCAL;
    cfsetispeed(&ios, B4000000);                                                                                                                                                    
    cfsetospeed(&ios, B4000000);
    
    ios.c_iflag = 0;  /* do not change '\n' to '\r' */
    //ios.c_iflag &= ~INLCR;  /* do not change '\n' to '\r' */
    //ios.c_iflag &= ~ICRNL;  /* do not change '\r' to '\n' */
    //ios.c_iflag |= IXON;   /* disable start/stop output control */
    //ios.c_iflag |= IXOFF;   /* disable start/stop output control */
    ios.c_lflag = 0;        /* disable ECHO, ICANON, etc... */
    ios.c_cc[VMIN]=1;
    ios.c_cc[VTIME]=5;
    
    tcsetattr(fd, TCSANOW, &ios );
    tcsetattr(fd,TCSAFLUSH,&ios);
    return 0;

}


int main(int argc, char ** argv) {
    struct termios attr;
    pthread_t tid;
    char   buf[100];
    size_t len = 100;
    char * line = buf;
    int ret;


    if (argc < 2) {
        LOGE("[Uage] no such port in command line\n");
        return 0;
    }

    port = open(*++argv,  O_RDWR);
    if (port == -1) {
        LOGE(" can not open port(%s) err:%d\n", *argv, errno);
        return -1;
    }

    ret = set_opt_ot(port);
    if(ret < 0) {
      LOGE("set opt error");
      return -1;
    }



    pthread_create(&tid, NULL, reader, NULL);
    for(;;) {
        memset(line, 0, sizeof(buf));
        getline(&line, &len, stdin);
        if (*line == '\n') {
            return 0;
        }
        LOGI("send >> %s\n", line);
        write(port, line, strlen(line));
    }

    return 0;
}

