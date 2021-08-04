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

#ifndef _GPIO_H_
#define _GPIO_H_

#include <uc_pulpino.h>

typedef enum
{
    GPIO_PIN_0  = 0,
    GPIO_PIN_1  = 1,
    GPIO_PIN_2  = 2,
    GPIO_PIN_3  = 3,
    GPIO_PIN_4  = 4,
    GPIO_PIN_5  = 5,
    GPIO_PIN_6  = 6,
    GPIO_PIN_7  = 7,
    GPIO_PIN_8  = 8,
    GPIO_PIN_9  = 9,
    GPIO_PIN_10 = 10,
    GPIO_PIN_11 = 11,
    GPIO_PIN_12 = 12,
    GPIO_PIN_13 = 13,
    GPIO_PIN_14 = 14,
    GPIO_PIN_15 = 15,
    GPIO_PIN_16 = 16,
    GPIO_PIN_17 = 17,
    GPIO_PIN_18 = 18,
} GPIO_PIN; /* pin number */

typedef enum
{
    GPIO_FUNC_0 = 0, /* just normal gpio */
    GPIO_FUNC_1 = 1, /* the second multiplexing function */
    GPIO_FUNC_2 = 2, /* the third multiplexing function */
} GPIO_FUNCTION; /* multiplexing function */

typedef enum
{
    GPIO_DIR_OUT = 0,
    GPIO_DIR_IN  = 1,
} GPIO_DIRECTION; /* pin direction */

typedef enum
{
    GPIO_VALUE_LOW  = 0,
    GPIO_VALUE_HIGH = 1
} GPIO_VALUE;

typedef enum
{
    GPIO_IT_HIGH_LEVEL = 0x0,
    GPIO_IT_LOW_LEVEL  = 0x1,
    GPIO_IT_RISE_EDGE  = 0x2,
    GPIO_IT_FALL_EDGE  = 0x3
} GPIO_IRQ_TYPE;

typedef enum
{
    GPIO_PUPD_NONE = 0,
    GPIO_PUPD_UP   = 1
} GPIO_PUPD;

typedef struct
{
    GPIO_PIN       pin;  /* pin number */
    GPIO_FUNCTION  func; /* select multiplexing function */
    GPIO_PUPD      pupd; /* select pull-up or pull-none */
    GPIO_DIRECTION dir;  /* pin direction */
} GPIO_CFG_Type;


#define PARAM_GPIO(gpio)                   ((gpio==UC_GPIO))

#define PARAM_GPIO_CFG(gpio_cfg)           ((gpio_cfg==UC_GPIO_CFG))
#define PARAM_GPIO_PIN(pin)                ((pin<=GPIO_PIN_29)&&(pin>=GPIO_PIN_0))
#define PARAM_GPIO_FUNC(func)              ((func==GPIO_FUNC_0)||(func==GPIO_FUNC_2))
#define PARAM_GPIO_PUPD(pupd)          ((pupd==GPIO_PUPD_NONE)||(pupd==GPIO_PUPD_UP))
#define PARAM_GPIO_DIRECTION(direction)    ((direction==GPIO_DIR_IN)||(direction==GPIO_DIR_OUT))
#define PARAM_GPIO_IRQ(irq_type)           ((irq_type==GPIO_IT_HIGH_LEVEL)||(irq_type==GPIO_IT_LOW_LEVEL) \
                                            ||(irq_type==GPIO_IT_RISE_EDGE)||(irq_type==GPIO_IT_FALL_EDGE))


extern void gpio_init(GPIO_TypeDef* GPIO, GPIO_CFG_TypeDef* GPIO_CFG, GPIO_CFG_Type* gpio_cfg);

extern void gpio_set_pin_mux(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin, GPIO_FUNCTION func);
extern GPIO_FUNCTION gpio_get_pin_mux(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin);

extern void gpio_set_pin_pupd(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin, GPIO_PUPD pupd);
extern GPIO_PUPD gpio_get_pin_pupd(GPIO_CFG_TypeDef* GPIO_CFG, GPIO_PIN pin);

extern void gpio_set_pin_direction(GPIO_TypeDef* GPIO, GPIO_PIN pin, GPIO_DIRECTION dir);
extern GPIO_DIRECTION gpio_get_pin_direction(GPIO_TypeDef* GPIO, GPIO_PIN pin);

extern void gpio_set_pin_value(GPIO_TypeDef* GPIO, GPIO_PIN pin, GPIO_VALUE value);
extern GPIO_VALUE gpio_get_pin_value(GPIO_TypeDef* GPIO, GPIO_PIN pin);

extern void gpio_set_irq_type(GPIO_TypeDef* GPIO, GPIO_PIN pin, GPIO_IRQ_TYPE type);
extern void gpio_set_irq_en(GPIO_TypeDef* GPIO, GPIO_PIN pin, uint8_t enable);
extern uint32_t gpio_get_irq_status(GPIO_TypeDef* GPIO);

extern void gpio_int_enable(void);
extern void gpio_int_clear_pending(void);
extern void gpio_int_disable(void);

#endif // _GPIO_H_
