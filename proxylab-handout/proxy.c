#include <stdio.h>
#include"csapp.h"
/*
编码日志：
05-04：单线程思考，编码，遇到bug，主要是eof的问题
05-05：手动检测http报文中首部行的结束，（\r\n），跳出循环；思考并且编写多线程版本，利用任务池，（也许这就是线程池。）
        这个版本，无法检测GET方法使用？后面传入的参数。不过题目没要求，我就不具体实现了。
        尝试
05-06：思考了一整天关于信号量，深入分析和理解信号量，生产者-消费者模式以及两类读者-写者问题。
*/


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_OBJ_NUM 10
#define NTHREAD 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *http_port = "80";
static const char *default_connection_opt = "Connection:close\r\n";
static const char *http_version = "HTTP/1.0";
static const char *default_proxyconn_opt = "Proxy-Connection:close\r\n";


/*
缓存结构及其辅助函数的实现区域

这部分，我简答粗暴地将1Mb的空间划分成10份，每个100kb，因此贪图省事地完成题目的要求。同事这十个也是完全分离的，也就是最多支持10写者同时工作。
缺点也是很明显的，那就是只能缓存十个对象，内存空间利用率是十分浪费的。

这里采用写者优先的读者-写者问题。我个人觉得这道题或者说这类题跟过桥问题很像。而且把它类比成过桥问题容易理解。
*/
typedef struct{
    char buf[MAX_OBJECT_SIZE];//用于存储对象
    char uri[MAXLINE];//用于储存对象对应的uri(实际上是host+uri)
    int size;//记录存储对象的实际大小，同时根据size是否为零来判断数据是否有效
    int LRU;//记录对象进入的时间，可能并没有用

    sem_t line_mutex;//对该结构的锁，相当于整个桥的锁

    unsigned int reader_cnt;//该结构当前读者计数器，相当于在桥头出树立了一块计数器，所有读者要过桥必须先过来签到。
    sem_t reader_cnt_mutex;//对reader_cnt共享变量的访问控制，也就是相当于在计数器每次只能有一个人操作，其他人只能等待。

    unsigned int writer_cnt;//该结构中当前写者数量，相当于在桥的另一侧树立一块计数器，所有人写者要过桥必须过来签到。
    sem_t writer_cnt_mutex;//对writer_cnt共享变量的访问控制，也就是相当于在计数器每次只能有一个人操作，其他人只能等待。

    sem_t queue;//每当第一个写者到来并且它在计数器上签到后，整个桥广播，为了满足写者优先，读者知道后，不再去争抢计数器，知道最后一个写者再次广播说：我们写者已经完事了，你们上吧、
}cache_line_t;

typedef struct{
    cache_line_t cache_line[MAX_OBJ_NUM];//申请十个，也就是共计1MB的空间
    char* uri_copy[MAX_OBJ_NUM];
    int cache_line_evic;//记录有已经缓存了几个对象，记录下一个写者更改的行数
    sem_t cache_line_evic_mutex;//控制该变量的访问和修改。
    sem_t uri_copy_mutex;
}cache_t;

/*cache结构的辅助函数*/
/*合理初始化整个cache结构*/
void cache_init(cache_t* cache_p){

    cache_p->cache_line_evic = 0;
    Sem_init(&(cache_p->cache_line_evic_mutex), 0, 1);
    Sem_init(&(cache_p->uri_copy_mutex), 0, 1);

    for(int i = 0; i < MAX_OBJ_NUM; ++i){
        //把副本也初始化下，分配下地址
        cache_p->uri_copy[i] = NULL;//插入的时候动态分配空间，并且赋值
        //bug 重大bug，c语言指针传递和值传递的概念不太清晰
        cache_line_t* oneline = &(cache_p->cache_line[i]);
        //初始化下
        memset(oneline->buf, 0, MAX_OBJECT_SIZE);
        memset(oneline->uri, 0, MAXLINE);
        oneline->size = 0;
        oneline->LRU = 0;
        oneline->reader_cnt = 0;
        oneline->writer_cnt = 0;
        Sem_init(&(oneline->line_mutex), 0, 1);
        Sem_init(&(oneline->reader_cnt_mutex), 0, 1);
        Sem_init(&(oneline->writer_cnt_mutex), 0, 1);
        Sem_init(&(oneline->queue), 0, 1);
    }
}

