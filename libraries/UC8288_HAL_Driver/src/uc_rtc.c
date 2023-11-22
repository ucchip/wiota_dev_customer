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

#include <rtthread.h>
#ifdef RT_USING_RTC
#include "uc_rtc.h"
#include "uc_pulpino.h"
#include "uc_event.h"
#include "uc_puf.h"

#ifndef NULL
#define NULL 0
#endif

#ifndef BIT
#define BIT(x) (1 << (x))
#endif
#ifndef MASK
#define MASK(H, L) (uint32_t)((~((0xffffffffU << (H)) << 1)) & (0xffffffffU << (L)))
#endif

#define RC32K_REFERENCE_VALUE 131000000U
//#define RC32K_DEVIATION_VALUE 13100000U/*rc32K accuracy +/-10%*/
//#define RC32K_DEVIATION_VALUE 6550000U/*rc32K accuracy +/-5%*/
//#define RC32K_DEVIATION_VALUE 1310000U/*rc32K accuracy +/-1%*/
#define RC32K_DEVIATION_VALUE 655000U /*rc32K accuracy +/-0.5%*/
//#define RC32K_DEVIATION_VALUE 131000U/*rc32K accuracy +/-0.1%*/
#define RC32K_UPPER_LIMIT_VALUE (RC32K_REFERENCE_VALUE + RC32K_DEVIATION_VALUE)
#define RC32K_LOWER_LIMIT_VALUE (RC32K_REFERENCE_VALUE - RC32K_DEVIATION_VALUE)

#define RTC_AS0 UC_RTC->AS0
#define RTC_AS1 UC_RTC->AS1
#define RTC_ACTRL UC_RTC->ACTRL
#define TPRB UC_TIMER1->CTR
#define TIRB UC_TIMER1->TRR

enum
{
    RC32K_FREQ_FIRST,
    RC32K_BIAS_FIRST,
};

//#define RC32K_DEBUG

#ifdef RC32K_DEBUG
#define RC32K_PRINTF printf
#else
#define RC32K_PRINTF(...)
#endif

int interval2time(int interval, rtc_time_t *rtc_time);

void rc32k_set_clock_freq(uint32_t freq_code)
{
    REG(0x1a104228) |= BIT(14);                                                               //manual mode
    REG(0x1a104228) = (REG(0x1a104228) & ~MASK(22, 15)) | ((freq_code << 15) & MASK(22, 15)); //set rc32k clock frequency code
}

void rc32k_set_bias_current(uint32_t current_code)
{
    REG(0x1a104228) |= BIT(14);                                                                  //manual mode
    REG(0x1a104228) = (REG(0x1a104228) & ~MASK(12, 10)) | ((current_code << 10) & MASK(12, 10)); //set rc32k bias current code
}

void rc32k_calibrate(void)
{
    REG(0x1a104228) |= BIT(0) | BIT(1); //enable and reset calibrate
    while (REG(0x1a104228) & BIT(0))    //wait calibrate complete
    {
        asm volatile("nop");
    }
}

static uint32_t rc32k_get_trim(uint32_t *bias_code, uint32_t *freq_code)
{
    *bias_code = (REG(0x1a104228) & MASK(12, 10)) >> 10;
    *freq_code = (REG(0x1a104228) & MASK(22, 15)) >> 15;
    return 0;
}

static void rc32k_alarm(uint32_t seconds)
{
    rtc_time_t rtc_time;

    //rtc_get_rtc_time(&rtc_time.year, &rtc_time.mon, &rtc_time.day, &rtc_time.week, &rtc_time.hour, &rtc_time.min, &rtc_time.sec);
    rtc_get_time(UC_RTC, &rtc_time);
    interval2time(1, &rtc_time);

    //set alram rtc_time and time
    if (rtc_time.year >= 2000)
    {
        rtc_time.year -= 2000;
    }
    RTC_AS0 = (((rtc_time.hour & 0x1f) << 16) | ((rtc_time.min & 0x3f) << 8) | ((rtc_time.sec & 0x3f)));
    RTC_AS1 = (((rtc_time.year & 0x3f) << 16) | ((rtc_time.mon & 0xf) << 12) | ((rtc_time.day & 0x1f) << 4) | (rtc_time.week & 0x7));

    RTC_ACTRL |= BIT(8);        //enable alarm
    IER |= BIT(0);              //enable alarm interrupt
    ICP |= BIT(0);              //clear alarm pending
    while ((IPR & BIT(0)) == 0) //wait alarm pending
    {
        asm volatile("nop");
    }
    //ICP |= BIT(0);
}

