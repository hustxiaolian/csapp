#include <stdio.h>
#include"csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *http_port = "80";
static const char *default_connection_opt = "Connection:close\r\n";
static const char *http_version = "HTTP/1.0";
static const char *default_proxyconn_opt = "Proxy-Connection:close\r\n";


void doit(int connfd);
int parse_request(char* buf, char* domain, char* port, char* uri, char* method, char* version);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sigpipe_handler(int sig);

int main(int argc, char** args)
{
    int listenfd, connfd;//监听套接字描述符，已连接描述符
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    //错误直接退出。用于指定监听的端口号
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", args[0]);
        exit(1);
    }
    //修改SIGPIPE默认行为。
    Signal(SIGPIPE, sigpipe_handler);
    /*open_listenfd函数思路解析：
    1. 初始化各变量，尤其是一个struct sockaddr类型的指针，用于指向getaddinfo函数返回的结构，hint结构体用于可选项的输入
    2. memset函数全0初始化整个hints结构体
    3. getaddinfo(NULL, port, &hints, &result_listp);第一个null参数已经暗示该套接字用于服务器；如果是客户端，该函数还完成了dns域名解析的过程
    4. 遍历返回的struct sockadd*链表，尝试socket函数和bind函数，成功则退出，否者尝试下一个
    5. 检查for循环中是否成功完成了bind，成功返回listendfd， 否则返回null
    */
    listenfd = Open_listenfd(args[1]);
    /*
    1.死循环，始终等待客户端的连接申请
    2.解析HTTP申请报文（method, uri, domain, port）
    */
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);//等待客户端TCP连接，否则服务端程序阻塞在这里。
        //该函数，完成对HTTP申请报文的解析，并且向初始服务器完成请求。
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accpet connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        //关闭该连接（套接字）
        Close(connfd);
    }
}

void doit(int connfd){
    //初始化各变量
    //主要用来存储客户端报文中的域名，申请端口号，uri， http方法，以及http版本
    char domain_name[MAXLINE], app_port[MAXLINE], uri[MAXLINE], method[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], header_buf[MAXLINE];
    //判断http申请中是否有port号。
    int proxy_clientfd;
    int rc;
    //定义一个rio结构体，用于数据的读写。它是可重入的，线程安全的。
    rio_t rio;
    int header_opt_flags[4];
    memset(header_opt_flags, 0, sizeof(int) * 4);

    //全部先统一全0初始化，免得出现昨天奇妙的bug
    memset(domain_name, 0, MAXLINE);
    memset(app_port, 0, MAXLINE);
    memset(uri, 0, MAXLINE);
    memset(method, 0 , MAXLINE);
    memset(version, 0, MAXLINE);

    //初始化rio结构体
    Rio_readinitb(&rio, connfd);
    //读取http报文的首行，并且解析下,这里得考虑下会不会溢出锕
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("request header:%s", buf);
    //调用parse_request函数解析请求，有端口时参数时返回1，无端口返回0raerror request
    if(! parse_request(buf, domain_name, app_port, uri, method, version)){
        clienterror(connfd, "error request", "888","error request", "parse this request error.");
        return;
    }

    //在open_clientfd函数内部调用了socket函数和connet函数，
    if((proxy_clientfd = Open_clientfd(domain_name, app_port)) < 0){
        clienterror(connfd, "error request", "888","error request", "domain name DNS error.");
        return;
    }

    //根据解析后的结果重新生成申请报文
    sprintf(header_buf, "%s %s %s\r\n", method, uri, version);
    Rio_writen(proxy_clientfd, header_buf, strlen(header_buf));
    printf("proxy-header:%s", header_buf);
    fflush(stdout);
    //rio写，把读缓冲区的直接写入到套接字中,读一行写一行。
    while((rc = rio_readlineb(&rio, buf, MAXLINE)) > 0){
        if(strstr(buf, "Host") != NULL){
            header_opt_flags[0] = 1;
        }
        else if(strstr(buf, "User-Agent") != NULL){
            header_opt_flags[1] = 1;
        }
        else if(strstr(buf, "Connection") != NULL){
            header_opt_flags[2] = 1;
            strcpy(buf, default_connection_opt);
            rc = strlen(buf);
        }
        else if(strstr(buf , "Proxy-Connection") != NULL){
            header_opt_flags[3] = 1;
        }
        //bug原因一直在这里等待着传完，我需要自己判断首部，看来我对套接字EOF的理解还是不够深刻。
        if(!strcmp(buf, "\r\n")){
            break;
        }
        Rio_writen(proxy_clientfd, buf, rc);
    }
    //判断flag数组是否全部为1，看哪些首部行需要补上
    for(int i = 0; i < 4; ++i){
        if(header_opt_flags[i] == 0){
            switch(i){
            case 0:
                sprintf(buf, "Host:%s\r\n", domain_name);break;
            case 1:
                sprintf(buf, "%s", user_agent_hdr);break;
            case 2:
                sprintf(buf, "%s", default_connection_opt);break;
            case 3:
                sprintf(buf, "%s", default_proxyconn_opt);break;
            }
            Rio_writen(proxy_clientfd, buf, strlen(buf));
        }
    }
    Rio_writen(proxy_clientfd, "\r\n", 2);
    //重新初始化rio缓存区，将其绑定到proxy_clientfd
    Rio_readinitb(&rio, proxy_clientfd);
    //服务器返回http响应报文，并且在报文主题部分包含对应的二进制数据。现在就是直接向客户端返回http报文
    while((rc = rio_readlineb(&rio, buf, MAXLINE)) > 0){
        if(rio_writen(connfd, buf, rc) == -1){
            break;
        }
    }
    //关闭套接字
    Close(proxy_clientfd);
}

/*
该函数把首行内容给赋值到domain，port（如果有的话，没有的话使用默认的80端口），uri，method，version（把版本改成1.0）
解析成功，满足要求返回1， 否则返回0
*/
int parse_request(char* buf, char* domain, char* port, char* uri, char* method, char* version){
    char sbuf[MAXLINE];

    sscanf(buf, "%s %s %s", method, sbuf, version);
    //判断方法是否http合法，该函数有
    if(strcasecmp(method, "GET") != 0){
        return 0;
    }
    //使用正则表达式来截取字符串，再判断domain中是否有端口号
    sscanf(sbuf, "%*[htps]://%[^/]%s", sbuf, uri);
    if(strchr(sbuf,':') != NULL){
        //这里出bug的原因是sscanf函数直接domain置值之后，后面继续用该正则表达式确实是找不到对应的字符串
        sscanf(sbuf, "%[^:]:%s", domain, port);
    }
    else{
        strcpy(domain, sbuf);
        strcpy(port, http_port);
    }
    //直接设置版本号
    strcpy(version , http_version);
    printf("host:%s, uri:%s, port:%s\n", domain, uri, port);
    if(!strlen(domain) || !strlen(uri) || !strlen(port)){
        return 0;
    }
    return 1;
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
//捕获SIGPIPE信号但是啥也不做，一旦客户端关闭套接字引起该信号，并且设置了error，rio_write包装的write函数会直接返回-1
//因此循环检测，如果任何时候有该信号，就捕获他，并且在循环中使用未包装的rio_write函数，检测返回值，-1则直接退出while循环
void sigpipe_handler(int sig){

}







