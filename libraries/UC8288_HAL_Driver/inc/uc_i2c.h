#ifndef _I2C_H_
#define _I2C_H_

#include <uc_pulpino.h>

#define I2C_PRESCALER_MASK    0xffff
#define I2C_RXDATA_MASK       0xff
#define I2C_ENABLE_MASK       0x80
#define I2C_BUSY_MASK         0x40
#define I2C_AL_MASK           0x20
#define I2C_TIP_MASK          0x02
#define I2C_IF_MASK           0x01
#define I2C_RXACK_MASK        0x80
#define I2C_CTR_EN            0x80 // enable only
#define I2C_STATUS_TIP        0x02
#define I2C_STATUS_RXACK      0x80

typedef enum
{
    Busy = 1,
    Idel = 0
} I2CStatus;

typedef enum
{
    SUCCESS = 1,
    ERROR_TIMEOUT = 0
} I2CACK;

typedef enum
{
    TRANSFER_DONE = 0,
    TRANSFER_ING = 1     //Transfer in progress
} I2CTXStatus;

typedef enum
{
    I2C_START = 0x80,
    I2C_STOP  = 0x40,
    I2C_READ  = 0x20,
    I2C_WRITE = 0x10,
    I2C_ACK = 0x08,
    I2C_CLR_INT = 0x01,
    I2C_START_READ = 0xA0,
    I2C_STOP_READ = 0x68,
    I2C_START_WRITE = 0x90,
    I2C_STOP_WRITE = 0x50
} I2C_CMD;

#define PARAM_I2C(i2c)                            ((i2c==UC_I2C))

#define PARAM_I2C_ENBIT(Enablebits)               ((Enablebits==ENABLE)||(Enablebits==DISABLE))
#define PARAM_I2C_TIMEOUT(I2C_TimeOut)            ((I2C_TimeOut>=0)&&(I2C_TimeOut<=0xfffff))
#define PARAM_I2C_TRANSFER_RATE(transfer_rate)    ((transfer_rate>=0)&&(transfer_rate<0xffff))

#define PARAM_I2C_CMD(cmd)  ((cmd==I2C_STOP_READ) || (cmd==I2C_START_WRITE ) \
                             || (cmd==I2C_START_READ) || (cmd==I2C_WRITE) || (cmd==I2C_CLR_INT) \
                             || (cmd==I2C_STOP) || (cmd==I2C_START) || (cmd==I2C_READ) \
                             || (cmd==I2C_STOP_WRITE))

typedef struct
{
    uint32_t            prescaler;
    uint32_t            I2C_TimeOut;
    FunctionalState     Enable;      /* parity bit enable */
} I2C_CFG_Type;

void i2c_setup(I2C_TYPE* I2C, I2C_CFG_Type* I2CconfigStruct);
void i2c_cmd(I2C_TYPE* I2C, FunctionalState NewState);
void i2c_send_command(I2C_TYPE* I2C, I2C_CMD cmd);
void i2c_send_data(I2C_TYPE* I2C, uint8_t data);
uint32_t i2c_get_status(I2C_TYPE* I2C);
I2CTXStatus i2c_get_txstatus(I2C_TYPE* I2C);
I2CACK i2c_get_ack(I2C_TYPE* I2C);
uint32_t i2c_get_data(I2C_TYPE* I2C);
I2CStatus i2c_busy(I2C_TYPE* I2C);



#endif // _I2C_H_