/*为了方便，把一个读者的信号量过程控制写成两个函数*/
void before_read(cache_t* cache_p, int i){
    cache_line_t* oneline = &(cache_p->cache_line[i]);
    P(&(oneline->queue));//比喻：看下写者大兄弟有没有已经到来了,相当于写者到来时可以在读者计数器外再加一把锁。
    P(&(oneline->reader_cnt_mutex));//比喻：抢夺桥头计数器
    ++oneline->reader_cnt;//计数器+1，公示大众
    if(oneline->reader_cnt == 1){//计数器检查你是不是第一个读者，是的话，代替后面可能的一堆读者申请桥的锁，后面的弟兄就不用申请了直接过桥
        P(&oneline->line_mutex);//锁整个桥，现在整个桥的权限归读者所有
    }
    V(&oneline->reader_cnt_mutex);//释放计数器锁
    V(&oneline->queue);
    /*读者干活，相当于过桥*/
}

void after_read(cache_t* cache_p, int i){
    cache_line_t* oneline = &(cache_p->cache_line[i]);
    P(&oneline->reader_cnt_mutex);//再次占据计数器
    --oneline->reader_cnt;//告诉大家我们已经下桥了。
    if(oneline->reader_cnt == 0){//判断是否是最后一个读者
        V(&oneline->line_mutex);//最后一个读者离开了就释放整个桥
    }
    V(&oneline->reader_cnt_mutex);//释放计数器
}

/*跟读者一样，分成两个部分，由于之后的函数调用，写者的思路跟读者类似，区别就是写者可以控制读者计数器的锁，同时，写者只能一个个通过*/
void before_write(cache_t* cache_p, int i){
    cache_line_t* oneline = &(cache_p->cache_line[i]);
    P(&oneline->writer_cnt_mutex);//去写者计数器签到
    printf("write_before_test\n");
    fflush(stdout);
    ++oneline->writer_cnt;
    if(oneline->writer_cnt == 1){//第一个到来的写者通知下读者那边，哎哎哎，我写大大来了，你们那边别抢了啊。让我先
        P(&oneline->queue);
    }
    V(&oneline->writer_cnt_mutex);//释放写者计数器
    P(&oneline->line_mutex);//等待读者那边老老实实把锁交出来，而且写者有牌面，一个人独占整个桥
    /*写者过桥*/
}

void after_writer(cache_t* cache_p, int i){
    cache_line_t* oneline = &cache_p->cache_line[i];
    V(&oneline->line_mutex);//写者下桥了，并且释放了桥
    P(&oneline->writer_cnt_mutex);//然后跑到写者计数器上，占领计数器
    --oneline->writer_cnt;
    if(oneline->writer_cnt == 0){//最后一个写者需要通知读者：我搞完了，
        V(&oneline->queue);
    }
    V(&oneline->writer_cnt_mutex);//离开计数器
}

/*传入uri，查询cache_uri副本，查看是否有对应的对象，所有的读者，写者公平竞争*/
/*后续改进：
可以为每个uri设一个锁，这样可以做到多个线程同事查询uri副本*/
int cache_find(cache_t* cache_p, char* uri){
    P(&cache_p->uri_copy_mutex);//独占搜索表
    int result;
    //根据uri遍历查询是否已经缓存了该对象
    for(int i = 0; i < MAX_OBJ_NUM; ++i){
        char* cache_uri = cache_p->uri_copy[i];
        if(cache_uri != NULL &&  strcasecmp(cache_uri, uri) == 0){
            result = i;
            V(&cache_p->uri_copy_mutex);//找到了，释放搜索表
            return result;
        }
    }
    //没找到，释放搜索表。
    V(&cache_p->uri_copy_mutex);
    return -1;
}
/*根据LRU策略，获得需要牺牲的块，这里我们使用的策略是：cache中的每个写者到来并且确定要写，都会+1，然后对该值取余数
也就是说：我的LRU策略是简单从头到尾的循环
*/
int get_RLU_num(cache_t* cache_p){
    int result;
    P(&cache_p->cache_line_evic_mutex);
    ++(cache_p->cache_line_evic);
    result = (cache_p->cache_line_evic) % MAX_OBJ_NUM;
    V(&cache_p->cache_line_evic_mutex);
    return result;
}

