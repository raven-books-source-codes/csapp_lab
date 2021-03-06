/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 *  
 * 数字电路里面的操作即可完成
 * 真值表
 * a  b y
 * 0  0 0
 * 0  1 1
 * 1  0 1
 * 1  1 0
 * 
 * a反b反 *  ab = y
 * 然后通过逻辑变换即可得下式
 */
int bitXor(int x, int y)
{
    return ~((~(~x & y) & (~(x & ~y))));
}

/*
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void)
{
    return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 * 
 *  !! 将0 变为 0 ,非0变为1,可用于排除操作 比如这里要排除 0xff ff ff ff
 *  就可以 !!(0xff ff ff ff + 1) = !1 = 0
 *        
 */
int isTmax(int x)
{
    return !((x + 1) ^ ~x) & !!(x + 1);
}

/*
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 *   这个题目难度不大,生成0xaa aa aa aa即可
 */
int allOddBits(int x)
{
    int temp = 0xaa;
    temp = temp << 8 | temp; // 0x aa aa
    temp = temp << 8 | temp; // 0x aa aa aa
    temp = temp << 8 | temp; // 0x aa aa aa aa
    return !((x & temp) ^ temp);
}

/*
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x)
{
    return (~x) + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x)
{
    // TODO:这里的做法应该超过max ops了
    int acsii_zero = 0x30;
    int acsii_nine = 0x39;
    int negative_acsii_zero = ~acsii_zero + 1;
    int negative_acsii_nine = ~acsii_nine + 1;
    
    int result1 = !(((x + negative_acsii_zero) >> 31) & 1); //0x30 <= x
    int result2 = ((x + negative_acsii_nine) >> 31) & 1;    // x < 0x39
    int result3 = !(x + (negative_acsii_nine));             // x = 0x39
    return result1 & (result2 | result3);
}

/*
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + <<  >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z)
{
    // x 为0 flag= 0 , x不为0,flag = 1
    int flag = !!x;              // flag =1 or flag = 0
    int negative_one = ~0x1 + 1; // -1
    return (~(flag + negative_one) & y) | ((flag + negative_one) & z);
}

/*
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y)
{
    int negative_y = ~y + 1;
    // 取出符号位
    int sx = (x >> 31) & 1;
    int sy = (y >> 31) & 1;
    
    // x>y ==> x-y 的符号位一定是0
    int x_less_equal_than_y = (((x + negative_y) >> 31) & 1) | !(x + negative_y);
    
    // x < 0, y>= 0 sx = 1 sy = 0, x < y
    int result1 = sx & (!sy);
    // x < 0 , y < 0 sx = 1, sy = 1, 且 x <= y
    int result2 = sx & sy & x_less_equal_than_y;
    // x >=0 , y >= 0, sx =0 , sy = 0, 且 x <= y
    int result3 = (!sx) & (!sy) & x_less_equal_than_y;
    // x >=0 , y < 0 , sx = 0, sy = 1, 这里 x > y 需要求反,并且不再其它condition中
    int result4 = (!sx) & sy;
    
    return result1 | result2 | result3 | ((!result4) & result1 & result2 & result3);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 *
 *   考虑0和其它数之间的区别, 0的相反数是其本身(Tmin也是)
 *   1. 对x取相反数 ~x+1
 *   2. x ^ ~x+1 =y 
 *  
 *   result1:
 *   y的首位如果是0,则代表y肯定是 0或者Tmin. 此时把y>>31再+1可得.
 *   0xxxx >> 31 = 0x0000 再+1 = 0x1
 *   如果y的首位是1, y>>31 可得:
 *   1xxxx >> 31 = 0xff ff ff ff 再+1 = 0x0
 *  
 *   result2:
 *   现在问题就变为,如果区分0和tmin
 *   注意到,tmin首位是1,所以用 tmin>>31 + 1一定等于0. 而 0>> 31 + 1一定等于1.
 *   所以可区分.
 */
int logicalNeg(int x)
{
    int negative_x = ~x + 1;
    int result1 = ((x ^ negative_x) >> 31) + 1;
    int result2 = (x >> 31) + 1;
    return result1 & result2;
}

/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x)
{
    // 如果 x< 0, 则翻转x
    int reverse_x = ~x;
    // 使用前面的conditional 来做
    int negative_one = ~0x1 + 1; // -1
    int flag = !(x >> 31);
    x = (~(flag + negative_one) & x) | ((flag + negative_one) & reverse_x);
    // 把最高位1后的所有位全部填充为 1
    x = x | x >> 1;
    x = x | x >> 2;
    x = x | x >> 4;
    x = x | x >> 8;
    x = x | x >> 16;
    // 计算现在1的个数 分治算法 将整个二进制bit分成4段,每段8bit
    int sum = 0;
    int mask = 1;         // 0001
    mask = mask << 8 | 1; // 0000 0001 后文同理
    mask = mask << 8 | 1;
    mask = mask << 8 | 1;
    mask = mask << 8 | 1;
    
    sum += x & mask;
    sum += (x >> 1) & mask;
    sum += (x >> 2) & mask;
    sum += (x >> 3) & mask;
    sum += (x >> 4) & mask;
    sum += (x >> 5) & mask;
    sum += (x >> 6) & mask;
    sum += (x >> 7) & mask;
    
    // 分段计算0的个数
    return (sum & 0xff) + ((sum >> 8) & 0xff) + ((sum >> 16) & 0xff) + ((sum >> 24) & 0xff) + 1; // 符号位
}
//float
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf)
{
    // 获取exp
    unsigned exp = 0;
    exp = (uf & 0x7fffffff) >> 23;
    
    // Nan
    unsigned Nan = 0xff;
    if (exp == Nan) {
        return uf;
    }
    
    // exp == 0
    if (exp == 0) {
        // 除符号位外,left shift即可
        unsigned sign = uf >> 31 & 1;
        uf = uf << 1;
        uf = sign ? (uf | 0x80000000) : (uf & 0x7fffffff);
        return uf;
    }
    
    // here , exp != 0
    // exp + 1即可
    exp += 1;
    if (exp == 0xff) {
        // 变为无穷大
        return (uf & 0x80000000) | 0x7f800000;
    } else {
        return (uf & 0x807fffff) | (exp << 23);
    }
}

