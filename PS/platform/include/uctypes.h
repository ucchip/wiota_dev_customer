#ifndef _UC_TYPE_H
#define _UC_TYPE_H

typedef unsigned long long  u64_t;
typedef signed long long  s64_t;
//typedef long long n64_t;
//typedef long long int n64_t;
//typedef unsigned long long int  u64_t;
//typedef signed long long int  s64_t;
//typedef uint64_t u64_t;
//typedef int64_t s64_t;

typedef unsigned long  ul32_t;
//typedef long nl32_t;
typedef signed long  sl32_t;
//tu32_typedef signed long int  sl32_t;
//typedef unsigned long int  ul32_t;
//typedef long int nl32_t;

typedef signed int  s32_t;
typedef unsigned int  u32_t;
//typedef int n32_t;
//typedef unsigned u32_t;
//typedef int32_t s32_t;
//typedef uint32_t u32_t;

//typedef short n16_t;
typedef signed short  s16_t;
typedef unsigned short  u16_t;
//typedef unsigned short int  u16_t;
//typedef signed short int  s16_t;
//typedef short int n16_t;
//typedef int16_t s16_t;

typedef char n8_t;
typedef signed char  s8_t;
typedef unsigned char  u8_t;
//typedef uint8_t u8_t;
//typedef uint16_t u16_t;
//typedef int8_t s8_t;
typedef unsigned char boolean;

#ifdef _FPGA_
#define BIG_ENDIAN_RNABLE
#endif
#ifdef BIG_ENDIAN_RNABLE
#define     _2B_SWAP_(data_2B)             ((((data_2B) << 8) & 0xff00) | (((data_2B) >> 8) & 0x00ff))
#define     _4B_SWAP_(data_4B)             ((_2B_SWAP_((data_4B) >> 16)) | (_2B_SWAP_((data_4B) & 0xffff) << 16))
#else
#define     _2B_SWAP_(data_2B)             (data_2B)
#define     _4B_SWAP_(data_4B)             (data_4B)
#endif

#endif