/*写者，修改副本，这里写者获取lru数值后，立即修改，也就是说，实际uri和副本之间产生一定的时间差。使得前面的*/
void modify_uri_copy(cache_t* cache_p, char* uri_writer, int lru_num){
    P(&cache_p->uri_copy_mutex);
    cache_p->uri_copy[lru_num] = (char*)Malloc(MAXLINE);//动态申请内存空间，让指针指向刚刚申请的空间
    strcpy(cache_p->uri_copy[lru_num], uri_writer);//把写者要写的uri，拷贝过去。
    V(&cache_p->uri_copy_mutex);
}


/*辅助结构实现*/
/*定义一个结构体用于描述任务队列*/
typedef struct{
    int* buf;/*由于套接字描述符刚好可以用int来存储,这里使用动态内存存储*/
    int n;/*描述任务队列的最大值*/
    int front;/*用于任务队列的开端，刚开始这两者重合*/
    int rear;/*用于任务队列的后端，*/
    sem_t mutex;/*访问buf缓冲区的信号量*/
    sem_t slots;/*描述目前buf中有多少空位*/
    sem_t items;/*描述目前buf中有多少项目*/
}task_buf_t;

/*初始化整个结构，*/
void sbuf_init(task_buf_t* sbuf, int nitem){
    sbuf->buf = malloc(sizeof(int) * nitem);
    sbuf->front = 0;
    sbuf->rear = 0;
    sbuf->n = nitem;
    Sem_init(&sbuf->mutex, 0, 1);
    Sem_init(&sbuf->slots, 0, nitem);
    Sem_init(&sbuf->items, 0, 0);
}
/*释放掉数组占据的内存资源*/
void sbuf_free(task_buf_t* sbuf){
    Free(sbuf->buf);
}

/*sbuf任务入队
思路：
1. 首先插入必须要有空位，也就是其对应的信号量不为零，为零时挂起当前线程
2. 上锁，修改任务池
3. 解锁后，把items信号量加1
*/
void sbuf_insert(task_buf_t* sbuf, int item){
    sem_wait(&sbuf->slots);//1
    sem_wait(&sbuf->mutex);//2
    sbuf->buf[(++sbuf->rear % sbuf->n)] = item;
    sem_post(&sbuf->mutex);
    sem_post(&sbuf->items);
}
/*
sbuf任务出队
思路：
1. 任务出队，任务池中要有任务才行，没有则挂起，直到items信号量大于0；
2. 上锁，修改任务池
3. 解锁，把空位的信号量+1
*/
int sbuf_remove(task_buf_t* sbuf){
    int out_item;
    sem_wait(&sbuf->items);
    sem_wait(&sbuf->mutex);
    out_item = sbuf->buf[++sbuf->front % sbuf->n];
    sem_post(&sbuf->mutex);
    sem_post(&sbuf->slots);
    return out_item;
}

/*辅助函数声明*/
void doit(int connfd);
int parse_request(char* buf, char* domain, char* port, char* uri, char* method, char* version);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sigpipe_handler(int sig);
void* thread_doit(void* thread_args);
void serve_cached(int fd, cache_t* cache_p, int i);
void get_filetype(char *filename, char *filetype);

/*全局变量*/
task_buf_t task_buf;//任务池
cache_t cache;//缓存空间