uint32_t rc32k_measure(uint32_t seconds)
{
    uint32_t count;
    //start timer counting
    TPRB &= ~BIT(0);
    TIRB = 0;
    TPRB = BIT(0);
    rc32k_alarm(seconds); //to measure the time
    count = TIRB;
    TPRB &= ~BIT(0);
    //stop timer counting

    return count;
}

static uint32_t rc32k_abs_diff(uint32_t value)
{
    return value <= RC32K_REFERENCE_VALUE ? (RC32K_REFERENCE_VALUE - value) : (value - RC32K_REFERENCE_VALUE);
}

uint32_t rc32k_autoset_bias(void)
{
    uint32_t try_cnt, cur, delta, count, diff;
    uint32_t best_bias_code, best_count, best_diff;

    best_diff = 0xffffffffU;
    best_count = 0;
    delta = (1 << 2);
    cur = delta;
    best_bias_code = cur;
    for (try_cnt = 0; try_cnt < 3; try_cnt++)
    {
        rc32k_set_bias_current(cur);
        rc32k_calibrate();
        count = rc32k_measure(1);
        diff = rc32k_abs_diff(count);
        if (diff < best_diff)
        {
            best_diff = diff;
            best_bias_code = cur;
            best_count = count;
        }
        RC32K_PRINTF("cur code=%d count=%d\n", cur, count);
        if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
        {
            cur &= ~delta;
            if (try_cnt == 2)
            {
                rc32k_set_bias_current(cur);
                rc32k_calibrate();
                count = rc32k_measure(1);
                diff = rc32k_abs_diff(count);
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_bias_code = cur;
                    best_count = count;
                }
                RC32K_PRINTF("cur code=%d count=%d\n", cur, count);
            }
        }
        else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
        {
            //do nothing
        }
        else //RC32K is OK
        {
            break;
        }
        delta >>= 1;
        cur |= delta;
    }

    if (cur != best_bias_code)
    {
        cur = best_bias_code;
        rc32k_set_bias_current(cur);
        rc32k_calibrate();
        count = best_count;
        RC32K_PRINTF("cur code=%d count=%d\n", cur, count);
    }

    return count;
}

uint32_t rc32k_autoset_freq(void)
{
    uint32_t try_cnt, cur, delta, diff, count;
    uint32_t best_freq_code, best_count, best_diff;

    best_diff = 0xffffffffU;
    best_count = 0;
    delta = (1 << 7);
    cur = delta;
    best_freq_code = cur;
    for (try_cnt = 0; try_cnt < 8; try_cnt++)
    {
        rc32k_set_clock_freq(cur);
        rc32k_calibrate();
        count = rc32k_measure(1);
        diff = rc32k_abs_diff(count);
        if (diff < best_diff)
        {
            best_diff = diff;
            best_freq_code = cur;
            best_count = count;
        }
        RC32K_PRINTF("freq code=%d count=%d\n", cur, count);
        if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
        {
            cur &= ~delta;
            if (try_cnt == 7)
            {
                rc32k_set_clock_freq(cur);
                rc32k_calibrate();
                count = rc32k_measure(1);
                diff = rc32k_abs_diff(count);
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_freq_code = cur;
                    best_count = count;
                }
                RC32K_PRINTF("freq code=%d count=%d\n", cur, count);
            }
        }
        else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
        {
            //do nothing
        }
        else //RC32K is OK
        {
            break;
        }
        delta >>= 1;
        cur |= delta;
    }

    if (cur != best_freq_code)
    {
        cur = best_freq_code;
        rc32k_set_clock_freq(cur);
        rc32k_calibrate();
        count = best_count;
        RC32K_PRINTF("freq code=%d count=%d\n", cur, count);
    }

    return count;
}

