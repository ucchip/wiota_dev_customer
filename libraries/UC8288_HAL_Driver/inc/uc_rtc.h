/*
 * Copyright (C) 2020 UCCHIP CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of UCCHIP CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RTC_H_
#define _RTC_H_

#include "uc_pulpino.h"
#include <stdint.h>

#define PARAM_RTC_Sec_RATE(Sec_rate)         (Sec_rate  <= (0x3B) && Sec_rate  >= (0x00))
#define PARAM_RTC_Min_RATE(Min_rate)         (Min_rate  <= (0x3B) && Min_rate  >= (0x00))
#define PARAM_RTC_Hour_RATE(Hour_rate)       (Hour_rate <= (0x17) && Hour_rate >= (0x00))
#define PARAM_RTC_Year_RATE(Year_rate)       (Year_rate <= (0x63) && Year_rate >= (0x00))
#define PARAM_RTC_Mon_RATE(Mon_rate)         (Mon_rate  <= (0x0C) && Mon_rate  >= (0x01))
#define PARAM_RTC_Day_RATE(Day_rate)         (Day_rate  <= (Day_rate_max[Mon_rate]) && (Day_rate_max[Mon_rate])  >= (0x01))
#define PARAM_RTC_Week_RATE(Week_rate)       (Week_rate <= (0x07) && Week_rate >= (0x01))
#define PARAM_RTC_ADDR(RTCx)                 (RTCx==UCCHIP_RTC)

#define RTC_YEAR_BASE   2000

#define SECOND_PER_MINUTE   (60UL)
#define SECOND_PER_HOUR     (60UL*SECOND_PER_MINUTE)
#define SECOND_PER_DAY      (24UL*SECOND_PER_HOUR)
#define SECOND_PER_NORMAL_YEAR      (365UL*SECOND_PER_DAY)
#define SECOND_PER_LEAP_YEAR        (366UL*SECOND_PER_DAY)

typedef struct
{
    uint16_t year;
    uint8_t  mon;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  week;
} rtc_time_t;

typedef enum
{
    RTC_AM_YEAR = (1U << 6),
    RTC_AM_MON  = (1U << 5),
    RTC_AM_DAY  = (1U << 4),
    RTC_AM_WEEK = (1U << 3),
    RTC_AM_HOUR = (1U << 2),
    RTC_AM_MIN  = (1U << 1),
    RTC_AM_SEC  = (1U << 0),
} RTC_ALARM_MASK;

typedef struct
{
    uint16_t year;
    uint8_t  mon;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  week;
    uint32_t mask;//for rtc alarm mask
} rtc_alarm_t;

typedef enum
{
    RTC_WDAY_MON  = 1,
    RTC_WDAY_TUES = 2,
    RTC_WDAY_WED  = 3,
    RTC_WDAY_THUR = 4,
    RTC_WDAY_FRI  = 5,
    RTC_WDAY_SAT  = 6,
    RTC_WDAY_SUN  = 7,
} RTC_WDAY;

#define RTC_MAKE_YMDW(y, m, d, w)  (((uint32_t)(w) & 0x07) | (((uint32_t)(d) & 0x1f) << 4) | (((uint32_t)(m) & 0x0f) << 12) | ((((uint32_t)(y) - RTC_YEAR_BASE) & 0x7f) << 16))

#define RTC_MAKE_HMS(h, m, s) (((uint32_t)(s) & 0x3f) | (((uint32_t)(m) & 0x3f) << 8)|(((uint32_t)(h) & 0x1f) << 16))

extern void rc32k_init(void);
extern void rtc_init(RTC_TypeDef* RTCx);
extern void rtc_enable(RTC_TypeDef* RTCx);
extern void rtc_disable(RTC_TypeDef* RTCx);
extern void rtc_set_time(RTC_TypeDef* RTCx, rtc_time_t* rtc_time);
extern void rtc_get_time(RTC_TypeDef* RTCx, rtc_time_t* rtc_time);
extern void rtc_set_alarm(RTC_TypeDef* RTCx, rtc_alarm_t* rtc_alarm);
extern void rtc_get_alarm(RTC_TypeDef* RTCx, rtc_alarm_t* rtc_alarm);
extern void rtc_enable_alarm_interrupt(RTC_TypeDef* RTCx);
extern void rtc_disable_alarm_interrupt(RTC_TypeDef* RTCx);
extern void rtc_clear_alarm_pending(RTC_TypeDef* RTCx);
extern void rtc_calibrate(void);

#endif