/*主函数*/
int main(int argc, char** args)
{
    int listenfd, connfd;//监听套接字描述符，已连接描述符
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    //错误直接退出。用于指定监听的端口号
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", args[0]);
        exit(1);
    }
    //修改SIGPIPE默认行为。
    Signal(SIGPIPE, sigpipe_handler);
    //初始化sbuf结构体和cache结构体，创建N个工作者线程
    sbuf_init(&task_buf, NTHREAD);
    cache_init(&cache);

    for(int i = 0; i < NTHREAD; ++i){
        //由于工作者都是通过从task——buf中取得connfd，所以不需要传参。第二个参数永远都是NULL
        Pthread_create(&tid, NULL, thread_doit, NULL);
    }
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
        //打印完成后，把该已连接描述符放入任务池中
        sbuf_insert(&task_buf, connfd);
        //关闭套接字的工作由工作者线程自己完成。
    }
}
/*
思路：
1. 首先分离线程，让线程自己运行完成后释放资源
2. 从任务池中获取相应的请求，调用sbuf_remove函数，如果任务池中没有任务，那么工作者线程会阻塞在这里
3. 开始干活，除了sbuf任务池外，其他所有的变量都是局部变量，而任务池的访问已经上锁同步了。
4. 现在共享的变量还有cache结构体，其他变量都是每个线程的局部变量。因此线程安全。
*/
void* thread_doit(void* thread_args){
    //线程分离
    Pthread_detach(Pthread_self());
    /*死循环，不停地在任务池取任务，直到进程退出*/
    while(1){
        int connfd = sbuf_remove(&task_buf);
        doit(connfd);
        Close(connfd);
        printf("normal exit\n");
    }
}

