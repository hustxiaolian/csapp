#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<time.h>

using namespace std;

/*
同时，在这里我也好好的复习总结下第五章的知识点。
这章节的核心就是了解机器内存，存储以及编译等方面的性能性能。

1. 编译器通常能够执行大量的优化，但是，为了保证编译出的汇编代码的安全性和与源码的一致性，编译器会非常谨慎小心的执行优化。
其中，最为恼火的是：内存别名引用。它是指两个指针可能指向同一内存位置的情况。它是一个大的优化妨碍因素。
因此，编译器就必须要假设不同的指针可能会指向内存中的同一位置。

2.我们引入每元素的周期数（CPE,Cycles per Element）来衡量一个函数执行速度的快慢。我们是追求更小的CPE
*/

typedef long data_t;


typedef struct{
	//这类来控制数组的长度
	long len;
	//相当于定义了一个数组
	data_t *data;
} vec_rec;
typedef vec_rec* vec_prt;
//typedef vec_rec* vec_prt;

/*
初始化新的向量
*/
vec_prt new_vec(long len){
	//分配这个指针
	vec_prt result = (vec_prt) malloc(sizeof(vec_rec));
	//把结构里面具体元素的空间也分配下
	data_t *data = NULL;
	//如果分配空间失败，直接返回
	if(!result)
		return NULL;

	result->len = len;
	//再分配具体的数组内存空间
	if(len > 0){
		data = (data_t*) malloc(sizeof(data_t) * len);
		//如果数组分配内存空间失败，需要释放该结构体的空间，然后再返回
		if(!data){
			free(result);
			return NULL;
		}
	}

	result->data = data;
	return result;
}
/*
获取当前向量的第index位置上的元素，并且把它存在dest指定的位置上
*/
int get_vec_element(vec_prt v,long index, data_t* dest){
	if(index < 0 || index >= v->len){
		return 0;
	}
	*dest = v->data[index];
	return 1;
}

//获取向量的长度
long vec_length(vec_prt v){
	return v->len;
}

//使用随机数来初始化向量各元素的数据
void random_initialize(vec_prt v){
	long i;
	srand((unsigned)time(NULL));

	for(i = 0; i < v->len;++i){
		v->data[i] = (data_t)(rand() % 20);
	}
}



//定义操作符和dest初始化数据
#define OP +
#define IDENT 0

//331运行时间
void combine1(vec_prt v, data_t* dest){
	long i;

	*dest = IDENT;
	for(i = 0; i < vec_length(v);i++){
		data_t val;
		get_vec_element(v, i ,&val);
		*dest = *dest OP val;
	}
}

/*
相比于1，相除了循环中没必要的函数调用，使用局部变量来存储在迭代过程中不变的向量长度
运行时间191
*/
void combine2(vec_prt v, data_t* dest){
	long i;
	long len = vec_length(v);

	for(i = 0; i < len;++i){
		data_t val;
		get_vec_element(v, i ,&val);
		*dest = *dest OP val;
	}
}

/*
相比于2消除了不必要的内存引用v->data[i]
31
*/
void combine3(vec_prt v, data_t* dest){
	long i;
	long length = vec_length(v);
	data_t* data = v->data;

	for(i = 0;i < length;++i){
		*dest = *dest OP data[i];
	}
}

/*
相比于3消除了不必要的内存引用v->data[i]
30
*/
void combine4(vec_prt v, data_t* dest){
	long i;
	long length = vec_length(v);
	data_t* data = v->data;
	data_t acc = IDENT;

	for(i = 0;i < length;++i){
		acc = acc OP data[i];
	}
	
	*dest = acc;
}

/*
使用循环展开技术，k*1展开内循环,
运行时间14.这就是减少了循环开销的原因，那岂不是使用多个变量并行更加夸张。
别看这里时间减少了，其实其cpe值还是op操作的延迟界限，要想突破。请看6
*/
void combine5(vec_prt v, data_t* dest){
	long i;
	int k = 2;
	long length = vec_length(v);
	long limit = length - (k - 1);
	data_t* data = v->data;
	data_t acc = IDENT;

	//循环展开技术
	for(i = 0;i < length; i += k){
		acc = (acc OP data[i]) OP data[i + 1];
	}

	for(;i < length; ++i){
		acc = acc OP data[i];
	}
	
	*dest = acc;
}

