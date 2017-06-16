#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#define port 3333

int sockfd,newfd;
struct sockaddr_in sockaddr;
struct sockaddr_in client_addr;
int sin_size;

void handle(char cmd)
{
	char filename[30]={0};
	int FileNameSize=0;
	int fd;
	int filesize=0;
	int count=0,totalrecv=0;
	char buf[1024];
	struct stat fstat;
	switch(cmd)
	{
		case 'U':
		{
			//接收文件名
			read(newfd, &FileNameSize, 4);
			read(newfd, (void *)filename, FileNameSize);
			filename[FileNameSize]='\0';
			//创建文件
			if((fd = open(filename,O_RDWR|O_CREAT)) == -1)
			{
				perror("creat:");
				_exit(0);	
			}
			//接收文件长度
			read(newfd, &filesize, 4);
			
			//接收文件
			while((count = read(newfd,(void *)buf,1024)) > 0)
			{
				write(fd,&buf,count);
				totalrecv += count;
				if(totalrecv == filesize)
					break;	
			}			
			//关闭文件
			close(fd);
		}
		break;
		
		case 'D':
		{
			//接收文件名
			read(newfd, &FileNameSize, 4);
			read(newfd, filename, FileNameSize);
			filename[FileNameSize]='\0';
			//打开文件
			if((fd = open(filename,O_RDONLY)) == -1)
			{
				perror("creat:");
				_exit(0);	
			}
			//发送文件包括文件长度
			if((stat(filename,&fstat)) == -1)
				return;
			write(newfd,&fstat.st_size,4);
			
			while((count = read(fd,(void *)buf,1024)) > 0)
			{
				write(newfd,&buf,count);	
			}
			close(fd);
		}
		break;	
	}
}
int main()
{
	char cmd;
	//建立连接
	
	//参加socket
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("socket:");
		_exit(0);	
	}
	
	memset(&sockaddr,0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//绑定地址
	if(bind(sockfd,(struct sockaddr *)&sockaddr,sizeof(sockaddr)) == -1)
	{
		perror("bind:");
		_exit(0);	
	}
	//监听
	if(listen(sockfd,10) == -1)
	{
		perror("listen");	
	}
	
	while(1)
	{
		//连接
		if((newfd = accept(sockfd, (struct sockaddr *)(&client_addr),&sin_size)) == -1)
		{
			perror("accept:");	
			_exit(0);
		}
		//处理事件
		while(1)
		{
			read(newfd,&cmd,1);
			
			if(cmd == 'Q')
			{
				break;	
			}
			else
			{
				handle(cmd);	
			}
		}
		close(newfd);
	}	
	close(sockfd);
	return 0;
}