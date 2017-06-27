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
#include <pthread.h> 
#include <sqlite3.h> 

#define port 3333

typedef struct task 
{ 
    void *(*process) (int arg); 
    int arg;
    struct task *next; 
} Cthread_task; 


/*线程池结构*/ 
typedef struct 
{ 
    pthread_mutex_t queue_lock; 
    pthread_cond_t queue_ready; 

    /*链表结构，线程池中所有等待任务*/ 
    Cthread_task *queue_head; 

    /*是否销毁线程池*/ 
    int shutdown; 
    pthread_t *threadid; 
    
    /*线程池中线程数目*/ 
    int max_thread_num; 
    
    /*当前等待的任务数*/ 
    int cur_task_size; 

} Cthread_pool; 

static Cthread_pool *pool = NULL; 

void *thread_routine (void *arg); 


int sockfd;
struct sockaddr_in sockaddr;
struct sockaddr_in client_addr;
int sin_size;
char passwd_d[20];

SSL_CTX *ctx;
sqlite3 *db; 

void pool_init (int max_thread_num) 
{ 
    int i = 0;
    
    pool = (Cthread_pool *) malloc (sizeof (Cthread_pool)); 

    pthread_mutex_init (&(pool->queue_lock), NULL); 
    /*初始化条件变量*/
    pthread_cond_init (&(pool->queue_ready), NULL); 

    pool->queue_head = NULL; 

    pool->max_thread_num = max_thread_num; 
    pool->cur_task_size = 0; 

    pool->shutdown = 0; 

    pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t)); 

    for (i = 0; i < max_thread_num; i++) 
    {  
        pthread_create (&(pool->threadid[i]), NULL, thread_routine, NULL); 
    } 
} 

void * thread_routine (void *arg) 
{ 
    printf ("starting thread 0x%x\n", pthread_self ()); 
    while (1) 
    { 
        pthread_mutex_lock (&(pool->queue_lock)); 

        while (pool->cur_task_size == 0 && !pool->shutdown) 
        { 
            printf ("thread 0x%x is waiting\n", pthread_self ()); 
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock)); 
        } 

        /*线程池要销毁了*/ 
        if (pool->shutdown) 
        { 
            /*遇到break,continue,return等跳转语句，千万不要忘记先解锁*/ 
            pthread_mutex_unlock (&(pool->queue_lock)); 
            printf ("thread 0x%x will exit\n", pthread_self ()); 
            pthread_exit (NULL); 
        } 

        printf ("thread 0x%x is starting to work\n", pthread_self ()); 

         
        /*待处理任务减1，并取出链表中的头元素*/ 
        pool->cur_task_size--; 
        Cthread_task *task = pool->queue_head; 
        pool->queue_head = task->next; 
        pthread_mutex_unlock (&(pool->queue_lock)); 

        /*调用回调函数，执行任务*/ 
        (*(task->process)) (task->arg); 
        free (task); 
        task = NULL; 
    } 
    /*这一句应该是不可达的*/ 
    pthread_exit (NULL); 
}

/*向线程池中加入任务*/ 
int pool_add_task (void *(*process) (int arg), int arg) 
{ 
    /*构造一个新任务*/ 
    Cthread_task *task = (Cthread_task *) malloc (sizeof (Cthread_task)); 
    task->process = process; 
    task->arg = arg; 
    task->next = NULL;

    pthread_mutex_lock (&(pool->queue_lock)); 
    /*将任务加入到等待队列中*/ 
    Cthread_task *member = pool->queue_head; 
    if (member != NULL) 
    { 
        while (member->next != NULL) 
            member = member->next; 
        member->next = task; 
    } 
    else 
    { 
        pool->queue_head = task; 
    } 

    pool->cur_task_size++; 
    pthread_mutex_unlock (&(pool->queue_lock)); 
    //唤醒一个线程
    //加入
    pthread_cond_signal (&(pool->queue_ready)); 
    
    return 0; 
} 

void handle(char cmd,SSL *ssl)
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

static int callback(void *NotUsed, int argc, char **argv, char **azColName) 
{ 
     int i; 
     for(i=0; i<argc; i++) 
     {          
     	strcpy(passwd_d,argv[i]);
     } 
    return 0;
} 

int login(SSL *ssl)
{
	char username[20];
	char password[33];
	int login_or_create;
	char temp[100];
	char buf[10];
	int  login_flag=0;
	char sql[100];
	int rc;

	sqlite3_open("user.db",&db);
	SSL_read(ssl,temp,100);
	sscanf(temp,"log:%d username:%s password:%s",&login_or_create,username,password);
	//注册
	if(login_or_create == 2)
	{
		sprintf(sql,"insert into stu(username,password) values('%s','%s');",username,password);
		printf("username:%s password:%s\n",username,password);
		rc = sqlite3_exec(db, sql, callback, 0, NULL);
		if(rc == SQLITE_OK)
		{ 
			login_flag = 1;
		} 
		else
		{
			login_flag = 0;
			printf("insert to database error!\n");
		}
	}
	//登录
	else
	{
		sprintf(sql, "select password from stu where username='%s';",username); 
		sqlite3_exec(db, sql, callback, 0, NULL);
		printf("username:%s password:%s\n",username,passwd_d);
		if(strcmp(password,passwd_d) == 0)
		{
			login_flag = 1;
		}
		else
		{
			login_flag = 0;
			return 0;
		}
	}
	sqlite3_close(db);
	sprintf(buf,"login:%d",login_flag);
	SSL_write(ssl,buf,10);
	
	return login_flag;

}
void *myprocess(int args)
{
	SSL *ssl;
	int tmp_fd = args;
	char cmd;

	//产生新的SSL
	ssl = SSL_new(ctx);
	SSL_set_fd(ssl,tmp_fd);
	SSL_accept(ssl);
	//处理事件
	while(1)
	{
		if(login(ssl) == 0)
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
			close(tmp_fd);
			break;	
		}

		SSL_read(ssl,&cmd,1);
		
		if(cmd == 'Q')
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
			close(tmp_fd);
			break;	
		}
		else
		{
			handle(cmd,ssl);	
		}
	}
	return NULL;
}


int main()
{
	int newfd;
	char sql[100];
	int rc;
	//初始化线程池
	pool_init(5);

	//创建数据库
	rc = sqlite3_open("user.db",&db);
	if(rc)
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db)); 
        sqlite3_close(db); 
	}
	sprintf(sql,"create table stu(username char(20), password char(40));");
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("create tables error!\n");
	}
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
		//给线程池添加任务
		pool_add_task(myprocess,newfd);
	}	
	close(sockfd);
	SSL_CTX_free(ctx);
	return 0;
}