void rc32k_autoset(uint32_t mode)
{
    rtc_time_t rtc_time;
    uint32_t try_cnt, cur, delta, count, diff;
    uint32_t best_bias_code, best_freq_code, best_count, best_diff;

    //try to auto set bias current, only do it once after power up
    rtc_time.year = 2020;
    rtc_time.mon = 1;
    rtc_time.day = 1;
    rtc_time.week = 3;
    rtc_time.hour = 0;
    rtc_time.min = 0;
    rtc_time.sec = 0;
    rtc_set_time(UC_RTC, &rtc_time);

    rc32k_alarm(2); //to find starting line

    if (mode == RC32K_FREQ_FIRST)
    {
        best_diff = 0xffffffffU;
        best_count = 0;
        delta = (1 << 7);
        cur = delta;
        for (try_cnt = 0; try_cnt < 8; try_cnt++)
        {
            rc32k_set_clock_freq(cur);
            rc32k_calibrate();
            count = rc32k_autoset_bias();
            diff = rc32k_abs_diff(count);
            if (diff < best_diff)
            {
                best_diff = diff;
                best_count = count;
                rc32k_get_trim(&best_bias_code, &best_freq_code);
                RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
            }
            if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
            {
                cur &= ~delta;
                if (try_cnt == 7)
                {
                    rc32k_set_clock_freq(cur);
                    rc32k_calibrate();
                    count = rc32k_autoset_bias();
                    diff = rc32k_abs_diff(count);
                    if (diff < best_diff)
                    {
                        best_diff = diff;
                        best_count = count;
                        rc32k_get_trim(&best_bias_code, &best_freq_code);
                        RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
                    }
                }
            }
            else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
            {
                //do nothing
            }
            else //RC32K is OK
            {
                break;
            }
            delta >>= 1;
            cur |= delta;
        }

        if (cur != best_freq_code)
        {
            rc32k_set_bias_current(best_bias_code);
            rc32k_set_clock_freq(best_freq_code);
            rc32k_calibrate();
            count = best_count;
            RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
        }
    }
    else if (mode == RC32K_BIAS_FIRST)
    {
        best_count = 0;
        best_diff = 0xffffffffU;
        rc32k_get_trim(&best_bias_code, &best_freq_code);
        delta = (1 << 2);
        cur = delta;
        for (try_cnt = 0; try_cnt < 3; try_cnt++)
        {
            rc32k_set_bias_current(cur);
            rc32k_calibrate();
            count = rc32k_autoset_freq();
            diff = rc32k_abs_diff(count);
            if (diff < best_diff)
            {
                best_diff = diff;
                best_count = count;
                rc32k_get_trim(&best_bias_code, &best_freq_code);
                RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
            }
            if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
            {
                cur &= ~delta;
                if (try_cnt == 2)
                {
                    rc32k_set_bias_current(cur);
                    rc32k_calibrate();
                    count = rc32k_autoset_freq();
                    diff = rc32k_abs_diff(count);
                    if (diff < best_diff)
                    {
                        best_diff = diff;
                        best_count = count;
                        rc32k_get_trim(&best_bias_code, &best_freq_code);
                        RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
                    }
                }
            }
            else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
            {
                //do nothing
            }
            else //RC32K is OK
            {
                break;
            }
            delta >>= 1;
            cur |= delta;
        }

        if (cur != best_bias_code)
        {
            rc32k_set_bias_current(best_bias_code);
            rc32k_set_clock_freq(best_freq_code);
            rc32k_calibrate();
            count = best_count;
            RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
        }
    }
}

void rc32k_init(void)
{
#if 0
    rc32k_set_clock_freq(0x80);
    rc32k_set_bias_current(1);//set bias current code manually
    rc32k_calibrate();
#else
    //rc32k_autoset(RC32K_FREQ_FIRST);
    rc32k_autoset(RC32K_BIAS_FIRST);
#endif
    IER &= (~(1 << 0)); //close RTC alm interrput
    ICP |= 1;           //clear RTC interrput pending
}

void rtc_init(RTC_TypeDef *RTCx)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));

    RTCx->ACTRL = 0;            // clear alarm control reg
    RTCx->CTRL &= ~(1U << 0);   // enable rtc


    /* do not reset rtc time value */
    /* 
    RTCx->TS0 = RTC_MAKE_HMS(0, 0, 0);                            //00:00:00
    RTCx->TS1 = RTC_MAKE_YMDW(RTC_YEAR_BASE, 1, 1, RTC_WDAY_SAT); //from 'RTC_YEAR_BASE'.01.01

    RTCx->CTRL = (1U << 1);               //uprtc_time
    while ((RTCx->CTRL & (1U << 1)) != 0) //wait ready
    {
        asm("nop");
    }
    */
}

void rtc_enable(RTC_TypeDef *RTCx)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    RTCx->CTRL &= ~(1U << 0); //enable rtc
}

