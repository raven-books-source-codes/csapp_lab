#include <stdio.h>
typedef unsigned char * byte_ptr;

void show_byte(byte_ptr ptr, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%p\n", ptr[i]);
    }
}


int main(int argc, char const *argv[])
{
    char buf[20];
    int result = sscanf("hello","%s",buf);
    printf("%d\n",result);
    return 0;
}