void doit(int connfd){
    //初始化各变量
    //主要用来存储客户端报文中的域名，申请端口号，uri， http方法，以及http版本
    char domain_name[MAXLINE], app_port[MAXLINE], uri[MAXLINE], method[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], header_buf[MAXLINE];
    char obj_buf[MAX_OBJECT_SIZE];
    //判断http申请中是否有port号。
    int proxy_clientfd;
    int rc;
    int cache_index;
    int need_cache_flag = 0;//用于向读者转发相应报文时，标记该对象的是否合适，是否需要缓存
    int start_cache_flag = 0;//标记从哪里开始缓存
    int obj_size;
    //定义一个rio结构体，用于数据的读写。它是可重入的，线程安全的。
    rio_t rio;
    int header_opt_flags[4];

    //全部先统一全0初始化，免得出现昨天奇妙的bug
    memset(domain_name, 0, MAXLINE);
    memset(app_port, 0, MAXLINE);
    memset(uri, 0, MAXLINE);
    memset(method, 0 , MAXLINE);
    memset(version, 0, MAXLINE);
    memset(header_opt_flags, 0, sizeof(int) * 4);
    memset(obj_buf, 0, MAXLINE);//这里必须初始化，必须保证一开始为空串，否则strcat函数会出现未知的结果。
    //初始化rio结构体
    Rio_readinitb(&rio, connfd);
    //读取http报文的首行，并且解析下,这里得考虑下会不会溢出锕
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("thread_num: %d, request header:%s", (int)Pthread_self(), buf);
    //调用parse_request函数解析请求，有端口时参数时返回1，无端口返回0raerror request
    if(! parse_request(buf, domain_name, app_port, uri, method, version)){
        clienterror(connfd, "error request", "888","error request", "parse this request error.");
        return;
    }
    //为了查询，将host和uri结合在一起，才能判断对象是否已经缓存在本地
    strcpy(buf, domain_name);
    strcat(buf, uri);
    //解析成功后，向缓存区提出查询uri副本请求申请
    if((cache_index = cache_find(&cache, buf)) > 0){
        printf("first find ok:%s,%s\n",cache.cache_line[cache_index].uri, buf);
        fflush(stdout);
        //找到了。立即开始准备读者操作
        before_read(&cache, cache_index);
        //判断自己拿到的文件对应的uri确实是这个，如果不是，那么执行
        if(strcasecmp(cache.cache_line[cache_index].uri, buf) == 0){
            //再次确认没错后，构建相应报文
            printf("submit obj cached success:%s(%d)\n", buf, cache_index);
            fflush(stdout);
            serve_cached(connfd, &cache, cache_index);
            //发送完成相应报文后，直接返回

            after_read(&cache, cache_index);
            return;
        }
        //再次确认失败，白忙活一场，被写者插队，点子背，算了还是老老实实地转发报文把
        after_read(&cache, cache_index);
    }

    printf("submit obj cached failed:%s\n", buf);
    fflush(stdout);
    //查询后没有，或者说本来有的，但是被写者插队后，再次确认失败的，向原始服务器转发http请求。

    //在open_clientfd函数内部调用了socket函数和connet函数，open_clientfd函数是线程安全的
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

    printf("send http request Ok\n");
    fflush(stdout);
    //重新初始化rio缓存区，将其绑定到proxy_clientfd
    Rio_readinitb(&rio, proxy_clientfd);
    //服务器返回http响应报文，并且在报文主题部分包含对应的二进制数据。现在就是直接向客户端返回http报文
    //这里复用header_buf的缓冲空间，用于储存需要缓存的对象
    //当然这里需要先解析首部行，判断对象的大小是否合适

    while(need_cache_flag == 0 && (rc = rio_readlineb(&rio, buf, MAXLINE)) > 0){
        if(strstr(buf, "Content-length") != NULL){
            //利用sscanf读取长度
            if(sscanf(buf, "%*[^123456789]%d", &obj_size) >= 0 && obj_size <= MAX_OBJECT_SIZE){
                //读取成功，并且大小合适，置位标记
                need_cache_flag = 1;
            }
        }
        if(rio_writen(connfd, buf, rc) == -1){
            Close(proxy_clientfd);
            break;
        }
    }

    printf("test1:%d,%d\n",need_cache_flag, start_cache_flag);
    fflush(stdout);

    //读取其他首部行,到检测到空行时，知道响应报文的首部行已经结束，下面是主体数据
    while(need_cache_flag && start_cache_flag == 0 && (rc = rio_readlineb(&rio, buf, MAXLINE)) > 0){
        if(strstr(buf, "\r\n") != NULL){
            start_cache_flag = 1;
        }
        if(rio_writen(connfd, buf, rc) == -1){
            Close(proxy_clientfd);
            break;
        }
    }
    printf("test2:%d,%d\n",need_cache_flag, start_cache_flag);
    fflush(stdout);
    //读取转发主体数据，并且把它存储在临时缓冲区中
    while(start_cache_flag && (rc = rio_readlineb(&rio, buf, MAXLINE)) > 0){
        strcat(obj_buf, buf);
        if(rio_writen(connfd, buf, rc) == -1){
            Close(proxy_clientfd);
            break;
        }
    }
    printf("test3:%d,%d\n",need_cache_flag, start_cache_flag);
    fflush(stdout);
    //通过判断标记位判断是否缓冲了对象
    if(need_cache_flag == 1 && start_cache_flag == 1){
        //缓冲了对象，写者再次检查是否已经有重复对象
        strcpy(buf, domain_name);
        strcat(buf, uri);
        printf("再次查重:%s\n", buf);
        fflush(stdout);
        if(cache_find(&cache, buf) < 0){
            //没找到，开始写
            printf("prepare cache obj:%s\n", buf);
            fflush(stdout);
            int i = get_RLU_num(&cache);//获取lru驱动的行，这种lru策略保证了写者的分配尽量均衡
            printf("lru num:%d\n", i);
            modify_uri_copy(&cache, buf, i);//修改uri副本，提示后来查询的所有人，我写者很快将要写了，这样只要是后来的读者就可以马上知道，即使写者还未写，但是它也去读者区排队争夺。因为他直到
            printf("modify uri copy ok:%s\n",cache.uri_copy[i]);
            before_write(&cache, i);
            //写者正式干活
            printf("writing...\n");
            cache.cache_line[i].size = obj_size;
            strcpy(cache.cache_line[i].buf, obj_buf);
            strcpy(cache.cache_line[i].uri, buf);
            after_writer(&cache, i);
        }
        else{
            printf("obj has cached,not repeated:%s\n", buf);
            fflush(stdout);
        }
        //找到了，那我就不缓存了，直接返回
    }
    else{
        printf("origin response header parse error:%d,%d\n", need_cache_flag, start_cache_flag);
        fflush(stdout);
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

//构建已缓存对象的http相应报文
void serve_cached(int fd, cache_t* cache_p, int i){
    char filetype[MAXLINE], buf[MAXBUF];
    int filesize = cache_p->cache_line[i].size;
    /* Send response headers to client */
    get_filetype(cache_p->cache_line[i].uri, filetype);       //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    //line:netp:servestatic:beginserve
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));       //line:netp:servestatic:endserve
    printf("Response headers:\n");
    printf("%s", buf);
    //返回实际对象。
    Rio_writen(fd, cache_p->cache_line[i].buf, filesize);
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}




