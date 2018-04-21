#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/*
今天就是要读取一个输入文件，根据输入文件的操作符，操作数，大小。
根据高速缓存的原理，我们编写一定的数据结构和算法，来达到计算这些操作的命中（hit）,不命中（miss)和牺牲或者赶出(eviction)

根据题意和网络上的操作资料，我们可以整理出以下的大致思路：
1.我们的在main函数中，必须能够读取相关参数的输入，处理各种输入参数和错误情况
	a)使用while循环和switch语句的结合来搞事
	
2.我们必须组织一定的数据结构和算法来模拟实现高速缓存的原理
3.在逐行读取文件数据的同时，完成算法（判断是否命中，如果不命中需要重新加载块，并且把原来的块写回内存），统计相关的数据。

-h get help info
-v Optional verbose flag that displays trace info 可选的详细标志，可以显示trace信息
-s <s> Number of set index bits 设置索引位的数量，即设定了组数
-E <E> Associativity (number of lines per set) 设置每组的行数
-b <b> Number of block bits 设定了块大小
-t <tracefile>: Name of the valgrind trace to replay 设定trace文件

再次值得注意的是，这里题目是让我们完成一个模拟器并不是在数据结构中真正存储数据，所以我们对于每一行只需要有三个数据域即可：
1.有效位
2.标记位
3.时间戳（用于LRU策略）

然后我们使用一个该结构的二维数组即可表示整个高速缓冲的每个行的状态，然后也就是能判断是否命中等等操作
具体为cache[S][E] S代表共计有几组，E表示一组内有几行。参考书中话表格的表达方式。

思路逻辑大概有了，开始写。

getopt函数的典型用法参考了cmu的教程：
http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt

fscanf函数的例子和用法参考cmu的教程：
http://crasseux.com/books/ctutorial/fscanf.html

深刻的经验教训：（以后绝对不偷懒了，一定写一个测试一个，一步一步脚印，相处，边测试边推进，远远比写一大段，最后出现莫名奇妙的错误好。尤其是c没有边界检查。）
1. 务必好好领会下值传递和引用传递的区别。搞清楚，啥时候需要值传递，什么时候需要引用传递。
2. 第二点，务必写一个函数，测试检查一个函数。起码要打印出出来，尤其是使用malloc的时候。
3. ide工具真的很重要，这是我第一次在linux上记事本 + gdb调试，真的是，我选择vs。vs真好用。
*/


//定义模拟高速缓存的每一行：即有效位，标记位，以及高速缓存的块大小

typedef struct{
	int valid;//有效位的标志
	long tag;//标记位
	long time_tamp;//时间戳，记录当前行的存入时间
}cache_line;

typedef cache_line* cache;


/*
main函数，主要处理相关参数的处理
主要是学习getopt函数的使用，看网络上的资料就是结合while和switch语句来实现每个参数的跳转。

*/

static void printHelpInfo();
static void get_opt_xl(int argc, char** argv, char* optStr, int* ep, int* sp, int* bp, char** file, int* vp);
static void allocate_all_cache_line(cache* cache_p, int s, int e, int b);
static void read_file_and_excute(cache* cache_p, char* file, int* hitp, int* missp, int* evicp, int e, int s ,int b, int vflag);
static void free_cache(cache* cache_p, int e, int s, int b);
static void handle(cache* cache_p,char op, long addr, int size, int e, int s ,int b, int* hit_p, int* miss_p, int* evic_p, int current_time_tamp, int vflag);

int main(int argc, char** argv)
{
	//初始化变量
	int hit = 0;
	int miss = 0;
	int evictions = 0;

	char* const optstr = "hvs:E:b:t:";
	int e = -1;
	int s = -1;
	int b = -1;
	int vflag = 0;
	char* file = NULL;

	cache cache = NULL;

	//第一部分，设定参数
	get_opt_xl(argc, argv, optstr, &e, &s, &b, &file, &vflag);

	//printf("s = %d, e = %d, b = %d\n", s, e ,b);
	//第二部分：动态分配内存生成高速缓冲行的所需的数据结构，准确地就是生成高速缓冲二维数组
	allocate_all_cache_line(&cache, s, e, b);

	/*
	第三部分：逐行读取文本
			a)判断是否操作符：I L M S
			b)根据操作符的不同和高速缓冲的原理，判断命中，以及不命中情况下根据LRU策略来驱逐一行，再重新加载数据块，相关的计数器必须计数。
	*/
	read_file_and_excute(&cache, file, &hit, &miss, &evictions, e ,s ,b, vflag);


	//最后部分：free掉molloc出来的堆
	free_cache(&cache, e, s, b);

	printSummary(hit, miss, evictions);
	return 0;
}

//-h选项下打印输出相关的信息
static void printHelpInfo(){
	printf("-h get help info\n");
	printf("-v Optional verbose flag that displays trace info 可选的详细标志，可以显示trace信息\n");
	printf("-s <s> Number of set index bits 设置索引位的数量，即设定了组数\n");
	printf("-E <E> Associativity (number of lines per set) 设置每组的行数\n");
	printf("-b <b> Number of block bits 设定了块大小\n");
	printf("-t <tracefile>: Name of the valgrind trace to replay 设定trace文件\n");
}

