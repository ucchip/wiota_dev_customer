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

#ifndef _ADDA_H_
#define _ADDA_H_

#include <stdbool.h>
#include "uc_pulpino.h"

#define PARAM_CLK_DIV_RATE(clk_div)         (clk_div <= (0x7FFF) && clk_div >= (0x00))
#define PARAM_ELE_LV_RATE(ele_lv)           (ele_lv  <= (0x3FF)  && ele_lv  >= (0x00))
#define PARAM_DAC_WT_RATE(dac_wt)           (dac_wt  <= (0x3FF)  && dac_wt  >= (0x00))
#define PARAM_ADDC(adda)                     (adda    == UC_ADDA)

typedef enum
{
    ADC_CHANNEL_TEMP = 1 << 31, /* channel for temperature measure */
    ADC_CHANNEL_A = 1 << 30, /* channel a */
    ADC_CHANNEL_B = 1 << 29, /* channel b */
    ADC_CHANNEL_C = 1 << 28, /* channel c, optimized for audio */
    ADC_CHANNEL_BAT = 1 << 12, /* channel for battery voltage measure */
} ADC_CHANNEL;

typedef enum
{
    ADC_SR_360KSPS = 0,
    ADC_SR_180KSPS = 1,
    ADC_SR_90KSPS = 2,
    ADC_SR_45KSPS = 3,  // 1/512
} ADC_SAMPLE_RATE;

typedef enum
{
    ADC_TEMP_A1 = 1 << 26, /* environment temperature A1*/
    ADC_TEMP_A2 = 1 << 25, /* environment temperature A2*/
    ADC_TEMP_B = 1 << 24, /* body temperature */
    ADC_TEMP_C = 1 << 23, /* in-chip temperature */
} ADC_TEMP_SRC;

extern void adc_power_set(ADDA_TypeDef* ADDA);
//extern void temperature_set(ADDA_TypeDef *ADDA);
extern void adc_set_sample_rate(ADDA_TypeDef* ADDA, ADC_SAMPLE_RATE sample_rate);
extern void adc_channel_select(ADDA_TypeDef* ADDA, ADC_CHANNEL CHANNEL);
extern void adc_int_enable(ADDA_TypeDef* ADDA);
extern void adc_int_disable(ADDA_TypeDef* ADDA);
extern void adc_int_clear_pending(void);
extern void adc_wait_data_ready(ADDA_TypeDef* ADDA);
extern uint16_t adc_read(ADDA_TypeDef* ADDA);
extern void adc_watermark_set(ADDA_TypeDef* ADDA, uint8_t water_mark);
extern bool is_adc_fifo_over_watermark(ADDA_TypeDef* ADDA);
extern bool is_adc_fifo_empty(ADDA_TypeDef* ADDA);
extern void adc_fifo_clear(ADDA_TypeDef* ADDA);
extern void adc_vbat_measure_enable(bool enable);
extern void adc_temp_source_sel(ADDA_TypeDef* ADDA, ADC_TEMP_SRC temp_src);
extern void adc_temp_sensor_enable(ADDA_TypeDef* ADDA, bool enable);

extern void dac_power_set(ADDA_TypeDef* ADDA);
extern void dac_fifo_clear(ADDA_TypeDef* ADDA);;
extern void dac_watermark_set(ADDA_TypeDef* ADDA, uint8_t water_mark);
extern bool is_dac_fifo_over_watermark(ADDA_TypeDef* ADDA);
extern bool is_dac_fifo_full(ADDA_TypeDef* ADDA);
extern void dac_write(ADDA_TypeDef* ADDA, uint16_t wdata);
extern void dac_clkdiv_set(ADDA_TypeDef* ADDA, uint16_t clk_div);
extern void dac_int_enable(ADDA_TypeDef* ADDA);
extern void dac_int_disable(ADDA_TypeDef* ADDA);
extern void dac_int_clear_pending(void);

extern void auxdac_init(ADDA_TypeDef* ADDA);
extern void auxdac_level_set(ADDA_TypeDef* ADDA, uint16_t ele_level);

extern void internal_temp_measure(ADDA_TypeDef *ADDA);
extern void dc_off_control(int control);

#endif
