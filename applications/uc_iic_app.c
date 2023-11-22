#include <rtthread.h>
#ifdef _IIC_APP_
#include <rtdevice.h>
#include "uc_iic_app.h"
#include "drivers/i2c.h"

#define IIC_DEVICE_NAME    "i2c1"
#define MPU6050_ADDR 0x68

#define MPU6050_GYRO_OUT        0x43
#define MPU6050_ACC_OUT         0x3B

#define ADDRESS_WHO_AM_I (0x75U)          // !< WHO_AM_I register identifies the device. Expected value is 0x68.
#define ADDRESS_SIGNAL_PATH_RESET (0x68U) // !<

static const uint8_t expected_who_am_i = 0x68U; // !< Expected value to get from WHO_AM_I register.

static rt_device_t iic_dev = NULL;

rt_err_t mpu6050_register_write(uint8_t register_address, uint8_t value)
{
    rt_uint8_t buf[2];

    struct rt_i2c_msg msgs;

    buf[0] = register_address;
    buf[1] = value;

    msgs.addr = MPU6050_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = 2;

    if (rt_i2c_transfer((struct rt_i2c_bus_device*)iic_dev, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

rt_err_t mpu6050_register_read(uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes)
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

    if (rt_i2c_transfer((struct rt_i2c_bus_device*)iic_dev, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

rt_err_t mpu6050_verify_product_id(void)
{
    uint8_t who_am_i;

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

void MPU6050_ReadGyro(int16_t *pGYRO_X, int16_t *pGYRO_Y, int16_t *pGYRO_Z)
{
    uint8_t buf[6];

    mpu6050_register_read(MPU6050_GYRO_OUT, buf, 6);

    *pGYRO_X = (buf[0] << 8) | buf[1];
    if (*pGYRO_X & 0x8000)
        *pGYRO_X -= 65536;

    *pGYRO_Y = (buf[2] << 8) | buf[3];
    if (*pGYRO_Y & 0x8000)
        *pGYRO_Y -= 65536;

    *pGYRO_Z = (buf[4] << 8) | buf[5];
    if (*pGYRO_Z & 0x8000)
        *pGYRO_Z -= 65536;
}

void MPU6050_ReadAcc(int16_t *pACC_X, int16_t *pACC_Y, int16_t *pACC_Z)
{

    uint8_t buf[6];

    mpu6050_register_read(MPU6050_ACC_OUT, buf, 6);
    *pACC_X = (buf[0] << 8) | buf[1];
    if (*pACC_X & 0x8000)
        *pACC_X -= 65536;

    *pACC_Y = (buf[2] << 8) | buf[3];
    if (*pACC_Y & 0x8000)
        *pACC_Y -= 65536;

    *pACC_Z = (buf[4] << 8) | buf[5];
    if (*pACC_Z & 0x8000)
        *pACC_Z -= 65536;
}

rt_err_t mpu6050_init()
{
    rt_err_t transfer_succeeded = RT_EOK;

    uint8_t inData[7] = {0x00,  // 0x19
                         0x00,  // 0x1A
                         0x03,  // 0x6B
                         0x10,  // 0x1B
                         0x00,  // 0x6A
                         0x32,  // 0x37
                         0x01}; // 0x38
    uint8_t acc = 0x00;

    // Do a reset on signal paths
    uint8_t reset_value = 0x04U | 0x02U | 0x01U; // Resets gyro, accelerometer and temperature sensor signal paths
    transfer_succeeded &= mpu6050_register_write(ADDRESS_SIGNAL_PATH_RESET, reset_value);

    // 设置GYRO
    mpu6050_register_write(0x19, inData[0]); // 设置采样率    -- SMPLRT_DIV = 0  Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
    mpu6050_register_write(0x1A, inData[1]); // CONFIG        -- EXT_SYNC_SET 0 (禁用晶振输入脚) ; default DLPF_CFG = 0 => (低通滤波)ACC bandwidth = 260Hz  GYRO bandwidth = 256Hz)
    mpu6050_register_write(0x6B, inData[2]); // PWR_MGMT_1    -- SLEEP 0; CYCLE 0; TEMP_DIS 0; CLKSEL 3 (PLL with Z Gyro reference)
    mpu6050_register_write(0x1B, inData[3]); // gyro配置 量程  0-1000度每秒
    mpu6050_register_write(0x6A, inData[4]); // 0x6A的 I2C_MST_EN  设置成0  默认为0 6050  使能主IIC
    // 0x37的 推挽输出，高电平中断，一直输出高电平直到中断清除，任何读取操作都清除中断 使能 pass through 功能 直接在6050 读取5883数据
    mpu6050_register_write(0x37, inData[5]);
    mpu6050_register_write(0x38, inData[6]); // 使能data ready 引脚

    // 设置 ACC
    mpu6050_register_write(0x1C, acc); // ACC设置  量程 +-2G s

    // Read and verify product ID
    transfer_succeeded &= mpu6050_verify_product_id();

    return transfer_succeeded;
}

int iic_app_init(void)
{
    rt_err_t ret = RT_EOK;

    iic_dev = rt_device_find(IIC_DEVICE_NAME);
    if (!iic_dev)
    {
        rt_kprintf("find %s failed!\n", IIC_DEVICE_NAME);
        return RT_ERROR;
    }

    return ret;
}

void iic_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    
    rt_kprintf("iic test demo.\r\n");
    
    ret = iic_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init iic failed!\n");
        return;
    }

    ret = mpu6050_init();
    if (ret != RT_EOK) {
        rt_kprintf("mpu6050 init error\n");    
    }

    uint8_t id;
    int16_t AccValue[3],GyroValue[3];

    rt_kprintf("mpu6050 init ok\n");
    mpu6050_register_read(0x75U, &id, 1);
    rt_kprintf("mpu6050 id is %d \n",id);

    while (1) {
        MPU6050_ReadAcc( &AccValue[0], &AccValue[1] , &AccValue[2] );
        MPU6050_ReadGyro(&GyroValue[0] , &GyroValue[1] , &GyroValue[2] );

        rt_kprintf("ACC:  %d	%d	%d	\n",AccValue[0],AccValue[1],AccValue[2]);
        rt_kprintf("GYRO: %d	%d	%d	\n",GyroValue[0],GyroValue[1],GyroValue[2]);

        rt_thread_delay(100);
    }
}

#endif
