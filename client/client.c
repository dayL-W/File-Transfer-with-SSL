#include<stdio.h>
#include<string.h>
#include<conio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#define port 3333

char ipaddr[15];
int sockfd;
struct sockaddr_in sockaddr;
//声明SSL套接字
SSL_CTX *ctx;
SSL *ssl;

int login()
{
	char username[20];
	char password[20];
	char ch;
	int  cnt_ch=0;
	int login_or_create;
	char temp[100];
	char buf[10];
	int  login_flag=0;
	
	printf("please input your selection: 1 to login 2 to create\n");
	printf("selection:");
	scanf("%d",&login_or_create);
	if(login_or_create != 1 && login_or_create != 2)
	{
		return 0;
	}
	printf("username:");
	scanf("%s",username);
	printf("password:");
	while((ch = getch()) != '\n')
	{
		password[cnt_ch] = ch;
		putchar('*');
		cnt_ch++;
	}
	password[cnt_ch] = '\0';
	getchar();
	sprintf(temp,"log:%d username:%s password:%s",login_or_create,username,password);

	SSL_write(ssl,temp,100);
	SSL_read(ssl,buf,10);
	sscanf(buf,"login:%d",&login_flag);

	if(login_flag == 0)
	{
		printf("-----wrong username or password!-------\n");
	}
	else if(login_or_create == 1)
	{
		printf("----------login successufl!------------\n");
	}
	else
	{
		printf("----------create successufl!------------\n");
	}
	//0失败1成功
	return login_flag;
}
int linkS()
{
	//创建socket
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("socket\n");
		_exit(0);
	}
	//连接
	memset(&sockaddr,0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = inet_addr(ipaddr);
	if(connect(sockfd,(struct sockaddr *)&sockaddr,sizeof(sockaddr)) == -1)
	{
		perror("connect\n");
		_exit(0);
	}
	
	ssl = SSL_new(ctx);
	SSL_set_fd(ssl,sockfd);
	if(SSL_connect(ssl) == -1)
	{
		printf("SSL connect error!\n");	
		_exit(0);
	}

	return login();
}

void upload_file(char *filename)
{
	int fd;
	char cmd = 'U';
	int FileNameSize = strlen(filename);
	char buf[1024];
	int count=0;
	struct stat fstat;
	
	//打开文件
	fd = open(filename,O_RDONLY);
	//发送命令
	SSL_write(ssl,&cmd,1);
	
	//发送文件名
	SSL_write(ssl,(void *)&FileNameSize,4);
	SSL_write(ssl,filename, FileNameSize);
	//发送文件长度
	if((stat(filename,&fstat)) == -1)
		return;
	SSL_write(ssl,(void *)&fstat.st_size,4);
	
	//发送文件数据
	while((count = read(fd,(void *)buf,1024)) > 0)
	{
		SSL_write(ssl,buf,count);	
	}
	//关闭文件
	close(fd);
}

void download_file(char *filename)
{
	int fd;
	char cmd = 'D';
	char buf[1024];
	int FileNameSize = strlen(filename);
	int filesize=0,count=0,totalrecv=0;
	
	//发送命令
	SSL_write(ssl,&cmd,1);
	
	//发送文件名
	SSL_write(ssl,(void *)&FileNameSize,4);
	SSL_write(ssl,filename,FileNameSize);
	
	//打开并创建文件
	if((fd = open(filename,O_RDWR|O_CREAT)) == -1)
	{
		perror("open:");
		_exit(0);	
	}
	
	//接收数据
	SSL_read(ssl,&filesize,4);
	while((count = SSL_read(ssl,(void *)buf,1024)) > 0)
	{
		write(fd,buf,count);
		totalrecv += count;
		if(totalrecv == filesize)
			break;	
	}
	
	//关闭文件
	close(fd);
}

void quit()
{
	char cmd = 'Q';
	//发送命令
	SSL_write(ssl,(void *)&cmd,1);
	//关闭及释放SSL连接
	SSL_shutdown(ssl);
	SSL_free(ssl);
	//退出
	_exit(0);
}
void menu()
{
	char cmd;
	char c;
	char file_u[30];
	char file_d[30];
	while(1)
	{
		printf("----------1.Upload Files---------------\n");
		printf("----------2.Download Files-------------\n");
		printf("----------3.Exit-----------------------\n");
		printf("Please input the Client command:");
		scanf("%c",&cmd);	
		
		switch(cmd)
		{
			case '1':
			{
				printf("Upload Files:");
				//输入文件名
				while((c = getchar()) != '\n' && c != EOF);
				fgets(file_u, 30, stdin);
				file_u[strlen(file_u)-1] = '\0';
				//上传文件
				upload_file(file_u);
			}
			break;	
			case '2':
			{
				printf("Download Files:");
				//输入文件名
				while((c = getchar()) != '\n' && c != EOF);
				fgets(file_d, 30, stdin);
				file_d[strlen(file_d)-1] = '\0';
				//下载文件
				download_file(file_d);
			}
			break;	
			case '3':
			{
				//退出
				quit();
				break;
			}
			break;	
			default:
			{
				printf("Please input right command!\n");	
			}
			break;	
		}
	}	
}
int main(int argc, char *args[])
{
	if(argc != 2)
	{
		printf("format error: you mast enter ipaddr like this : client 192.168.0.6\n");
	    	_exit(0);	
	}
	strcpy(ipaddr,args[1]);
	
	//初始化SSl
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	//创建SSL套接字，参数表明支持版本和客户机
	ctx = SSL_CTX_new(SSLv23_client_method());
	if(ctx == NULL)
	{
		printf("Creat CTX error!!!");	
	}
	
	//建立连接
	if(linkS() == 0)
	{
		printf("user name or password error!\n");
		//关闭及释放SSL连接
		SSL_shutdown(ssl);
		SSL_free(ssl);
		//清屏
		system("clear");
		_exit(0);
	}
	//打印菜单
	menu();
	//结尾操作
	close(sockfd);
	//释放CTX
	SSL_CTX_free(ctx);
	return 0;	
}
