#include <stdio.h>
#include <limits.h>

int isLessOrEqual(int x, int y) {
  int negative_y = ~y + 1;
  int tmin = 1 << 31;
  // 取出符号位
  int sx = (x & tmin) >> 31;
  int sy = (y & tmin) >> 31;
  // x < 0, y>= 0 sx = 1 sy = 0
  int result1 = sx & !sy;
  // x < 0 , y < 0 sx = 1, sy = 1, 且 x <= y 
  int result2 = sx & sy & (!((x + negative_y) & (tmin) ^ (tmin)) | !(x + negative_y));
  // x >=0 , y >= 0, sx =0 , sy = 0, 且 x <= y 
  int result3 = !sx & !sy & (!((x + negative_y) & (tmin) ^ (tmin)) | !(x + negative_y));
  // x >=0 , y < 0 , sx = 0, sy = 1
  int result4 = (!sx & sy);

  return result1 | result2 | result3 | (!result4 & result1 & result2 & result3 );

  // int negative_y = ~y + 1;
  // int tmin = 1 << 31;
  // int result1 = !((x + negative_y) & (tmin) ^ (tmin)); // x < y 且 x -y 没有发生负溢出
  // int result2 = !(x + negative_y);  // x = y
  // int result3 = 1; // x < y 且 x-y 发生了负溢出
}


int logicalNeg(int x) {
  int negative_x = ~x + 1;
  int result1 = ((x ^ negative_x) >> 31) + 1;
  int result2 = (x >> 31) + 1;
  return result1 & result2;
}

int howManyBits(int x)
{
    // 如果 x< 0, 则翻转x
    int reverse_x = ~x;
    // 使用前面的conditional 来做
    int negative_one = ~0x1 + 1;  // -1
    int flag = !(x>>31);
    x = (~(flag + negative_one) & x) | ((flag + negative_one) & reverse_x);
    // 把最高位1后的所有位全部填充为 1
    x = x | x>>1;
    x = x | x>>2;
    x = x | x>>4;
    x = x | x>>8;
    x = x | x>>16;
    // 计算现在1的个数 分治算法 将整个二进制bit分成8段,每段4bit
    int sum = 0;
    int mask = 1; // 0001
    mask = mask <<8 | 1; // 0000 0001 后文同理
    mask = mask <<8 | 1;
    mask = mask <<8 | 1;
    mask = mask <<8 | 1;
    
    sum += x & mask;
    sum += (x>>1) & mask;
    sum += (x>>2) & mask;
    sum += (x>>3) & mask;
    sum += (x>>4) & mask;
    sum += (x>>5) & mask;
    sum += (x>>6) & mask;
    sum += (x>>7) & mask;
    
    // 分段计算0的个数
    return (sum & 0xff) + ((sum>>8) & 0xff) + ((sum>>16) & 0xff) + ((sum >> 24) & 0xff)
           + 1; // 符号位
}
int main(int argc, char const *argv[])
{
    printf("%d\n",howManyBits(0x1));
    return 0;
}