void rtc_disable(RTC_TypeDef *RTCx)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    RTCx->CTRL |= (1U << 0); //disable rtc
}

void rtc_set_time(RTC_TypeDef *RTCx, rtc_time_t *rtc_time)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    CHECK_PARAM(PARAM_RTC_Sec_RATE(rtc_time->sec));
    CHECK_PARAM(PARAM_RTC_Min_RATE(rtc_time->min));
    CHECK_PARAM(PARAM_RTC_Hour_RATE(rtc_time->hour));
    CHECK_PARAM(PARAM_RTC_Year_RATE((rtc_time->year - RTC_YEAR_BASE)));
    CHECK_PARAM(PARAM_RTC_Mon_RATE(rtc_time->mon));
    CHECK_PARAM(PARAM_RTC_Day_RATE(rtc_time->day));

    RTCx->TS0 = RTC_MAKE_HMS(rtc_time->hour, rtc_time->min, rtc_time->sec);
    RTCx->TS1 = RTC_MAKE_YMDW(rtc_time->year, rtc_time->mon, rtc_time->day, rtc_time->week);

    RTCx->CTRL |= (1U << 1);              //uprtc_time
    while ((RTCx->CTRL & (1U << 1)) != 0) //wait ready
    {
        asm("nop");
    }
}

void rtc_get_time(RTC_TypeDef *RTCx, rtc_time_t *rtc_time)
{
    uint32_t tm0, tm1;

    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));

    RTCx->CTRL |= (1U << 2);              //
    while ((RTCx->CTRL & (1U << 2)) != 0) //wait ready
    {
        asm("nop");
    }

    tm0 = RTCx->TIM0;
    tm1 = RTCx->TIM1;
    rtc_time->year = ((tm1 >> 16) & 0x7f) + RTC_YEAR_BASE;
    rtc_time->mon = (tm1 >> 12) & 0x0f;
    rtc_time->day = (tm1 >> 4) & 0x1f;
    rtc_time->week = tm1 & 0x07;

    rtc_time->hour = (tm0 >> 16) & 0x1f;
    rtc_time->min = (tm0 >> 8) & 0x3f;
    rtc_time->sec = tm0 & 0x3f;
}

void rtc_set_alarm(RTC_TypeDef *RTCx, rtc_alarm_t *rtc_alarm)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    CHECK_PARAM(PARAM_RTC_Sec_RATE(rtc_alarm->sec));
    CHECK_PARAM(PARAM_RTC_Min_RATE(rtc_alarm->min));
    CHECK_PARAM(PARAM_RTC_Hour_RATE(rtc_alarm->hour));
    CHECK_PARAM(PARAM_RTC_Year_RATE((rtc_alarm->year - RTC_YEAR_BASE)));
    CHECK_PARAM(PARAM_RTC_Mon_RATE(rtc_alarm->mon));
    CHECK_PARAM(PARAM_RTC_Day_RATE(rtc_alarm->day));

    //    rt_kprintf("year %d mon %d day %d week %d hour %d min %d sec %d mask 0x%x\n",
    //        rtc_alarm->year,rtc_alarm->mon,rtc_alarm->day,rtc_alarm->week,rtc_alarm->hour,rtc_alarm->min,rtc_alarm->sec,rtc_alarm->mask);

    RTCx->AS0 = RTC_MAKE_HMS(rtc_alarm->hour, rtc_alarm->min, rtc_alarm->sec);
    RTCx->AS1 = RTC_MAKE_YMDW(rtc_alarm->year, rtc_alarm->mon, rtc_alarm->day, rtc_alarm->week);

    // RTCx->ACTRL = (RTCx->ACTRL & 0x7f) | rtc_alarm->mask;
    RTCx->ACTRL = (RTCx->ACTRL & 0xff00) | rtc_alarm->mask; //set rtc alarm mask

    RTCx->ACTRL |= (1U << 8); //enable rtc alarm
}

