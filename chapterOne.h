#include<stdio.h>
#include<iostream>

typedef unsigned char *byte_pointer;

void show_unsigned_int(unsigned ui);

/*
用这台电脑的逻辑右移实现算术右移
*/
int right_shift_arith(int x, int k){
	unsigned xsrl = x >> k;
	unsigned w = sizeof(int) << 3;//获取该机器的int
	unsigned z = 1 << (w - k);//移动到x的最高位
	unsigned mask = z - 1;
	unsigned left = xsrl & mask;
	unsigned right = ~mask & ((!(x >> (w - 1))) + (int)-1);//自己的方案
	//unsigned right = ~mask &(~(z & xsrl) + z);
	return left | right;
}

/*
书中第2.42题
*/
int div16(int x){
	return x>>4;
}

/*
书中2.55题
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
书中2.57题
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
书中2.58题
*/
bool is_little_endian(){
	int i = 0x01;
	//把对应的数组拿出来
	byte_pointer p = (byte_pointer) &i;
	//查看字节序列首个字节是0x01
	return *p;	
}

/*
书中2.59题,区分64位和32位
(x & 0xFF) | (y & ~0xFF) 
*/


/*
书中2.60题
*/
unsigned replace_byte(unsigned x, int i, unsigned char b){
	byte_pointer bp = (byte_pointer) &x;
	//由于是小端存储模式，所以是倒着
	bp[i] = b;
	show_bytes(bp,4);
	//哇！是真的恼火，出错的那版是(unsigned)bp,直接把bp地址值传回来了，而不是地址的存储的数据
	return *((unsigned*)bp);
	//return (unsigned)bp;
}

/*
2.60答案
*/
unsigned replace_byte_daan(unsigned x, int i, unsigned char b){
	//理解一件事就是i<<3 == i * 8;
	return (x & ~(0xff << (i << 3))) | (b << (i << 3));
}

/*
书中2.61 A实现
*/
bool check_int_all_byte_is_one(int x){
	return ! ~x;
}

/*
书中2.62题
*/
bool int_shifts_are_arithmetic(){
	unsigned i = ~(0);
	return check_int_all_byte_is_one(i >> 1);
}

/*
书中2.62题答案
*/
bool int_shifts_are_arithmetic_daan(){
	int i = -1;//位表示：全为1
	return (i >> 1) == -1;
}

/*
书中第2.63题,
ps：我的笔记本是小端存储，逻辑右移,未测试
*/
unsigned srl(unsigned x, int k){
	unsigned xsra = (int) x >> k;
	unsigned word_size = sizeof(int) << 3;//相当于*8
	//unsigned z = 1 << (word_size - k),结果和下面一样
	unsigned z = 2 << (word_size - k - 1);
	return (z - 1) & xsra;//这个 - 1很骚，然后生成了一个0x00..fff样式的位，然后与下，就把前面都变成0了。
}

int sra (int x, int k){
	int xsrl = (unsigned) x >> k;
	unsigned w = sizeof(int) << 3;
	unsigned z = 1 << (w - k - 1);
	unsigned mask = z - 1;
	unsigned right_part = mask & xsrl;//构造完成了除了xsrl的最高位的剩下构造
	unsigned left_part = ~mask & (~(z & xsrl) + z);//卧槽，这个操作实在是太骚了。外圈没想到。
	return left_part | right_part;
}

/*
书中第2.63题,这里认为从32 - 1位这样的。
*/
int any_odd_bit_one(unsigned x){
	return (!(~(x | 0xAAAAAAAA)));
}

/*
书中第2.66题
*/
int leftmost_one(unsigned x){
	//以下的重复就是将高位的1全部或到比它低的所有位上。也就是提示上的转换成[0..01...1]这种形式
	x = x | x >> 16;
	x = x | x >> 8;
	x = x | x >> 4;
	x = x | x >> 2;
	x = x | x >> 1;
	return (x >> 1) + 1;
}

/*
书中第2.67题B
*/
int bad_int_size_is_32_B(){
	//如果是32位的机器，那么mask全1，如果是32以上的机器，那么就是0x01,然后使用逻辑运算符判断下就行。
	int mask = (1 << 31) >> 31;
	return !(mask + 1);
}

int bad_int_size_is_32_C(){
	//还是延续上面的思路，由于最低是16位的机器，所有最多移位15
	int mask = (1 << 15) << 15;
	mask = (mask >> 15) >> 15;
	//这样处理之后，如果int小于32位，那么mask = 0x0;如果int 大于32位 ，mask = 0x01, 只有int等于32位时，mask = -1（全1）;
	return !(mask + 1);
}

/*
书中2.68题 1<=n <=w
*/
int lower_one_mask(int n){
	//生成全1
	int mask = ~(0x0);
	//避免直接移32位，导致错误。
	int z = (1 << (n - 1)) << 1;
	//1移位，然后加到相应的位置上，即可将前面的1全部置为0
	return mask + z;
}

/*
书中第2.69题
*/
unsigned rotate_left(unsigned x, int n){
	//记录将被移出去的位
	int ff = ~(0);
	//生成[1..10..0]这样的掩码，同时注意n=0时，不能直接移位32
	int miss_bytes_mask = (ff << (31 - n)) << 1;
	//将要移出的位就记录下来了
	int miss_bytes = miss_bytes_mask & x;
	//将移为的位左移到合适的位置,并且注意消除算术右移带来的置位
	int mask = ((miss_bytes >> (31 - n)) >> 1) & (~(ff << n));
	//
	return (x << n) | mask;
}