//使用getopt函数把参数全部传入进来。
//这里，这里使用了指针来传递，保证修改的同一个值
static void get_opt_xl(int argc, char** argv, char* const optstr, int* ep, int* sp, int* bp, char** file, int* vp){

	int c;
	opterr = 0;

	while((c = getopt(argc, argv, optstr)) != -1){
		switch(c){
			case 'h':
				printHelpInfo();
				exit(0);
				break;
			case 'v':
				*vp = 1;
				break;
			case 's':
				//注意这里返回的是字符串，所以需要atoi函数完成转换
				*sp = atoi(optarg);
				//printf("S = %d\n",*sp);
				break;
			case 'E':
				*ep = atoi(optarg);
				//printf("E = %d\n",*ep);
				break;
			case 'b':
				*bp = atoi(optarg);
				//printf("b = %d\n",*bp);
				break;
			case 't':
				*file = optarg;
				//printf("file:%s\n", *file);
				break;
			case '?':
				printf("未知参数");
				printHelpInfo();
				exit(0);
			default:
				abort();
		}
	}
}

//使用malloc函数动态分配内存
static void allocate_all_cache_line(cache* cache_p, int s, int e, int b) {
	//首先分配出s组出来，每个组也就是
	// bug 1 索引位数应该换算成实际组数
	int bigs = 1 << s;
	*cache_p = (cache_line*)(malloc(sizeof(cache_line) * bigs * e));
	cache cache = *cache_p;
	for(int i = 0; i < bigs * e;++i){
		cache[i].valid = 0;
		cache[i].tag = 0;
		cache[i].time_tamp = 0;
	}
}

//逐行读取文件，根据高速缓冲原理，完成计数
static void read_file_and_excute(cache* cache_p, char* file, int* hitp, int* missp, int* evicp, int e, int s ,int b, int vflag){
	//初始化变量
	cache cache = *cache_p;
	char op;
	long addr;
	int size;
	FILE* my_stream;

	//第一部分，使用io函数，打开文件
	my_stream = fopen(file, "r");

	if(my_stream == NULL){
		printf("找不到文件:%s\n",file);
		fflush(stdout);
		exit(0);
	}
	long cnt = 1;
	//fscanf函数返回读到文件结尾会返回null
	while(!feof(my_stream)){
		int tr = fscanf(my_stream, "%c %lx,%x", &op, &addr, &size);
		if(tr != 3){
			continue;
		}
		else{
			if(op != 'I'){
				if(vflag)
					printf("%c %lx,%x ",op,addr,size);
				switch(op){
					case 'L':
					handle(&cache, op, addr, size, e ,s ,b, hitp, missp, evicp, cnt++, vflag);
					break;
					case 'S':
					handle(&cache, op, addr, size, e ,s ,b, hitp, missp, evicp, cnt++, vflag);
					break;
					case 'M':
					//need twice
					handle(&cache, op, addr, size, e ,s ,b, hitp, missp, evicp, cnt++, vflag);
					handle(&cache, op, addr, size, e ,s ,b, hitp, missp, evicp, cnt++, vflag);
					break;
				}
				if(vflag){
					printf("\n");
					fflush(stdout);
				}
			}
		}
	}

	fclose(my_stream);
        
}

//释放分配的内存空间
static void free_cache(cache* cache_p, int e, int s, int b){
	if(*cache_p == NULL)
		return;
	free(*cache_p);
}

/*
判断是否命中，根据高速缓冲原理，首先，我们提取出组索引，然后在组内寻找符合的标记位，同时检查它的有效位
*/
static void handle(cache* cache_p,char op, long addr, int size, int e, int s ,int b, int* hit_p, int* miss_p, int* evic_p, int current_time_tamp, int vflag){
	//首先，先新建一个局部变量来引用cache
	cache cache = *cache_p;
	
	//解析下addr，分离出组索引，标记位
	//也就是前面学习的位级操作
	long co = ((0x1 << b) - 1) & addr;//块偏移
	long ci = ((0x1 << s) - 1 ) & (addr >> b);//组索引
	long ct = ((0x1 << (b + s)) - 1) & (addr >> (b + s));//标记位
			  
	//test
	//printf("ct:%lx\tci:%lx\tco:%lx\t\n",ct, ci, co);
	
	//将co,ci,ct转换成数组索引的形式
	int sindex = ci * e;
	int i;
	//在组内遍历寻找有对应标记的缓存行
	for(i = sindex;i < sindex + e; i++){
		if(cache[i].tag == ct && cache[i].valid == 1){
			break;
		}
	}
	
	//根据索引判断是否命中，并且分情况处理
	if(i != sindex + e){
		//命中情况
		//题意中指出不会出现这种情况，保险起见我还是加上了
		if(co + size > (1 << b)){
			printf("这里！");
		}
		//judge size
		++(*hit_p);
		if(vflag)
			printf("hit ");
	}
	else{
		//不命中
		if(vflag)
			printf("miss ");
		++(*miss_p);
		
		cache_line* empty_line;
		cache_line* replace_line;
		//寻找有无空行
		for(i = sindex;i < sindex + e; i++){
			if(cache[i].valid == 0){
				empty_line = &(cache[i]);
				break;
			}
		}
		
		//判断是否有空行
		if(i != sindex + e){	
			replace_line = empty_line;
		}
		else{
			for(replace_line = &(cache[sindex]),i = sindex + 1;i < sindex + e;++i){
				if(cache[i].time_tamp < replace_line->time_tamp){
					replace_line = &(cache[i]);
				}
			}
			++(*evic_p);
			if(vflag)
				printf("eviction ");
		}
		
		replace_line->valid = 1;
		replace_line->tag = ct;
		replace_line->time_tamp = current_time_tamp;
		
	}
	/*
	//test
	for(int j = 0;j < e*(1 << s); ++j){
		printf("%d",j);
		printf("\tvalid:%d \ttag:%lx \ttime_tamp:%ld\n",cache[j].valid, cache[j].tag, cache[j].time_tamp);
	}
	*/
	//printf("\thandle return!");
}

