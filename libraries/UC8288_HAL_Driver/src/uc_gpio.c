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

#include <uc_gpio.h>
#include "uc_event.h"

void gpio_init(GPIO_TypeDef* GPIO, GPIO_CFG_TypeDef* GPIO_CFG, GPIO_CFG_Type* gpio_cfg)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_CFG(GPIO_CFG));
    CHECK_PARAM(PARAM_GPIO_PIN(gpio_cfg->pin));
    CHECK_PARAM(PARAM_GPIO_FUNC(gpio_cfg->func));
    CHECK_PARAM(PARAM_GPIO_DIRECTION(gpio_cfg->dir));
    CHECK_PARAM(PARAM_GPIO_PUPD(gpio_cfg->pupd));

    //set pin mux
    if (gpio_cfg->func == GPIO_FUNC_1)
    { GPIO_CFG->PADMUX |= (1 << gpio_cfg->pin); }
    else
    { GPIO_CFG->PADMUX &= ~(1 << gpio_cfg->pin); }

    //set pin direction
    if (gpio_cfg->dir == GPIO_DIR_OUT)
    { GPIO->PADDIR &= ~(1 << gpio_cfg->pin); }
    else
    { GPIO->PADDIR |= (1 << gpio_cfg->pin); }

    //set pin pupd
    if (gpio_cfg->pupd == GPIO_PUPD_UP)
    { GPIO_CFG->PADCFG |= (1 << gpio_cfg->pin); }
    else
    { GPIO_CFG->PADCFG &= ~(1 << gpio_cfg->pin); }
}

void gpio_set_pin_mux(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin, GPIO_FUNCTION func)
{
    CHECK_PARAM(PARAM_GPIO_CFG(GPIO_CFG));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));
    CHECK_PARAM(PARAM_GPIO_FUNC(func));

    //set pin mux
    if (func == GPIO_FUNC_1)
    {
        GPIO_CFG->PADMUX |= (1 << pin);
        GPIO_CFG->PADMUX1 &= ~(1 << pin);
    }
    else if (func == GPIO_FUNC_2)
    {
        GPIO_CFG->PADMUX &= ~(1 << pin);
        GPIO_CFG->PADMUX1 |= (1 << pin);
    }
    else
    {
        GPIO_CFG->PADMUX &= ~(1 << pin);
        GPIO_CFG->PADMUX1 &= ~(1 << pin);
    }
}

GPIO_FUNCTION gpio_get_pin_mux(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin)
{
    CHECK_PARAM(PARAM_GPIO_CFG(GPIO_CFG));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));

    if  ((GPIO_CFG->PADMUX & (1 << pin)) != 0)
    {
        return GPIO_FUNC_1;
    }
    else if  ((GPIO_CFG->PADMUX1 & (1 << pin)) != 0)
    {
        return GPIO_FUNC_2;
    }
    else
    {
        return GPIO_FUNC_0;
    }
}

void gpio_set_pin_pupd(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin, GPIO_PUPD pupd)
{
    CHECK_PARAM(PARAM_GPIO_CFG(GPIO_CFG));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));
    CHECK_PARAM(PARAM_GPIO_PUPD(pupd));

    //set pin pupd
    if (pupd == GPIO_PUPD_UP)
    { GPIO_CFG->PADCFG |= (1 << pin); }
    else
    { GPIO_CFG->PADCFG &= ~(1 << pin); }
}

GPIO_PUPD gpio_get_pin_pupd(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin)
{
    CHECK_PARAM(PARAM_GPIO_CFG(GPIO_CFG));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));
    return ((GPIO_CFG->PADCFG >> pin) & 0x01);
}

void gpio_set_pin_direction(GPIO_TypeDef* GPIO, GPIO_PIN pin, GPIO_DIRECTION dir)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));
    CHECK_PARAM(PARAM_GPIO_DERECTION(dir));

    if (dir == GPIO_DIR_OUT)
    { GPIO->PADDIR &= ~(1 << pin); }
    else
    { GPIO->PADDIR |= 1 << pin; }
}

GPIO_DIRECTION gpio_get_pin_direction(GPIO_TypeDef* GPIO, GPIO_PIN pin)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));

    GPIO_DIRECTION dir = (GPIO->PADDIR >> pin) & 0x01;
    return dir;
}

void gpio_set_pin_value(GPIO_TypeDef* GPIO, GPIO_PIN pin, GPIO_VALUE value)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));

    if (value == GPIO_VALUE_LOW)
    { GPIO->PADOUT &= ~(1 << pin); }
    else
    { GPIO->PADOUT |= 1 << pin; }
}

GPIO_VALUE gpio_get_pin_value(GPIO_TypeDef* GPIO, GPIO_PIN pin)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));

    GPIO_VALUE value = 0;
    GPIO_DIRECTION dir = gpio_get_pin_direction(GPIO, pin);

    if (dir == GPIO_DIR_IN)
    { value = (GPIO->PADIN >> pin) & 0x01; }
    else
    { value = (GPIO->PADOUT >> pin) & 0x01; }

    return value;
}

void gpio_set_irq_en(GPIO_TypeDef* GPIO, GPIO_PIN pin, uint8_t enable)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));

    if (enable == 0)
    { GPIO->INTEN &= ~(1 << pin); }
    else
    { GPIO->INTEN |= 1 << pin; }
}

void gpio_set_irq_type(GPIO_TypeDef* GPIO, GPIO_PIN pin, GPIO_IRQ_TYPE type)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));
    CHECK_PARAM(PARAM_GPIO_PIN(pin));
    CHECK_PARAM(PARAM_GPIO_IRQ(type));

    if ((type & 0x1) == 0)
    { GPIO->INTTYPE0 &= ~(1 << pin); }
    else
    { GPIO->INTTYPE0 |= 1 << pin; }

    if ((type & 0x2) == 0)
    { GPIO->INTTYPE1 &= ~(1 << pin); }
    else
    { GPIO->INTTYPE1 |= 1 << pin; }
}

uint32_t gpio_get_irq_status(GPIO_TypeDef* GPIO)
{
    CHECK_PARAM(PARAM_GPIO(GPIO));

    return GPIO->INTSTATUS;
}

void gpio_int_enable(void)
{
    IER |= (1 << 25);//enable gpio interrupt
}

void gpio_int_clear_pending(void)
{
    ICP |= 1 << 25; //clear gpio interrupt pending
}

void gpio_int_disable(void)
{
    IER &= ~(1 << 25);//disable gpio interrupt
}
