// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <uc_i2c.h>
#include "uc_gpio.h"
#include "board.h"

void i2c_setup(I2C_TYPE* I2C, I2C_CFG_Type* I2CconfigStruct)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    CHECK_PARAM(PARAM_I2C_TRANSFER_RATE(I2CconfigStruct->prescaler));

    // GPIO 5 / 6  OR GPIO4 / 14
    #ifdef BSP_USING_HW_I2C1
    gpio_set_pin_mux(UC_GPIO_CFG, BSP_HW_I2C1_SCL_PIN, GPIO_FUNC_1);
    gpio_set_pin_pupd(UC_GPIO_CFG, BSP_HW_I2C1_SCL_PIN, GPIO_PUPD_UP);
    gpio_set_pin_mux(UC_GPIO_CFG, BSP_HW_I2C1_SDA_PIN, GPIO_FUNC_1);
    gpio_set_pin_pupd(UC_GPIO_CFG, BSP_HW_I2C1_SDA_PIN, GPIO_PUPD_UP);
    #elif defined(BSP_USING_HW_I2C2)
    gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_4, GPIO_FUNC_2);
    gpio_set_pin_pupd(UC_GPIO_CFG, GPIO_PIN_4, GPIO_PUPD_UP);
    gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_14, GPIO_FUNC_2);
    gpio_set_pin_pupd(UC_GPIO_CFG, GPIO_PIN_14, GPIO_PUPD_UP);
    #endif

    I2C->CPR = I2CconfigStruct->prescaler & I2C_PRESCALER_MASK;

    I2C->CTR |= (I2C_ENABLE_MASK);
}

void i2c_cmd(I2C_TYPE* I2C, FunctionalState NewState)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    CHECK_PARAM(PARAM_I2C_ENBIT(NewState));

    if (NewState)
    {
        I2C->CTR |= (I2C_ENABLE_MASK);
    }
    else
    {
        I2C->CTR &= ~(I2C_ENABLE_MASK);
    }
}

void i2c_send_command(I2C_TYPE* I2C, I2C_CMD cmd)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    CHECK_PARAM(PARAM_I2C_CMD(cmd));

    I2C->CDR = cmd;
    //while((I2C->STR & I2C_STATUS_TIP) != 0);
}

void i2c_send_data(I2C_TYPE* I2C, uint8_t data)
{
    CHECK_PARAM(PARAM_I2C(I2C));

    I2C->TXR = data;
}

uint32_t i2c_get_status(I2C_TYPE* I2C)
{
    uint32_t temreg;

    CHECK_PARAM(PARAM_I2C(I2C));

    temreg = I2C->STR;

    return temreg;
}

I2CTXStatus i2c_get_txstatus(I2C_TYPE* I2C)
{
    I2CTXStatus temstatus;

    CHECK_PARAM(PARAM_I2C(I2C));

    temstatus = (I2CTXStatus)(I2C->STR & I2C_TIP_MASK);

    return temstatus;
}

I2CACK i2c_get_ack(I2C_TYPE* I2C)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    while ((I2C->STR & I2C_STATUS_TIP) == 0); // need TIP go to 1
    while ((I2C->STR & I2C_STATUS_TIP) != 0); // and then go back to 0

    return !(I2C->STR & I2C_STATUS_RXACK);// invert since signal is active low
}

uint32_t i2c_get_data(I2C_TYPE* I2C)
{
    CHECK_PARAM(PARAM_I2C(I2C));

    return (I2C->RXR & 0xff);
}

I2CStatus i2c_busy(I2C_TYPE* I2C)
{
    CHECK_PARAM(PARAM_I2C(I2C));

    return ((I2C->STR & I2C_BUSY_MASK) == I2C_BUSY_MASK);
}
