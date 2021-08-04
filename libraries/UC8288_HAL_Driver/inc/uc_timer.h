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

#ifndef _TIMER_H_
#define _TIMER_H_

#include "uc_pulpino.h"


#define TIMER_ENABLE_MASK 0x1

typedef struct
{
    uint8_t  pre;
    uint32_t cmp;
    uint32_t cnt;
} TIMER_CFG_Type;

typedef enum
{
    TIMER_IT_OVF = 0,
    TIMER_IT_CMP = 1
} TIMER_INT_TYPE;

#define PARAM_TIMER(TIMERx)                  ((TIMERx==UC_TIMER0)||(TIMERx==UC_TIMER1))

#define PARAM_TIMER_PRESCALER(pre)           ((pre>=0)&&(pre<=7))

#define PARAM_TIMER_IT_TYPE(it)              ((it==TIMER_IT_OVF)||(it==TIMER_IT_CMP))

extern void timer_init(TIMER_TYPE* TIMERx, TIMER_CFG_Type* cfg);
extern void timer_enable(TIMER_TYPE* TIMERx);
extern void timer_disable(TIMER_TYPE* TIMERx);
extern void timer_int_enable(TIMER_TYPE* TIMERx, TIMER_INT_TYPE it);
extern void timer_int_disable(TIMER_TYPE* TIMERx, TIMER_INT_TYPE it);
extern void timer_int_clear_pending(TIMER_TYPE* TIMERx, TIMER_INT_TYPE it);
extern void timer_set_count(TIMER_TYPE* TIMERx, uint32_t count);
extern uint32_t timer_get_count(TIMER_TYPE* TIMERx);
extern void timer_set_compare_value(TIMER_TYPE* TIMERx, uint32_t cmp);
extern void timer_set_prescaler_value(TIMER_TYPE* TIMERx, uint8_t pre);

#endif
