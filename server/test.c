#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include <pthread.h> 
#include <sqlite3.h> 

sqlite3 *db;
int rc;
char passwd_d[100];
char sql[100];
static int callback(void *NotUsed, int argc, char **argv, char **azColName) 
{ 
     int i; 
     for(i=0; i<argc; i++) 
     {          
     	strcpy(passwd_d,argv[i]);
     } 
    return 0;
} 

int main()
{
	char username[20];
	char password[20];
	char *temp;
	rc = sqlite3_open("test.db",&db);
	if(rc)
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db)); 
        sqlite3_close(db); 
	}
	sprintf(sql,"create table stu(username char(20), password char(20));");
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("create tables error!\n");
	}

	temp = "username:lw password:123";
	sscanf(temp,"username:%s password:%s",username,password);
	sprintf(sql,"insert into stu(username,password) values('%s','%s');",username,password);
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("insert error!\n");
	}

	temp = "username:lb password:234";
	sscanf(temp,"username:%s password:%s",username,password);
	sprintf(sql,"insert into stu(username,password) values('%s','%s');",username,password);
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("insert error!\n");
	}

	temp = "username:lwlb password:345";
	sscanf(temp,"username:%s password:%s",username,password);
	sprintf(sql,"insert into stu(username,password) values('%s','%s');",username,password);
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("insert error!\n");
	}

	temp = "username:qaz password:345";
	sscanf(temp,"username:%s password:%s",username,password);
	sprintf(sql,"insert into stu(username,password) values('%s','%s');",username,password);
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("insert error!\n");
	}

	temp = "username:wsx password:345";
	sscanf(temp,"username:%s password:%s",username,password);
	sprintf(sql,"insert into stu(username,password) values('%s','%s');",username,password);
	rc = sqlite3_exec(db, sql, callback, 0, NULL); 
	if(rc != SQLITE_OK)
	{
		printf("insert error!\n");
	}

	sprintf(sql, "select password from stu where username='wsx';"); 
	sqlite3_exec(db, sql, callback, 0, NULL);
	printf("\n%s\n",passwd_d);
	return 0;
}