//#include <string.h>
#include <stdint.h>
#include <ctype.h>

void* memcpy(void* dest, const void* src, size_t len)
{
    const char* s = src;
    char* d = dest;
    if ((((uintptr_t)dest | (uintptr_t)src) & (sizeof(uintptr_t) -1)) == 0)
    {
        while ((void*)d < (dest + len - (sizeof(uintptr_t) -1)))
        {
            *(uintptr_t*)d = *(const uintptr_t*)s;
            d += sizeof(uintptr_t);
            s += sizeof(uintptr_t);
        }
    }

    while (d < (char*)(dest + len))
    {
        *d = *s;
        d++;
        s++;
    }

    return dest;
}


void* memset(void* dest, int byte, size_t len)
{
    if ((((uintptr_t)dest | len) & (sizeof(uintptr_t) -1)) == 0)
    {
        uintptr_t word = byte & 0xFF;
        word |= word << 8;
        word |= word << 16;
        word |= word << 16 << 16;
        uintptr_t* d = dest;
        while (d < (uintptr_t*)(dest + len))
        {
            *d = word;
            d++;
        }
    }
    else
    {
        char* d = dest;
        while (d < (char*)(dest + len))
        {
            *d = byte;
            d++;
        }
    }
    return dest;
}

size_t strlen(const char* s)
{
    const char* p = s;
    while (*p)
    {
        p++;
    }
    return p - s;
}

int strcmp(const char* s1, const char* s2)
{
    uint8_t c1, c2;

    do
    {
        c1 = *s1;
        c2 = *s2;
        s1++;
        s2++;
    } while (c1 != 0 && c1 == c2);

    return c1 - c2;
}

char* strcpy(char* dest, const char* src)
{
    char* d = dest;
    while (*src)
    {
        *d = *src;
        d++;
        src++;
    }
    *d = *src;
    return dest;
}

long atol(const char* str)
{
    long res = 0;
    int sign = 0;

    while (*str == ' ')
    {
        str++;
    }

    if ((*str == '-') || (*str == '+'))
    {
        sign = (*str == '-');
        str++;
    }

    while ((*str >= '0') && (*str <= '9'))
    {
        res *= 10;
        res += *str - '0';
        str++;
    }

    return sign ? -res : res;
}
