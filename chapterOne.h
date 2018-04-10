#include<stdio.h>
#include<iostream>
#include<stdlib.h>
#include<time.h>

using namespace std;

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

思路：就是直接把一个int变量置为-1，即该变量得位级表示为全1，然后我们右移下，再判断下是否还是-1.
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

/*
书中第2.70，也是dataLab里面的题目
思路就是，把它左移32 - n，再右移32 - n。看变化前后的低n位是否相等

有点问题，回头再想想，这个只能用来判断无符号int
*/
int fits_bits(int x, int n){
	//先左移，再右移
	int xx = ( x << (32 - n)) >> (32 - n);
	//生成[00..011..,]这样的掩码
	int mask = ~((~(0)) << n );
	//使用异或来判断两个是否完全相同，两者完全相同异或后为全0
	return !((xx & mask) ^ (x & mask));
}

/*
不得不说，答案的思路真是很妙。"该题的本质就是判断x的高w - n + 1位是否为0或者-1"
*/
int fits_bits2(int x, int n){
	x = x >> (n - 1);
	return (x == -1) || (x == 0);
}

/*
书中第2.71题目
测试通过

我的机器的右移是逻辑右移。所以vs上有点问题。
*/
int xbyte (unsigned word, int bytenum){
	return (word << ( 24 - (bytenum << 3 ) )) >> 24;
}

/*
书中第2.72题

步骤和思路：
1. 当a b异号时，不会溢出。
2. a > 0, b > 0, t = a + b < 0时，正溢出
3. a < 0, b < 0, t = a + b > 0时，负溢出

就是根据这三个条件，生成三个掩码。当符合相应时，对应掩码的所有位全部为1.

*/
int saturating_add(int x, int y){
	//w = 32
	int w = sizeof(int) << 3;
	//暂存计算结果
	int t = x + y;
	int ans = x + y;
	//将x，y和t三个值都向左移31位。提取符号位。如果符号为0，算术右移后为全0.如果符号为1，算术右移后全1
	x = x >> (w - 1);
	y = y >> (w - 1);
	t = t >> (w - 1);
	//根据上面步骤设置正溢出标志,负溢出标志和无溢出标志
	//这样，当无溢出时，pos_ovf所有位全为0，neg_ovf所有位全为0，non_ovf全部为1.其他情况类似。
	//也就是说，只会有一个全为1
	int pos_ovf = (~x) & (~y) & (t);
	int neg_ovf = (x) & (y) & (~t);
	int non_ovf = ~(~pos_ovf | ~neg_ovf);
	//设置Tmim和Tmax，注意这里的右移均为算术右移
	int Tmin = (1 << (w - 1));
	int Tmax = ~Tmin;
	return (neg_ovf & Tmin) | (non_ovf & ans) | (pos_ovf & Tmax);
}

/*
书中第2.74题
其实这题和上一题非常类似，感觉算是简单版。

测试通过~

原理：
1. 如果x > 0, y < 0, 但是结果小于0.则是正溢出了
2. 反之，如果x < 0, y > 0 ,但是结果大于0，则说明结果负溢出了

*/
int tsub_ok(int x, int y){
	//获取当前int的位值
	int w = sizeof(int) << 3;
	int t = x + y;
	//右移31位
	x >>= (w - 1);
	y >>= (w - 1);
	t >>= (w - 1);
	//如果正溢出，则全1，
	int pos_ovf = ~x & y & t;
	int neg_ovf = x & ~y & ~t;
	//如果既没有正溢出，也没有负溢出，那么ok
	return !(pos_ovf | neg_ovf);
}
/*
书中第2.78题

直接计算x>>k，当x为正数时，没问题，向下舍入，而当x为负数时，我们可以再进行向下舍入后，判断它下，然后看条件向上+1即可

*/

int divide_power2(int x, int k){
	int w = sizeof(int) << 3;
	int ans = x >> k;
	//负数时，且x的低k位不全为0
	int is_add_plus_one = x >> (w - 1) && (x << (w - k));
	return ans + is_add_plus_one;
}

/*
书中第2.79题,会溢出
*/
int mul3div4(int x){
	//计算3x
	int temp = (x << 1) + x;
	
	//3x / 4。考虑负数向零舍入，思路同2.78题。判断x<0 && x的低2位是否全为0
	int w = sizeof(int) << 3;
	int ans = temp >> 2;
	int is_need_plus_one = (temp >> (w - 1)) && (temp << (w - 2));
	return ans + is_need_plus_one;
}

/****************************浮点数位级编程*********************/

typedef unsigned float_bits;

float u2f(unsigned x){
	//先取地址，然后更改指针类型，然后*操作符来读取具体数值。
	return *((float*) &x);
}

unsigned f2u(float f){
	return *((unsigned *) &f);
}

/*直接比较位级是否完全一样*/
bool is_float_eqaul(float_bits f1, float f2){
	return f2u(f2) == f1;
}


