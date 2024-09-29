#include <rtthread.h>

#ifdef APP_EXAMPLE_I2C

#ifndef RT_USING_I2C
#error "Please enable rt-thread i2c device driver"
#endif

#ifndef BSP_USING_I2C
#error "Please enable on-chip peripheral i2c config"
#endif

#include <rtdevice.h>
#include "uc_i2c_app.h"

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define I2C_DEVICE_NAME "i2c0"
#define I2C_BUS_CLOCK 100000 // 100kHz,只在使用硬件i2c时才有效

#define MPU6050_ADDR 0x68

#define MPU6050_GYRO_OUT 0x43
#define MPU6050_ACC_OUT 0x3B

#define ADDRESS_WHO_AM_I (0x75U)          // !< WHO_AM_I register identifies the device. Expected value is 0x68.
#define ADDRESS_SIGNAL_PATH_RESET (0x68U) // !<

static const rt_uint8_t expected_who_am_i = 0x68U; // !< Expected value to get from WHO_AM_I register.

static struct rt_i2c_bus_device *i2c_dev = RT_NULL;

static rt_err_t mpu6050_register_write(rt_uint8_t register_address, rt_uint8_t value)
{
    rt_uint8_t buf[2];

    struct rt_i2c_msg msgs;

    buf[0] = register_address;
    buf[1] = value;

    msgs.addr = MPU6050_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = 2;

    if (rt_i2c_transfer(i2c_dev, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t mpu6050_register_read(rt_uint8_t register_address, rt_uint8_t *destination, rt_uint8_t number_of_bytes)
{
    struct rt_i2c_msg msgs[2];

    rt_uint8_t buf[1];
    buf[0] = register_address;

    msgs[0].addr = MPU6050_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = buf;
    msgs[0].len = 1;

    msgs[1].addr = MPU6050_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = destination;
    msgs[1].len = number_of_bytes;

    if (rt_i2c_transfer(i2c_dev, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t mpu6050_verify_product_id(void)
{
    rt_uint8_t who_am_i;

    if (mpu6050_register_read(ADDRESS_WHO_AM_I, &who_am_i, 1) == RT_EOK)
    {
        if (who_am_i != expected_who_am_i)
        {
            return -RT_ERROR;
        }
        else
        {
            return RT_EOK;
        }
    }
    else
    {
        return -RT_ERROR;
    }
}

static void mpu6050_read_gyro(rt_int16_t *gyro_x, rt_int16_t *gyro_y, rt_int16_t *gyro_z)
{
    rt_uint8_t buf[6];

    mpu6050_register_read(MPU6050_GYRO_OUT, buf, 6);

    *gyro_x = (buf[0] << 8) | buf[1];
    if (*gyro_x & 0x8000)
        *gyro_x -= 65536;

    *gyro_y = (buf[2] << 8) | buf[3];
    if (*gyro_y & 0x8000)
        *gyro_y -= 65536;

    *gyro_z = (buf[4] << 8) | buf[5];
    if (*gyro_z & 0x8000)
        *gyro_z -= 65536;
}

static void mpu6050_read_acc(rt_int16_t *acc_x, rt_int16_t *acc_y, rt_int16_t *acc_z)
{
    rt_uint8_t buf[6];

    mpu6050_register_read(MPU6050_ACC_OUT, buf, 6);
    *acc_x = (buf[0] << 8) | buf[1];
    if (*acc_x & 0x8000)
        *acc_x -= 65536;

    *acc_y = (buf[2] << 8) | buf[3];
    if (*acc_y & 0x8000)
        *acc_y -= 65536;

    *acc_z = (buf[4] << 8) | buf[5];
    if (*acc_z & 0x8000)
        *acc_z -= 65536;
}

static rt_err_t mpu6050_init()
{
    rt_err_t transfer_succeeded = RT_EOK;

    // Do a reset on signal paths
    rt_uint8_t reset_value = 0x04U | 0x02U | 0x01U; // Resets gyro, accelerometer and temperature sensor signal paths
    transfer_succeeded |= mpu6050_register_write(ADDRESS_SIGNAL_PATH_RESET, reset_value);

    // 设置采样率    -- SMPLRT_DIV = 0  Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
    transfer_succeeded |= mpu6050_register_write(0x19, 0x00);
    // CONFIG        -- EXT_SYNC_SET 0 (禁用晶振输入脚) ; default DLPF_CFG = 0 => (低通滤波)ACC bandwidth = 260Hz  GYRO bandwidth = 256Hz)
    transfer_succeeded |= mpu6050_register_write(0x1A, 0x00);
    // PWR_MGMT_1    -- SLEEP 0; CYCLE 0; TEMP_DIS 0; CLKSEL 3 (PLL with Z Gyro reference)
    transfer_succeeded |= mpu6050_register_write(0x6B, 0x03);
    // gyro配置 量程  0-1000度每秒
    transfer_succeeded |= mpu6050_register_write(0x1B, 0x10);
    // 0x6A的 I2C_MST_EN  设置成0  默认为0 6050  使能主I2C
    transfer_succeeded |= mpu6050_register_write(0x6A, 0x00);
    // 0x37的 推挽输出，高电平中断，一直输出高电平直到中断清除，任何读取操作都清除中断 使能 pass through 功能 直接在6050 读取5883数据
    transfer_succeeded |= mpu6050_register_write(0x37, 0x32);
    // 使能data ready 引脚
    transfer_succeeded |= mpu6050_register_write(0x38, 0x01);

    // ACC设置  量程 +-2G s
    transfer_succeeded |= mpu6050_register_write(0x1C, 0x00);

    // Read and verify product ID
    transfer_succeeded |= mpu6050_verify_product_id();

    return transfer_succeeded;
}

static int i2c_app_init(void)
{
    rt_err_t ret = RT_EOK;

    i2c_dev = (struct rt_i2c_bus_device *)rt_device_find(I2C_DEVICE_NAME);
    if (!i2c_dev)
    {
        rt_kprintf("find %s failed!\n", I2C_DEVICE_NAME);
        return RT_ERROR;
    }

#ifndef BSP_USING_SOFT_I2C_PIN_GROUP

    ret = rt_i2c_control(i2c_dev, RT_I2C_DEV_CTRL_CLK, I2C_BUS_CLOCK);
    if (ret != RT_EOK)
    {
        rt_kprintf("set bus speed failed!\n");
        return RT_ERROR;
    }

#endif

    return ret;
}

static void i2c_thread_entry(void *parameter)
{
    rt_int16_t acc_value[3], gyro_value[3];
    while (1)
    {
        mpu6050_read_acc(&acc_value[0], &acc_value[1], &acc_value[2]);
        mpu6050_read_gyro(&gyro_value[0], &gyro_value[1], &gyro_value[2]);

        rt_kprintf("ACC:  %d	%d	%d	\n", acc_value[0], acc_value[1], acc_value[2]);
        rt_kprintf("GYRO: %d	%d	%d	\n", gyro_value[0], gyro_value[1], gyro_value[2]);

        rt_thread_delay(100);
    }
}

int i2c_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_thread_t thread = RT_NULL;

    rt_kprintf("i2c_app_sample\n");

    ret = i2c_app_init();
    if (ret != RT_EOK)
    {
        rt_kprintf("init i2c failed!\n");
        return -RT_ERROR;
    }

    ret = mpu6050_init();
    if (ret != RT_EOK)
    {
        rt_kprintf("mpu6050 init error\n");
        return -RT_ERROR;
    }

    rt_kprintf("mpu6050 init ok\n");

    /* 创建 serial 线程 */
    thread = rt_thread_create("i2c_app",
                              i2c_thread_entry,
                              RT_NULL,
                              THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_TIMESLICE);
    /* 创建成功则启动线程 */
    if (RT_NULL == thread)
    {
        return -RT_ERROR;
    }
    else
    {
        rt_thread_startup(thread);
    }

    return RT_EOK;
}

#endif // APP_EXAMPLE_I2C
