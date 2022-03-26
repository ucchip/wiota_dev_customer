
#ifndef _UC_PRINTF_H_
#define _UC_PRINTF_H_

int printf_(const char *format, ...);
int sprintf_(char* str, const char* fmt, ...);

//#define _UC_BOOT_ALLOW_TRACE_PRITF_TEST_
#ifdef _UC_BOOT_ALLOW_TRACE_PRITF_TEST_
#define TRACE_PRINTF printf_
#define SPRINTF sprintf_
#else
#define TRACE_PRINTF(...) 
#define SPRINTF(...)
#endif

#endif

