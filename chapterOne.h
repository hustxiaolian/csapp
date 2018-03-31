#include<stdio.h>
#include<iostream>

using namespace std;

typedef unsigned char *byte_pointer;

void show_unsigned_int(unsigned ui);

/*
����̨���Ե��߼�����ʵ����������
*/
int right_shift_arith(int x, int k){
	unsigned xsrl = x >> k;
	unsigned w = sizeof(int) << 3;//��ȡ�û�����int
	unsigned z = 1 << (w - k);//�ƶ���x�����λ
	unsigned mask = z - 1;
	unsigned left = xsrl & mask;
	unsigned right = ~mask & ((!(x >> (w - 1))) + (int)-1);//�Լ��ķ���
	//unsigned right = ~mask &(~(z & xsrl) + z);
	return left | right;
}

/*
���е�2.42��
*/
int div16(int x){
	return x>>4;
}

/*
����2.55��
*/
void show_bytes(byte_pointer start, size_t len){
	size_t i;
	for(i = 0;i < len;++i){
		printf("%p\t0x%0.2x\n", start + i, start[i]);
	}
	printf("\n");
}

void show_int(int x){
	show_bytes((byte_pointer) &x, sizeof(int));
}

void show_float(float f){
	show_bytes((byte_pointer) &f, sizeof(float));
}

void show_double(double d){
	show_bytes((byte_pointer) &d, sizeof(double));
}

/*
����2.57��
*/
void show_short(short s){
	show_bytes((byte_pointer) &s, sizeof(short));
}

void show_long(long l){
	show_bytes((byte_pointer) &l, sizeof(long));
}

void show_unsigned_int(unsigned ui){
	show_bytes((byte_pointer) &ui, sizeof(unsigned));
}

/*
����2.58��
*/
bool is_little_endian(){
	int i = 0x01;
	//�Ѷ�Ӧ�������ó���
	byte_pointer p = (byte_pointer) &i;
	//�鿴�ֽ������׸��ֽ���0x01
	return *p;	
}

/*
����2.59��,����64λ��32λ
(x & 0xFF) | (y & ~0xFF) 
*/


/*
����2.60��
*/
unsigned replace_byte(unsigned x, int i, unsigned char b){
	byte_pointer bp = (byte_pointer) &x;
	//������С�˴洢ģʽ�������ǵ���
	bp[i] = b;
	show_bytes(bp,4);
	//�ۣ�������ջ𣬳�����ǰ���(unsigned)bp,ֱ�Ӱ�bp��ֵַ�������ˣ������ǵ�ַ�Ĵ洢������
	return *((unsigned*)bp);
	//return (unsigned)bp;
}

/*
2.60��
*/
unsigned replace_byte_daan(unsigned x, int i, unsigned char b){
	//���һ���¾���i<<3 == i * 8;
	return (x & ~(0xff << (i << 3))) | (b << (i << 3));
}

/*
����2.61 Aʵ��
*/
bool check_int_all_byte_is_one(int x){
	return ! ~x;
}

/*
����2.62��
*/
bool int_shifts_are_arithmetic(){
	unsigned i = ~(0);
	return check_int_all_byte_is_one(i >> 1);
}

/*
����2.62���
*/
bool int_shifts_are_arithmetic_daan(){
	int i = -1;//λ��ʾ��ȫΪ1
	return (i >> 1) == -1;
}

/*
���е�2.63��,
ps���ҵıʼǱ���С�˴洢���߼�����,δ����
*/
unsigned srl(unsigned x, int k){
	unsigned xsra = (int) x >> k;
	unsigned word_size = sizeof(int) << 3;//�൱��*8
	//unsigned z = 1 << (word_size - k),���������һ��
	unsigned z = 2 << (word_size - k - 1);
	return (z - 1) & xsra;//��� - 1��ɧ��Ȼ��������һ��0x00..fff��ʽ��λ��Ȼ�����£��Ͱ�ǰ�涼���0�ˡ�
}

int sra (int x, int k){
	int xsrl = (unsigned) x >> k;
	unsigned w = sizeof(int) << 3;
	unsigned z = 1 << (w - k - 1);
	unsigned mask = z - 1;
	unsigned right_part = mask & xsrl;//��������˳���xsrl�����λ��ʣ�¹���
	unsigned left_part = ~mask & (~(z & xsrl) + z);//�Բۣ��������ʵ����̫ɧ�ˡ���Ȧû�뵽��
	return left_part | right_part;
}

/*
���е�2.63��,������Ϊ��32 - 1λ�����ġ�
*/
int any_odd_bit_one(unsigned x){
	return (!(~(x | 0xAAAAAAAA)));
}

/*
���е�2.66��
*/
int leftmost_one(unsigned x){
	//���µ��ظ����ǽ���λ��1ȫ���򵽱����͵�����λ�ϡ�Ҳ������ʾ�ϵ�ת����[0..01...1]������ʽ
	x = x | x >> 16;
	x = x | x >> 8;
	x = x | x >> 4;
	x = x | x >> 2;
	x = x | x >> 1;
	return (x >> 1) + 1;
}

/*
���е�2.67��B
*/
int bad_int_size_is_32_B(){
	//�����32λ�Ļ�������ômaskȫ1�������32���ϵĻ�������ô����0x01,Ȼ��ʹ���߼�������ж��¾��С�
	int mask = (1 << 31) >> 31;
	return !(mask + 1);
}

int bad_int_size_is_32_C(){
	//�������������˼·�����������16λ�Ļ��������������λ15
	int mask = (1 << 15) << 15;
	mask = (mask >> 15) >> 15;
	//��������֮�����intС��32λ����ômask = 0x0;���int ����32λ ��mask = 0x01, ֻ��int����32λʱ��mask = -1��ȫ1��;
	return !(mask + 1);
}

