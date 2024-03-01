#include <stdio.h>


/* integer bit operation */
#define BITS_MASK(bit)       ((bit) < 64 ? (1LLU << (bit)) : 0LLU)
#define BITS_SET(value, bit) ((value) |= (1LLU << (bit)))
#define BITS_CLR(value, bit) ((value) &= ~(1LLU << (bit)))
#define BITS_TST(value, bit) (!!((value) & (1LLU << (bit))))


int main()
{
    /* 比特位的运算 */
    long StuStatus = 0;

    BITS_SET(StuStatus, 1);
    BITS_CLR(StuStatus, 2);
    BITS_SET(StuStatus, 3);

    int status = BITS_TST(StuStatus, 3);
    printf("status:%d\n", status);
    




  


}