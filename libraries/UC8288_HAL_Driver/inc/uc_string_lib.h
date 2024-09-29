#ifndef STRING_LIB_H
#define STRING_LIB_H

#include <stddef.h>

#include "uc_pulpino.h"

// putchar is defined as a macro which gets in the way of our prototype below
#ifdef putchar
#undef putchar
#endif

size_t strlen (const char* str);
int  strcmp (const char* s1, const char* s2);
char* strcpy (char* s1, const char* s2);
//int puts(const char* s);
//int printf(const char* format, ...);
//int sprintf(char* str, const char* fmt, ...);
void* memset (void* dest, int val, size_t len);
int putchar(int s);
void* memcpy(void* dest, const void* src, size_t len);


#endif