/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x)
{
    // 0是特殊情况,本应该采用denormalized方式表达接近0的数,但是这里只有0这个数需要采用denormalized
    if (x == 0)
        return 0;
    
    // tmin 特殊 因为 -tmin = tmin ,二进制情况
    if (x == 0x80000000)
        return 0xcf000000;
    
    // 记录符号位
    int sign = (x >> 31) & 1;
    if (sign == 1)
        x = -x;
    
    const int bias = 127;
    int exp = 0;
    int m = 0;
    
    // 找到最高有效位
    int highest_one_offset = 30;                 // 略过符号位
    while (((x >> highest_one_offset) & 1) != 1) // 因为x!=0, 所有在遍历过程中一定会遇到1
        highest_one_offset--;
    exp = bias + highest_one_offset;
    
    //  构造截断mask,截出所有小数位
    int trunc_frac_mask = 1;
    for (int i = 0; i < highest_one_offset - 1; i++)
        trunc_frac_mask = (trunc_frac_mask << 1) | 1;
    
    // 最低有效位1的便宜量
    int lowest_one_offset = 0;
    while (((x >> lowest_one_offset) & 1) != 1) // 去掉所有末尾的0
        lowest_one_offset++;
    
    // 如果截断长度大于了23位,考虑舍入问题
    int frac_len = highest_one_offset - lowest_one_offset;
    if (frac_len <= 23) {
        // 不用舍入
        m = (x & trunc_frac_mask) >> lowest_one_offset;
        return sign << 31 | exp << 23 | m << (23 - highest_one_offset + lowest_one_offset);
    } else {
        // 需要舍入,(nearest even)
        int temp_frac = (x & trunc_frac_mask) >> lowest_one_offset;
        // 检验有效位后的第一位
        if ((temp_frac >> (frac_len - 23 - 1) & 1) == 0) {
            // 情况1:如果为0,说明将要舍入的部分 未达到小数范围一半直接舍入即可
            m = temp_frac >> (frac_len - 23);
        } else {
            // 情况2:如果为1, 检验是否后面的舍入位是否为全0
            int offset_r = 0;
            while ((offset_r < (frac_len - 23 - 1)) && (temp_frac >> (offset_r) & 1) == 0)
                offset_r++;
            if (offset_r < frac_len - 23 - 1) {
                // 如果后面的舍入位不全为0,则直接向上舍入
                m = (temp_frac >> (frac_len - 23)) + 1;
            } else {
                //  如果后面的舍入位(包含leading位)刚好为一般,及 ?.1000000这种形式,需要考虑偶数舍入
                if ((temp_frac >> (frac_len - 23) & 1) == 0) {
                    //情况3, 向下舍
                    m = temp_frac >> (frac_len - 23);
                } else {
                    // 情况4,向上舍
                    m = (temp_frac >> (frac_len - 23)) + 1;
                }
            }
        }
        return (sign << 31 | exp << 23) + m;
    }
}

/*
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf)
{
    //tmin 是特殊
    if (uf == 0xcf000000) {
        return 0x80000000;
    }
    
    // 获取exp
    unsigned exp = 0;
    exp = (uf & 0x7fffffff) >> 23;
    // 两个特殊情况, exp 全0或全1
    // if exp is 0
    if (exp == 0) {
        return 0;
    }
    // if ex = nan or inf
    if (exp == 0xff) {
        return 0x80000000;
    }
    
    // sign位
    int sign = (uf >> 31) & 1;
    // e就是小数位数
    int e = exp - 127; // bias = 127
    // m小数位
    int m;
    // 保留结果
    int result;
    // 指数 < 0
    if (e < 0) {
        return 0;
    } else if(e == 0){
        return sign ? -1 : 1;
    }
    else {
        // 指数 > 0
        // 找到小数位的第一个1
        int frac_leading_one_offset = 22;
        while (frac_leading_one_offset > 0 && ((uf >> frac_leading_one_offset) & 1) == 0)
            frac_leading_one_offset++;
        if (frac_leading_one_offset == 0) {
            // m为全0
            result = 1 << e;
            return sign ? -result : result;
        } else {
           // m 不为全0 需要是否考虑out of range
           if(frac_leading_one_offset + e >= 31) // 条件检测
           {
               // out of range
               return 0x80000000;
           }else{
               // 没有 out of range
               m = uf & 0x7fffff;
               int result = (1 << (e + 1)) + (m >> (23 - e));
               return sign ? -result : result;
           }
        }
    }
}