void rtc_get_alarm(RTC_TypeDef *RTCx, rtc_alarm_t *rtc_alarm)
{
    uint32_t as0, as1;

    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));

    as0 = RTCx->AS0;
    as1 = RTCx->AS1;
    rtc_alarm->year = ((as1 >> 16) & 0x7f) + RTC_YEAR_BASE;
    rtc_alarm->mon = (as1 >> 12) & 0x0f;
    rtc_alarm->day = (as1 >> 4) & 0x1f;
    rtc_alarm->week = as1 & 0x07;

    rtc_alarm->hour = (as0 >> 16) & 0x1f;
    rtc_alarm->min = (as0 >> 8) & 0x3f;
    rtc_alarm->sec = as0 & 0x3f;

    rtc_alarm->mask = RTCx->ACTRL & 0x7f;

    //    rt_kprintf("year %d mon %d day %d week %d hour %d min %d sec %d mask 0x%x\n",
    //        rtc_alarm->year,rtc_alarm->mon,rtc_alarm->day,rtc_alarm->week,rtc_alarm->hour,rtc_alarm->min,rtc_alarm->sec,rtc_alarm->mask);
}

void rtc_enable_alarm_interrupt(RTC_TypeDef *RTCx)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    RTCx->ACTRL |= BIT(12);
    IER |= (1U << 0);
    RTCx->ACTRL |= BIT(9);
}

void rtc_disable_alarm_interrupt(RTC_TypeDef *RTCx)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    IER &= ~(1U << 0);
    RTCx->ACTRL &= ~BIT(9);
}

void rtc_clear_alarm_pending(RTC_TypeDef *RTCx)
{
    CHECK_PARAM(PARAM_RTC_ADDR(RTCx));
    RTCx->ACTRL |= 1 << 12; //it will be done before clear pending
    ICP |= (1U << 0);       //clear RTC interrput pending
}

int is_leap_year(int year)
{
    if ((year & 0x03) == 0) //simplify for '2000~2099'
    {
        return 1; //true
    }

    return 0; //false
}

int get_month_day(int year, int mon)
{
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (is_leap_year(year) && (mon == 2))
    {
        return 29;
    }
    else
    {
        return days[mon - 1];
    }
}

int interval2time(int interval, rtc_time_t *rtc_time)
{
    int i;
    int delta;
    int total = 0;

    rtc_time->week = (rtc_time->week + ((interval + rtc_time->hour * SECOND_PER_HOUR + rtc_time->min * SECOND_PER_MINUTE + rtc_time->sec) / SECOND_PER_DAY)) % 7;

    for (i = 1; i < rtc_time->mon; i++)
    {
        total += get_month_day(rtc_time->year, i) * SECOND_PER_DAY;
    }

    total += (rtc_time->day - 1) * SECOND_PER_DAY + rtc_time->hour * SECOND_PER_HOUR + rtc_time->min * SECOND_PER_MINUTE + rtc_time->sec;
    interval += total;
    //calculate from xxxx-01-01 00:00:00, 'xxxx' is current year
    rtc_time->mon = 1;
    rtc_time->day = 1;
    rtc_time->hour = 0;
    rtc_time->min = 0;
    rtc_time->sec = 0;

    while (interval > 0)
    {
        delta = is_leap_year(rtc_time->year) ? SECOND_PER_LEAP_YEAR : SECOND_PER_NORMAL_YEAR;
        if (interval >= delta)
        {
            interval -= delta;
            rtc_time->year++;
        }
        else
        {
            break;
        }
    }

    for (i = 1; i < 12; i++)
    {
        delta = get_month_day(rtc_time->year, i) * SECOND_PER_DAY;
        if (interval >= delta)
        {
            interval -= delta;
            rtc_time->mon++;
        }
        else
        {
            break;
        }
    }

    rtc_time->day += interval / SECOND_PER_DAY;
    interval = interval % SECOND_PER_DAY;

    rtc_time->hour += interval / SECOND_PER_HOUR;
    interval = interval % SECOND_PER_HOUR;

    rtc_time->min += interval / SECOND_PER_MINUTE;
    rtc_time->sec += interval % SECOND_PER_MINUTE;

    if (rtc_time->week == 0)
    {
        rtc_time->week = 7; //due to rtc hardware
    }

    return 0;
}

static void afc_delay(int value)
{
    int i, j;
    for (i = 0; i < value; i++)
        for (j = 0; j < 100; j++)
        {
            asm("nop");
        }
}

#define AFC_CRT 0x3b0c04
#define AFC_SEC 0x3b0c08
#define AFC_HCY 0x3b0c0c

#define AFC_INT 0x3b0c20
#define AFC_SUB 0x3b0c1c
#define AFC_STA 0x3b0c24

#define AFC_RESET 0x1a10a038
#define POWER_LDO 0x1a10a028
#define PMC 0x1a10a000

