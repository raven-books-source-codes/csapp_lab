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
int main(int argc, char const *argv[])
{
    logicalNeg(0);
    return 0;
}

