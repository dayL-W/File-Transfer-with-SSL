#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#define port 3333

int sockfd,newfd;
struct sockaddr_in sockaddr;
struct sockaddr_in client_addr;
int sin_size;
SSL_CTX *ctx;
SSL *ssl;

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
			SSL_read(ssl, &FileNameSize, 4);
			SSL_read(ssl, (void *)filename, FileNameSize);
			filename[FileNameSize]='\0';
			//创建文件
			if((fd = open(filename,O_RDWR|O_CREAT)) == -1)
			{
				perror("creat:");
				_exit(0);	
			}
			//接收文件长度
			SSL_read(ssl, &filesize, 4);
			
			//接收文件
			while((count = SSL_read(ssl,(void *)buf,1024)) > 0)
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
			SSL_read(ssl, &FileNameSize, 4);
			SSL_read(ssl, filename, FileNameSize);
			filename[FileNameSize]='\0';
			//打开文件
			if((fd = open(filename,O_RDONLY)) == -1)
			{
				perror("creat:");
				_exit(0);	
			}
			//发送文件长度和文件名
			if((stat(filename,&fstat)) == -1)
				return;
			SSL_write(ssl,&fstat.st_size,4);
			
			while((count = read(fd,(void *)buf,1024)) > 0)
			{
				SSL_write(ssl,&buf,count);	
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
	
	//SSL连接
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_server_method());
	//载入数字证书
	SSL_CTX_use_certificate_file(ctx,"./cacert.pem",SSL_FILETYPE_PEM);
	//载入私钥
	SSL_CTX_use_PrivateKey_file(ctx,"./privkey.pem",SSL_FILETYPE_PEM);
	SSL_CTX_check_private_key(ctx);
	//创建socket
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
		//产生新的SSL
		ssl = SSL_new(ctx);
		SSL_set_fd(ssl,newfd);
		SSL_accept(ssl);
		//处理事件
		while(1)
		{
			SSL_read(ssl,&cmd,1);
			
			if(cmd == 'Q')
			{
				break;	
			}
			else
			{
				handle(cmd);	
			}
		}
		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(newfd);
	}	
	close(sockfd);
	SSL_CTX_free(ctx);
	return 0;
}