/*
����2.68�� 1<=n <=w
*/
int lower_one_mask(int n){
	//����ȫ1
	int mask = ~(0x0);
	//����ֱ����32λ�����´���
	int z = (1 << (n - 1)) << 1;
	//1��λ��Ȼ��ӵ���Ӧ��λ���ϣ����ɽ�ǰ���1ȫ����Ϊ0
	return mask + z;
}

/*
���е�2.69��
*/
unsigned rotate_left(unsigned x, int n){
	//��¼�����Ƴ�ȥ��λ
	int ff = ~(0);
	//����[1..10..0]���������룬ͬʱע��n=0ʱ������ֱ����λ32
	int miss_bytes_mask = (ff << (31 - n)) << 1;
	//��Ҫ�Ƴ���λ�ͼ�¼������
	int miss_bytes = miss_bytes_mask & x;
	//����Ϊ��λ���Ƶ����ʵ�λ��,����ע�������������ƴ�������λ
	int mask = ((miss_bytes >> (31 - n)) >> 1) & (~(ff << n));
	//
	return (x << n) | mask;
}

/*
���е�2.70��Ҳ��dataLab�������Ŀ
˼·���ǣ���������32 - n��������32 - n�����仯ǰ��ĵ�nλ�Ƿ����

�е����⣬��ͷ�����룬���ֻ�������ж��޷���int
*/
int fits_bits(int x, int n){
	//�����ƣ�������
	int xx = ( x << (32 - n)) >> (32 - n);
	//����[00..011..,]����������
	int mask = ~((~(0)) << n );
	//ʹ��������ж������Ƿ���ȫ��ͬ��������ȫ��ͬ����Ϊȫ0
	return !((xx & mask) ^ (x & mask));
}

/*
���ò�˵���𰸵�˼·���Ǻ��"����ı��ʾ����ж�x�ĸ�w - n + 1λ�Ƿ�Ϊ0����-1"
*/
int fits_bits2(int x, int n){
	x = x >> (n - 1);
	return (x == -1) || (x == 0);
}

/*
���е�2.71��Ŀ
����ͨ��

�ҵĻ������������߼����ơ�����vs���е����⡣
*/
int xbyte (unsigned word, int bytenum){
	return (word << ( 24 - (bytenum << 3 ) )) >> 24;
}

/*
���е�2.72��

�����˼·��
1. ��a b���ʱ�����������
2. a > 0, b > 0, t = a + b < 0ʱ�������
3. a < 0, b < 0, t = a + b > 0ʱ�������

���Ǹ��������������������������롣��������Ӧʱ����Ӧ���������λȫ��Ϊ1.

*/
int saturating_add(int x, int y){
	//w = 32
	int w = sizeof(int) << 3;
	//�ݴ������
	int t = x + y;
	int ans = x + y;
	//��x��y��t����ֵ��������31λ����ȡ����λ���������Ϊ0���������ƺ�Ϊȫ0.�������Ϊ1���������ƺ�ȫ1
	x = x >> (w - 1);
	y = y >> (w - 1);
	t = t >> (w - 1);
	//�������沽�������������־,�������־���������־
	//�������������ʱ��pos_ovf����λȫΪ0��neg_ovf����λȫΪ0��non_ovfȫ��Ϊ1.����������ơ�
	//Ҳ����˵��ֻ����һ��ȫΪ1
	int pos_ovf = (~x) & (~y) & (t);
	int neg_ovf = (x) & (y) & (~t);
	int non_ovf = ~(~pos_ovf | ~neg_ovf);
	//����Tmim��Tmax��ע����������ƾ�Ϊ��������
	int Tmin = (1 << (w - 1));
	int Tmax = ~Tmin;
	return (neg_ovf & Tmin) | (non_ovf & ans) | (pos_ovf & Tmax);
}

/*
���е�2.74��
��ʵ�������һ��ǳ����ƣ��о����Ǽ򵥰档

����ͨ��~

ԭ��
1. ���x > 0, y < 0, ���ǽ��С��0.�����������
2. ��֮�����x < 0, y > 0 ,���ǽ������0����˵������������

*/
int tsub_ok(int x, int y){
	//��ȡ��ǰint��λֵ
	int w = sizeof(int) << 3;
	int t = x + y;
	//����31λ
	x >>= (w - 1);
	y >>= (w - 1);
	t >>= (w - 1);
	//������������ȫ1��
	int pos_ovf = ~x & y & t;
	int neg_ovf = x & ~y & ~t;
	//�����û���������Ҳû�и��������ôok
	return !(pos_ovf | neg_ovf);
}
/*
���е�2.78��

ֱ�Ӽ���x>>k����xΪ����ʱ��û���⣬�������룬����xΪ����ʱ�����ǿ����ٽ�������������ж����£�Ȼ����������+1����

*/

int divide_power2(int x, int k){
	int w = sizeof(int) << 3;
	int ans = x >> k;
	//����ʱ����x�ĵ�kλ��ȫΪ0
	int is_add_plus_one = x >> (w - 1) && (x << (w - k));
	return ans + is_add_plus_one;
}

/*
���е�2.79��,�����
*/
int mul3div4(int x){
	//����3x
	int temp = (x << 1) + x;
	
	//3x / 4�����Ǹ����������룬˼·ͬ2.78�⡣�ж�x<0 && x�ĵ�2λ�Ƿ�ȫΪ0
	int w = sizeof(int) << 3;
	int ans = temp >> 2;
	int is_need_plus_one = (temp >> (w - 1)) && (temp << (w - 2));
	return ans + is_need_plus_one;
}
