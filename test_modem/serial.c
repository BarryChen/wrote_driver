#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int s_max_fd;
#define MAX_READ_BUF 20
static char save_buf[100];
static int recv_count = 0;

char handshake[6]= "\r\nOK\r\n";
#define IS_DEBUG 1
#define DEBUG()     \
do{                 \
    if(IS_DEBUG)    \
        printf("%s:line:%d\n", __FUNCTION__, __LINE__);  \
}while(0)             

typedef enum {
    CMD_OK = 0,
    CMD_RING,
    CMD_DTMF,
    CMD_DIAL,
    CMD_HOOK,
    CMD_NUM,   
} cmd_id; 

typedef struct { 
   int id;
   char *cmd;
}SLIC_CMD;

SLIC_CMD send_cmd[8] = {
    {CMD_OK,    "\r\nOK\r\n"            },
    {CMD_RING,  "AT+RING=%d,%d,%d\r\n"  },
    {CMD_DTMF,  "AT+DTMF=%d,%d,%s\r\n"  },
    {CMD_DIAL,  "AT+DIAL=%d\r\n"        },
    {CMD_HOOK,  "AT+HOOK?\r\n"          }

}; 

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


void get_one_byte_from_uart(void)
{
    int ret = 0;
    struct timeval timeout;
    fd_set read_fds;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&read_fds);
    FD_SET(s_max_fd, &read_fds);
    ret = select(s_max_fd+1, &read_fds, NULL, NULL, NULL);//&timeout);
    //usleep(700);
    if (ret > 0 )
    {
        if(FD_ISSET(s_max_fd, &read_fds))
        {
            int read_num;
            char readBuf[MAX_READ_BUF];

            read_num = read(s_max_fd, readBuf, 1);
            //readBuf[read_num] = '\0';
            // printf("readed data :count:%d\n",read_num);
            if(read_num > 0 && readBuf[0] != 0 && recv_count < 100)
            {
                save_buf[recv_count++] = readBuf[0];
                printf("%s", readBuf);
            }             
        }
    }
    else 
    {
        printf("Select error or timeout, ret_val = %d", ret);
    }
}

static void clean_buf(void)
{
    memset(save_buf, 0, MAX_READ_BUF);
    recv_count = 0;
}

static int writeline(const char *s)
{
    // int ret;
    
    // ret = write(s_max_fd, handshake, sizeof(handshake));
    // if( ret > 0) {
    //     printf("write handshake success!\n");
    // }


    size_t cur = 0;
    size_t len = strlen(s);
    ssize_t written;

    if (s_max_fd < 0) {
        return -1;
    }

    /* the main string */
    while (cur < len) {
        do {
            written = write (s_max_fd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);
        if (written < 0) {
            return -1;
        }

        cur += written;
    }

    /* the \r  */

    do {
        written = write (s_max_fd, "\r\n" , 2);
    } while ((written < 0 && errno == EINTR) || (written == 0));

    if (written < 0) {
        return -1;
    }

    return 0;
}


static int at_send_cmd(const char *command)
{
    int err;
    err = writeline(command);
    if (err < 0) {
        return -1;
    }
    return 0;
}
static void read_buffer_handler(void)
{
    char enable_use_buf[MAX_READ_BUF];
    if (recv_count >= 2)
    {
        while(save_buf[recv_count-1] != '\r')
            get_one_byte_from_uart();
        save_buf[recv_count] = '\0';
        memcpy(enable_use_buf, save_buf, recv_count);

        printf("count:%d,string:%s\n", recv_count, enable_use_buf); 
        if(strstr(enable_use_buf,"ATE0"))
        {
            DEBUG();
            at_send_cmd("OK");
        }      
        clean_buf();
    }
}

int serial_init(void)
{
    int nset,ret = 0;    

    s_max_fd = open( "/dev/ttyUSB0", O_RDWR);
    if (s_max_fd == -1)
        ret = -1;

    printf("open   success!!\n");

    nset = set_opt(s_max_fd, 9600, 8, 'N', 1);
    if (nset == -1)
        ret = -1;
    printf("serial_init: ret=%d!!\n",ret);
    return ret;
}

int main(void)
{
    int ret = 0;
    ret = serial_init();
    if (ret)
        exit(1);
    
    printf("enter the loop!!\n");
    // ret = write(s_max_fd, "\r\nOK\r\n", 6);
    // if( ret > 0) {
    //     printf("write success!  wait data receive\n");
    // }
    sleep(1);
    while (1)
    {
        get_one_byte_from_uart();
        read_buffer_handler();
        //
        // int nread;
        // char buf1[MAX_READ_BUF];

        // memset(buf1, 0, sizeof(buf1));
        // // ret = write(s_max_fd, "\r\nOK\r\n", 6);
        // // if( ret > 0) {
        // //     printf("write success!  wait data receive\n");
        // // }
        // sleep(1);
        // nread = read(s_max_fd, buf1, 1);
        // //printf("nread:%d\n",nread);
        // if(nread > 0) {
        //     printf("%x ", buf1[0]);
        // }
        
        //sleep(2);
        //nread = read(fd1, buf1,1);
        //if(buf1[0] == 'q')
        //break;
    }
    close(s_max_fd);

    return 0;
}