static void afc_measure(int *data)
{
    //release cce reset
    *(volatile int *)(PMC) |= 0x1 << 15;
    // enable afc
    *(volatile int *)(AFC_RESET) |= 0x1 << 3;
    // set afc cont
    *(volatile int *)(AFC_HCY) = 375000;  //  96M / 128 / 32.768K
    // select 128 cycle
    *(volatile int *)(AFC_SEC) = 0x0;
    // reset afc
    *(volatile int *)(AFC_CRT) = 0x1;
    *(volatile int *)(AFC_CRT) = 0x0;
    // wait afc done
    afc_delay(1000);
    while (*(volatile int *)(AFC_STA) == 0)
    {
        asm("nop");
    }
    data[0] = *(volatile int *)(AFC_INT);
    data[1] = *(volatile int *)(AFC_SUB);
}

typedef struct
{
    unsigned int reserved1 : 10;
    unsigned int power_ldo : 8;
    unsigned int reserved2 : 14;
} PowerLdoReg_T, *PowerLdoReg_P;

void rtc_set_power_ldo(unsigned int power_ldo)
{
    volatile unsigned int *ptr = (volatile unsigned int *)(POWER_LDO);
    unsigned int reg_value = *ptr;
    PowerLdoReg_P reg_ptr = (PowerLdoReg_P)(&reg_value);

    reg_ptr->power_ldo = power_ldo;

    *ptr = reg_value;
}

// extern void state_set_dcxo_by_idx(unsigned char dcxo_idx);

void rtc_calibrate(void)
{
    init_puf(0);

    int data[2] = {0, 0};
    //    int i = 0 ;
    int left = 0;
    int right = 255;
    int mid = (left + right) / 2;
    int target[2] = {127, 127};
    int remainder[2] = {0, 2930};   // 2930 is 375000 / 128 = 2929.6875

    RC32K_PRINTF("rtc_calib start\r\n");
    RC32K_PRINTF("LDO = %x\r\n", *(volatile int *)(POWER_LDO));

    // state_set_dcxo_by_idx(32);

    do
    {
        //set mode
        *(volatile int *)(POWER_LDO) |= 1 << 9;
        //unsigned char temp = 128;
        //unsigned char base_count;

        rtc_set_power_ldo(mid);
        afc_delay(10);

        rt_kprintf("LDO = %x\r\n", *(volatile int *)(POWER_LDO));
        while (left < right)
        {
            afc_measure(data);
            // rt_kprintf("%d %d %d, data %d %d\r\n", left, mid, right, data[0], data[1]);

            if (127 == data[0] && data[1] > remainder[0])
            {
                target[0] = mid;
                remainder[0] = data[1];
            }
            if (128 == data[0] && data[1] < remainder[1])
            {
                target[1] = mid;
                remainder[1] = data[1];
            }

            if (data[0] >= 128)
            {
                left = mid + 1;
                mid = (left + right) / 2;
                rtc_set_power_ldo(mid);
                afc_delay(10);
            }
            else if (data[0] <= 127)
            {
                right = mid - 1;
                mid = (left + right) / 2;
                rtc_set_power_ldo(mid);
                afc_delay(10);
            }
        }

        // measure the target mid, need record this remainder
        afc_measure(data);
        // rt_kprintf("%d %d %d, data %d %d\r\n", left, mid, right, data[0], data[1]);

        if (127 == data[0] && data[1] > remainder[0])
        {
            target[0] = mid;
            remainder[0] = data[1];
        }
        if (128 == data[0] && data[1] < remainder[1])
        {
            target[1] = mid;
            remainder[1] = data[1];
        }

        if (remainder[1] < (2930 - remainder[0]))
        {
            mid = target[1];
        }
        else
        {
            mid = target[0];
        }

        RC32K_PRINTF("hello world\r\n");
        rtc_set_power_ldo(mid);
        // afc_delay(10);
        // afc_measure(data);  // in fact, no need to remeasure
        // rt_kprintf("%d data %d %d\r\n", mid, data[0], data[1]);

        // *(volatile int *)(POWER_LDO) &= ~(0xFF << 10);
        // *(volatile int *)(POWER_LDO) |= 1 << 16;
        // *(volatile int *)(POWER_LDO) |= 0x1F << 10;
        rt_kprintf("LDO 0x%x\r\n", *(volatile int *)(POWER_LDO));

        afc_delay(10000);
    } while (0);
    RC32K_PRINTF("rtc_calib over\r\n");
}
#endif