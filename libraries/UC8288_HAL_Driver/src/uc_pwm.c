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

#include "uc_pwm.h"

void pwm_enable(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    PWM->CTRL |= (1 << 0);
}

void pwm_disable(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    PWM->CTRL &= ~(1 << 0);
}

void pwm_set_period(PWM_TypeDef* PWM, int period_cnt)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    PWM->CNTMAX = period_cnt;
}

void pwm_set_duty(PWM_TypeDef* PWM, int duty_cnt)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    int max_cnt = PWM->CNTMAX;
    if (duty_cnt > max_cnt)
    { PWM->DUTY = max_cnt; }
    else
    { PWM->DUTY = duty_cnt; }
}

int pwm_get_period(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    return PWM->CNTMAX;
}

int pwm_get_duty(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    return PWM->DUTY;
}
