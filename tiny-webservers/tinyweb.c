#include"csapp.h"
/*
 * 各辅助函数的声明区域
 */

void doit(int fd);
void read_requesthdrs(rio_t *rp, char* request_header_buf);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize, char* request_header);
void get_filetype(char* filename, char* filetype);
void serve_dynamics(int fd, char* filename, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);



/*
 * 主函数区域
 */

int main(int argc, char** argv){
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    //该类型定义在sys/socket.h
    socklen_t clientlen;
    //该类型实现协议无关，能够放下各种addr结构
    struct sockaddr_storage clientaddr;

    /*check command-line args*/
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    /*其内部的工作机理十分有趣，
    1. 初始化变量，尤其是一个struct sockaddr类型的指针，用于稍后变量该类型的链表
    2. memset初始化addrinfo结构，然后设置ai_socktype，ai_flags参数，用来指示当前链接为服务端套接字，而且是TCP面向连接的套接字。
    3. getaddinfo(NULL, port, &hints, &result_list);
    4. 遍历链表，调用socket，bind函数，一个个尝试，注意每次对socket函数得到的套接字的释放、
    5. 释放链表，判断结构，非null则调用listen函数，返回套接字描述符。
    */
    listenfd = open_listenfd(argv[1]);
    //循环接受来个所有ip的套接字链接请求
    while(1){
        //为了协议无关，每个协议可能的长度都可能不一样。
        clientlen = sizeof(clientaddr);
        //调用监听套接字的accepet，打开对应的描述符，该函数会填充clientaddr套接字结构体，后面直接调用getnameinfo即可得到客户端的域名地址信息和端口信息。
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        //使用getnameinfo获取申请套接字的ip地址，port端口，
        Getnameinfo((SA*) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        //实际解析相应的http请求，首部行，我们这个tinyweb主要是解析URl
        doit(connfd);
        //调用对应的包装函数，关闭套接字
        Close(connfd);
    }
}

/*
 * 主要是解析http请求的url资源定位符，根据是否是服务器的静态文件。
 * 静态文件，生成http响应报文。动态可执行文件则获取参数fork子进程，执行它，并且前台子进程等待。
 */
void doit(int fd){
    //初始化变量
    int is_static;
    //原来这里是用来保存文件信息的，我以前怎么没看到
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    char request_header_buf[MAXLINE];
    //每次都重新,否则连续两次错误请求将会产生bug
    memset(filename, 0, MAXLINE);
    memset(uri, 0, MAXLINE);
    memset(method, 0 , MAXLINE);
    //线程安全的io读，看来还得回去看看，搞清楚为什么它是线程安全的
    //初始化rio缓冲数据结构
    Rio_readinitb(&rio, fd);
    //读取http申请报文首行到buf
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request header:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s",method, uri, version);
    //strcasecmp函数忽略大小写的比较两个字符串，相等则返回0
    if(strcasecmp(method, "GET")){
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }
    //请求首行
    strcpy(request_header_buf, buf);
    read_requesthdrs(&rio, request_header_buf);
    is_static = parse_uri(uri, filename, cgiargs);
    /*stat函数系统调用获取文件的元数据，比如filesize，mode等等。*/
    if(stat(filename, &sbuf) < 0){
        clienterror(fd, filename, "404", "Not Found", "Tiny couldn't find this file");
        return;
    }

    if(is_static){
        //静态文件下，判断是否是真的静态文件以及检查用户权限
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
            clienterror(fd, filename, "403", "Forbbidden", "Tiny could not read this file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size, NULL);
    }
    else{
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
            clienterror(fd, filename, "403", "Forbidden", "Tiny could not run the CGI program");
            return;
        }
        serve_dynamics(fd, filename, cgiargs);
    }
}
/*
在tiny服务器中，不需要利用请求报文中任何首部航的信息,因此直接在服务端简单的打印，之后略过，不做任何处理直到读到一行只有\r\n直到马上下面就是主题了。
*/
/*习题11.6.1 使得tiny原样返回请求报头*/
void read_requesthdrs(rio_t *rp, char* request_header_buf){
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    strcat(request_header_buf, buf);

    while(strcmp(buf, "\r\n")){
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
        if(strlen(request_header_buf) + strlen(buf) >= MAXLINE){
            unix_error("request header buffer overflow");
            continue;
        }
        strcat(request_header_buf, buf);
    }


    return;
}

/*
生成调用了错误方法的响应报文。
*/
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg){
    char buf[MAXLINE], body[MAXBUF];

    //这里生成响应的html文件主题，也就是http响应报文的主体
    //设置标题行
    sprintf(body, "<html><title>Tiny Error</title>");
    //设置网页背景
    sprintf(body, "%s<body bgcolor = ""fffffe"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em> The Tiny Web  server</em>\r\n",body);
    sprintf(body, "%s</html>",body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf,"Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

/*
解析http请求报文中uri请求，静态文件请求直接返回当前文件返回1，如果是动态文件则需要把cgiargs参数设置好，返回0
*/
int parse_uri(char* uri, char* filename, char* cgiargs){
    char* ptr;


    //printf("uri:%s\n", uri);
    if(!strstr(uri, "cgi-bin")){
        //静态文件的情形,设置参数为空字符串，同事将filename设置为linux的文件格式
        strcpy(cgiargs, "");
        strcpy(filename,".");
        strcat(filename, uri);
        //如果uri最后一个字符为/，表示访问主页
        if(strlen(uri) == 1){
            //fprintf(stdout, "uri:%s\n", uri);
            strcat(filename, "home.html");
            fprintf(stdout, "filename:%s\n", filename);
        }
        return 1;
    }
    else{

        //执行动态文件的情形
        //判断有uri中是否包含参数
        ptr = index(uri, '?');
        if(ptr){
            //有参数的话
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else{
            strcpy(cgiargs, "");
        }
        strcat(filename, ".");
        strcat(filename, uri);
        return 0;
    }

}
/*
 *生成静态内容的相应报文
 */
void serve_static(int fd, char* filename, int filesize, char* request_header){
    int srcfd;
    char* srcp, filetype[MAXLINE], buf[MAXBUF];

    get_filetype(filename, filetype);
    //生成静态http相应报文的头部
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n",buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    printf("Response headers:\n");
    printf("%s", buf);

    //以只读的模式打开一个文件，并且显式内存映射文件的二进制字节到虚拟内存空间的一个地址，返回该地址
    srcfd = Open(filename, O_RDONLY, 0);
    //mmap函数将数据映射到虚拟内存空间的私有只读区域
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);

    if(request_header){
        Rio_writen(fd, request_header, strlen(request_header));
    }

    //释放内存地址资源
    Munmap(srcp, filesize);
}

/*
直接根据申请访问的静态文件名，来生成http相应报文的content-type
*/
void get_filetype(char* filename, char* filetype){
    if(strstr(filename, ".html"))
        strcpy(filetype, "text/html");

    else if(strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");

    else if(strstr(filename, ".png"))
        strcpy(filetype, "image/png");

    else if(strstr(filename, ".jpeg"))
        strcpy(filetype, "image/jpeg");

    else
        strcpy(filetype, "text/plain");
}

/*这里我的浏览器有点不认账啊，主要是它就返回一个报头，只有是运行文件本身去产生相应的响应报文，
或者该函数编写好报文，等待这个程序运算，而且这个结构是一个文件，然后再把这个文件通过静态的方式返回。*/
void serve_dynamics(int fd, char* filename, char* cgiargs){
    char buf[MAXLINE], *emptylist[] = {NULL};

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if(Fork() == 0){
        //子进程
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
        exit(0);
    }
    //主进程等待子进程结束后退出。
    Wait(NULL);
}








