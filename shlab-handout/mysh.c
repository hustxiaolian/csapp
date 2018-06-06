#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXARGS 128
#define	MAXLINE	 8192  /* Max text line length */

void eval(char* cmdline);
int parseline(char* buf, char** args);
int builtin_command();
//fgets系统调用函数的包装函数
//将流的n字节放到制定内存地址。
char* Fgets(char* ptr, int n, File* stream);

int main(){
	char cmdline[MAXLINE];
	
	while(1){
		printf("> ");
		Fgets(cmdline, MAXLINE, stdin);
		if(feof(stdin))
			exit(0);
		
		eval(cmdline);
	}
}

/*

*/
void eval(char* cmdline){
	char* argv[MAXARGS];//参数列表
	char* buf[MAXLINE];//命令行存储
	int bg; 			//前后台运行标志
	pid_t pid;
}

/*

*/
char* Fgets(char* ptr, int n, File* stream){
	//to-do
}