/*测试函数*/

int testFun(float_bits (*fun1)(float_bits), float (*fun2)(float)){
	unsigned x = 0;
	do{
		float_bits fb = fun1(x);
		float ff = fun2(u2f(x));

		if(!is_float_eqaul(fb,ff) &&  !_isnan(fb)){
			//%x 输出给定参数的十六进制
			printf("%x error\t float_bits:%x\t real_float:%.2f\n", x, fb, ff);
			x++;
			continue;
		}
		//挨个所有树都检查一遍
		x++;
	}while(x != 0);

	printf("Test Ok!\n");
	return 1;
}

/*
书中第2.92题
compute -f.if f is NaN, then return f

思路：
无非是就是把符号位取反即可。

*/
float_bits float_negative(float_bits f){
	int w = sizeof(int) << 3;
	int sf = 1 << (w - 1);
	int mask = ~sf;
	return (mask & f) | (~f & sf); 
}

float_bits float_negative_answer(float_bits f){
	if(_isnan(f)) return f;
	int w = sizeof(int) << 3;
	int mask = 1 << (w - 1);
	return f ^ mask; 
}

float float_negative(float f){
	return -f;
}

/*
书中第2.93题

思路也是异常简单。直接把最高位的符号位置为0即可。
在浮点数中，所能表示的正负数是完全对称的。

*/
float_bits float_absval(float_bits f){
	if(_isnan(f))
		return f;

	int w = sizeof(int) << 3;
	int mask = ~(1 << (w - 1));
	return mask & f;
}

float float_absval(float f){
	if(_isnan(f))
		return f;
	return abs(f);
}

/*
书中第2.94题

这道题三颗星，有点叼的。主要是考虑非规格-》规格-》非规格的转化。

思路：
1. 如果是最小的那批非规格化数，阶码全0的那批，直接在尾数中左移即可。
2. 1中较大的那部分，乘以2.0，就会变成规格化的数。

其实核心就在于，判断尾数向解码进位的问题
1.直接判断尾数的最高位是否为1。没有，直接移尾数即可。有，移位尾数的基础上，需要在阶码上+1
2.如果f isNaN,直接返回。
3.如果f 的阶码全是1，尾数全0，即无穷大的表示，直接返回。
4.如果f 的阶码全是0，而且尾数全0，即0的表示，直接返回。

*/
float_bits float_twice(float_bits f){
	if(_isnan(f))
		return f;
	//取出f的阶码8位和尾数23位
	unsigned mask_e = (((1 << 8) - 1) << 23);
	unsigned mask_ff = ((1 << 23) - 1);
	unsigned mask_s = (1 << 31);

	unsigned e = f & mask_e;
	unsigned ff = f & mask_ff;
	unsigned s = f & mask_s;
	
	//判断f为0和无穷大的情况
	if(e == mask_e){
		//无穷大的情况
		return f;
	}
	else{
		if(e == 0){
			if(ff == 0)
				return f;
			//最小规格化数的乘以2操作。包含0
			//判断阶码尾数的最高位是否为1
			//bug 1 这个规则只适合最小的非规格化数*2.0规则
			unsigned need_plus_one = ((1 << 22) & ff) << 1;
			return (ff << 1) & mask_ff | (e + need_plus_one) | s;
		}
		else{
			//到这里它已经是正常的规格化数了
			return (f & ~mask_e) | (e + (1 << 23));
		}
	}
}

float float_twice(float f){
	if(_isnan(f))
		return f;
	return f * 2.0;
}

/*
书中第2.95题。
其实思路和上面一题差不多。

还是分成几种情况。
1.如果是nan，直接返回。
2.如果是无穷大，也是直接返回。
3.如果是0，直接返回
4.如果是较大的规格化数，直接返回。
5.如果是较小的规格化数，即阶码只有最低位为1，其他都是0.这时候应该是，阶码置为0后，尾数右移一位。

*/
float_bits float_half(float_bits f){
	if(_isnan(f))
		return f;
	//取出f的阶码8位和尾数23位
	unsigned mask_e = (((1 << 8) - 1) << 23);
	unsigned mask_ff = ((1 << 23) - 1);
	unsigned mask_s = (1 << 31);

	unsigned e = f & mask_e;
	unsigned ff = f & mask_ff;
	unsigned s = f & mask_s;

	//判断f为0和无穷大的情况
	if(e == mask_e){
		//无穷大的情况
		return f;
	}
	else{
		if(e == 0){
			//阶码全为0，说明它要不是非规格化数，要不是零，直接右移尾数即可
			return s | e | (ff >> 1);
		}
		else{
			if(e & (1 << 23) == e){
				//说明是较小的规格化数这种特殊情况
				return s | (f & (~mask_s) >> 1);
			}
			else{
				//较大的规格化数，阶码-1即可
				return s | (e - (1 << 23)) | ff; 
			}
		}
	}
}
