#ifndef __DRV_SPI_H_
#define __DRV_SPI_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "board.h"

rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, GPIO_PIN cs_gpio_pin);

int rt_hw_spi_init(void);

#endif /*__DRV_HARD_SPI_H_ */