/*
相比于5，使用了k * k循环展开技术后，达到指令级并行.
运行时间15-18，看来优化并不是很多，原因估计上上面那个vs编译器已经优化过了。
*/
void combine6(vec_prt v, data_t* dest){
	long i;
	int k = 2;
	long length = vec_length(v);
	long limit = length - (k - 1);
	data_t* data = v->data;
	data_t acc = IDENT;
	data_t acc2 = IDENT;

	//循环展开技术
	for(i = 0;i < length; i += k){
		acc = (acc OP data[i]);
		acc2 = acc2 OP data[i + 1];
	}

	for(;i < length; ++i){
		acc = acc OP data[i];
	}
	
	*dest = acc OP acc2;
}

/*
使用重新结合变换技术，更加计算顺序，让关键路径上的延迟尽量下
运行时间14-15
*/
void combine7(vec_prt v, data_t* dest){
	long i;
	int k = 2;
	long length = vec_length(v);
	long limit = length - (k - 1);
	data_t* data = v->data;
	data_t acc = IDENT;

	//循环展开技术
	for(i = 0;i < length; i += k){
		acc = acc OP (data[i] OP data[i + 1]);
	}

	for(;i < length; ++i){
		acc = acc OP data[i];
	}
	
	*dest = acc;
}

/*
测试不同的combine函数的CPE，并且标准打印输出
*/
void test_time(void(*func)(vec_prt, data_t*), long len){
	//分配了指定长度的内存空间
	vec_prt vec = new_vec(len);
	data_t dest = 0;
	//随机初始化数据
	random_initialize(vec);
	//使用time.h的clock函数把，执行1000次，取平均值
	int clc_num = 1000;
	time_t start = clock();

	for(int i = 0; i < clc_num; ++ i){
		func(vec, &dest);
	}
	time_t run_time = clock() - start;
	cout<< "循环" << clc_num << "次，总结花费" << (double)run_time<<", 平均需要"<<(double)run_time / clc_num;

}

/*
书中第5.14题，
其中，我来好好体会下循环展开，和重新结合变换给程序性能带来的巨大提升。
这道题要求是使用6*1循环展开
*/
void inner5(vec_prt u, vec_prt v, data_t *dest){
	long i;
	long length = vec_length(v);
	long limit = length - (6 - 1);
	data_t* udata = u->data;
	data_t* vdata = v->data;
	data_t sum = 0;

	for(i = 0; i < limit ; i += 6){
		sum += udata[i] * vdata[i] + udata[i + 1] * vdata[i + 1]
				+udata[i + 2] * vdata[i + 2] + udata[i + 3] * vdata[i + 3]
				+udata[i + 4] * vdata[i + 4] + udata[i + 5] * vdata[i + 5];
	}

	for(;i < length; ++i){
		sum += udata[i] * vdata[i];
	}

	*dest = sum;
}

void inner6(vec_prt u, vec_prt v, data_t *dest){
	long i;
	long length = vec_length(v);
	long limit = length - (6 - 1);
	data_t* udata = u->data;
	data_t* vdata = v->data;
	data_t sum0 = 0;
	data_t sum1 = 0;
	data_t sum2 = 0;
	data_t sum3 = 0;
	data_t sum4 = 0;
	data_t sum5 = 0;

	for(i = 0; i < limit ; i += 6){
		sum0 += udata[i] * vdata[i];
		sum1 += udata[i + 1] * vdata[i + 1];
		sum2 += udata[i + 2] * vdata[i + 2];
		sum3 += udata[i + 3] * vdata[i + 3];
		sum4 += udata[i + 4] * vdata[i + 4];
		sum5 += udata[i + 5] * vdata[i + 5];
	}

	for(;i < length; ++i){
		sum0 += udata[i] * vdata[i];
	}

	*dest = sum0;
}

void inner7(vec_prt u, vec_prt v, data_t *dest){
	long i;
	long length = vec_length(v);
	long limit = length - (6 - 1);
	data_t* udata = u->data;
	data_t* vdata = v->data;
	data_t sum = 0;

	for(i = 0; i < limit ; i += 6){
		sum += udata[i] * vdata[i] + (udata[i + 1] * vdata[i + 1]
				+(udata[i + 2] * vdata[i + 2] + (udata[i + 3] * vdata[i + 3]
				+(udata[i + 4] * vdata[i + 4] + (udata[i + 5] * vdata[i + 5])))));
	}

	for(;i < length; ++i){
		sum += udata[i] * vdata[i];
	}

	*dest = sum;
}
/*
void* my_memset(void *s, int c, size_t n){
	int K = sizeof(unsigned long);
	size_t cnt;
	size_t limit = cnt - 7;
	unsigned long* sulong = (unsigned long*)s;

	for(cnt = 0; cnt < limit; cnt += 8){
		*sulong = 
	}
}
*/

