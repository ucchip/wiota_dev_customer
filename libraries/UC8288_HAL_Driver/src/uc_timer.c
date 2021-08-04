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

#include "uc_timer.h"
#include "uc_event.h"

void timer_init(TIMER_TYPE* TIMERx, TIMER_CFG_Type* cfg)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    CHECK_PARAM(PARAM_TIMER_PRESCALER(cfg->pre));

    TIMERx->CTR |= (cfg->pre << 3);
    TIMERx->CMP = cfg->cmp;
    TIMERx->TRR = cfg->cnt;
}

void timer_enable(TIMER_TYPE* TIMERx)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));

    TIMERx->CTR |= (TIMER_ENABLE_MASK);
}

void timer_disable(TIMER_TYPE* TIMERx)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));

    TIMERx->CTR &= ~(TIMER_ENABLE_MASK);
}

void timer_int_enable(TIMER_TYPE* TIMERx, TIMER_INT_TYPE it)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    CHECK_PARAM(PARAM_TIMER_IT_TYPE(it));
    if (TIMERx == UC_TIMER0)
    {
        if (it == TIMER_IT_OVF)
        {
            IER |= (1 << 28);
        }
        else
        {
            IER |= (1 << 29);
        }
    }
    else
    {
        if (it == TIMER_IT_OVF)
        {
            IER |= (1 << 30);
        }
        else
        {
            IER |= (1 << 31);
        }
    }
}

void timer_int_disable(TIMER_TYPE* TIMERx, TIMER_INT_TYPE it)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    CHECK_PARAM(PARAM_TIMER_IT_TYPE(it));
    if (TIMERx == UC_TIMER0)
    {
        if (it == TIMER_IT_OVF)
        {
            IER &= ~(1 << 28);
        }
        else
        {
            IER &= ~(1 << 29);
        }
    }
    else
    {
        if (it == TIMER_IT_OVF)
        {
            IER &= ~(1 << 30);
        }
        else
        {
            IER &= ~(1 << 31);
        }
    }
}

void timer_int_clear_pending(TIMER_TYPE* TIMERx, TIMER_INT_TYPE it)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    CHECK_PARAM(PARAM_TIMER_IT_TYPE(it));
    if (TIMERx == UC_TIMER0)
    {
        if (it == TIMER_IT_OVF)
        {
            ICP |= (1 << 28);
        }
        else
        {
            ICP |= (1 << 29);
        }
    }
    else
    {
        if (it == TIMER_IT_OVF)
        {
            ICP |= (1 << 30);
        }
        else
        {
            ICP |= (1 << 31);
        }
    }
}

void timer_set_count(TIMER_TYPE* TIMERx, uint32_t count)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));

    TIMERx->TRR = count;
}

uint32_t timer_get_count(TIMER_TYPE* TIMERx)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));

    return TIMERx->TRR;
}

void timer_set_compare_value(TIMER_TYPE* TIMERx, uint32_t cmp)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));

    TIMERx->CMP = cmp;
}

void timer_set_prescaler_value(TIMER_TYPE* TIMERx, uint8_t pre)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));

    if (pre > 0x07)
    {
        pre = 0x07;
    }

    TIMERx->CTR &= (0x07 << 3);
    TIMERx->CTR |= (pre << 3);
}