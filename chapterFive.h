#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<time.h>

using namespace std;

/*
ͬʱ����������Ҳ�úõĸ�ϰ�ܽ��µ����µ�֪ʶ�㡣
���½ڵĺ��ľ����˽�����ڴ棬�洢�Լ�����ȷ�����������ܡ�

1. ������ͨ���ܹ�ִ�д������Ż������ǣ�Ϊ�˱�֤������Ļ�����İ�ȫ�Ժ���Դ���һ���ԣ���������ǳ�����С�ĵ�ִ���Ż���
���У���Ϊ�ջ���ǣ��ڴ�������á�����ָ����ָ�����ָ��ͬһ�ڴ�λ�õ����������һ������Ż��������ء�
��ˣ��������ͱ���Ҫ���費ͬ��ָ����ܻ�ָ���ڴ��е�ͬһλ�á�

2.��������ÿԪ�ص���������CPE,Cycles per Element��������һ������ִ���ٶȵĿ�����������׷���С��CPE
*/

typedef long data_t;


typedef struct{
	//��������������ĳ���
	long len;
	//�൱�ڶ�����һ������
	data_t *data;
} vec_rec;
typedef vec_rec* vec_prt;
//typedef vec_rec* vec_prt;

/*
��ʼ���µ�����
*/
vec_prt new_vec(long len){
	//�������ָ��
	vec_prt result = (vec_prt) malloc(sizeof(vec_rec));
	//�ѽṹ�������Ԫ�صĿռ�Ҳ������
	data_t *data = NULL;
	//�������ռ�ʧ�ܣ�ֱ�ӷ���
	if(!result)
		return NULL;

	result->len = len;
	//�ٷ������������ڴ�ռ�
	if(len > 0){
		data = (data_t*) malloc(sizeof(data_t) * len);
		//�����������ڴ�ռ�ʧ�ܣ���Ҫ�ͷŸýṹ��Ŀռ䣬Ȼ���ٷ���
		if(!data){
			free(result);
			return NULL;
		}
	}

	result->data = data;
	return result;
}
/*
��ȡ��ǰ�����ĵ�indexλ���ϵ�Ԫ�أ����Ұ�������destָ����λ����
*/
int get_vec_element(vec_prt v,long index, data_t* dest){
	if(index < 0 || index >= v->len){
		return 0;
	}
	*dest = v->data[index];
	return 1;
}

//��ȡ�����ĳ���
long vec_length(vec_prt v){
	return v->len;
}

//ʹ�����������ʼ��������Ԫ�ص�����
void random_initialize(vec_prt v){
	long i;
	srand((unsigned)time(NULL));

	for(i = 0; i < v->len;++i){
		v->data[i] = (data_t)(rand() % 20);
	}
}



//�����������dest��ʼ������
#define OP +
#define IDENT 0

//331����ʱ��
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
�����1�������ѭ����û��Ҫ�ĺ������ã�ʹ�þֲ��������洢�ڵ��������в������������
����ʱ��191
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
�����2�����˲���Ҫ���ڴ�����v->data[i]
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
�����3�����˲���Ҫ���ڴ�����v->data[i]
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
ʹ��ѭ��չ��������k*1չ����ѭ��,
����ʱ��14.����Ǽ�����ѭ��������ԭ��������ʹ�ö���������и��ӿ��š�
������ʱ������ˣ���ʵ��cpeֵ����op�������ӳٽ��ޣ�Ҫ��ͻ�ơ��뿴6
*/
void combine5(vec_prt v, data_t* dest){
	long i;
	int k = 2;
	long length = vec_length(v);
	long limit = length - (k - 1);
	data_t* data = v->data;
	data_t acc = IDENT;

	//ѭ��չ������
	for(i = 0;i < length; i += k){
		acc = (acc OP data[i]) OP data[i + 1];
	}

	for(;i < length; ++i){
		acc = acc OP data[i];
	}
	
	*dest = acc;
}

/*
�����5��ʹ����k * kѭ��չ�������󣬴ﵽָ�����.
����ʱ��15-18�������Ż������Ǻܶ࣬ԭ������������Ǹ�vs�������Ѿ��Ż����ˡ�
*/
void combine6(vec_prt v, data_t* dest){
	long i;
	int k = 2;
	long length = vec_length(v);
	long limit = length - (k - 1);
	data_t* data = v->data;
	data_t acc = IDENT;
	data_t acc2 = IDENT;

	//ѭ��չ������
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
ʹ�����½�ϱ任���������Ӽ���˳���ùؼ�·���ϵ��ӳپ�����
����ʱ��14-15
*/
void combine7(vec_prt v, data_t* dest){
	long i;
	int k = 2;
	long length = vec_length(v);
	long limit = length - (k - 1);
	data_t* data = v->data;
	data_t acc = IDENT;

	//ѭ��չ������
	for(i = 0;i < length; i += k){
		acc = acc OP (data[i] OP data[i + 1]);
	}

	for(;i < length; ++i){
		acc = acc OP data[i];
	}
	
	*dest = acc;
}

/*
���Բ�ͬ��combine������CPE�����ұ�׼��ӡ���
*/
void test_time(void(*func)(vec_prt, data_t*), long len){
	//������ָ�����ȵ��ڴ�ռ�
	vec_prt vec = new_vec(len);
	data_t dest = 0;
	//�����ʼ������
	random_initialize(vec);
	//ʹ��time.h��clock�����ѣ�ִ��1000�Σ�ȡƽ��ֵ
	int clc_num = 1000;
	time_t start = clock();

	for(int i = 0; i < clc_num; ++ i){
		func(vec, &dest);
	}
	time_t run_time = clock() - start;
	cout<< "ѭ��" << clc_num << "�Σ��ܽỨ��" << (double)run_time<<", ƽ����Ҫ"<<(double)run_time / clc_num;

}

/*
���е�5.14�⣬
���У������ú������ѭ��չ���������½�ϱ任���������ܴ����ľ޴�������
�����Ҫ����ʹ��6*1ѭ��չ��
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

