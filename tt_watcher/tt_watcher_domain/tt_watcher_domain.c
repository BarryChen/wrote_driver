#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

int main(int argc, char **argv)
{
	int fd = 0;
	char* filename="/dev/ttw";
	unsigned char tt_status;
	unsigned long cnt=0;
	int ret;
	struct pollfd fds;//定义一个pollfd结构体key_fds


	fd = open(filename, O_RDWR);//打开dev/firstdrv设备文件
	if (fd < 0)//小于0说明没有成功
	{
		printf("error, can't open %s\n", filename);
		return 0;
	}

	if(argc !=1)
	{
		printf("Usage : %s ",argv[0]);
		return 0;
	}

	fds.fd = fd;//文件
	fds.events = POLLIN;//poll直接返回需要的条件
	fds.revents = 0;

	while(1)
	{

		ret =  poll(&fds, 1, -1);//调用sys_poll系统调用，如果5S内没有产生POLLIN事件，那么返回，如果有POLLIN事件，直接返回
		if(!ret)
		{
			printf("time out\n");
		}
		else
		{
			if(fds.revents == POLLIN)//如果返回的值是POLLIN，说明有数据POLL才返回的
			{
				read(fd, &tt_status, 1);           //读取按键值
				printf("tt_status: %x\n", tt_status);//打印
			}
		}

	}

	return 0;
}