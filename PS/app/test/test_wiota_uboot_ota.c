/******************************************************************************
 * @file      test_wiota_uboot_ota.c
 * @brief     Simulate OTA upgrade
 * @author    ypzhang
 * @version   1.0
 * @date      2023.12.20
 *
 * @copyright Copyright (c) 2018 UCchip Technology Co.,Ltd. All rights reserved.
 *
 ******************************************************************************/
#include "uc_wiota_api.h"

#ifdef WIOTA_IOTE_UBOOT_TEST
#define FLASH_ERASE_SIZE (4096)
#define FLASH_WRITE_LEN (256)
// #define UBOOT_OTA_UPDATE_ASYNC // 是异步ota升级
#define BOOT_RUN_OTA_ONLY_UPDATE 'b'

#ifdef UBOOT_OTA_UPDATE_ASYNC
/**
 * @brief    异步接收开关控制
 * @param[IN]    enable 0 关闭, 1打开
 * @return int   0 正常, 1 错误
 */
static int wiota_async_recv_ctrl(int enable)
{
    int count = 0;
    if (enable)
    {
        uc_wiota_set_recv_mode(UC_AUTO_RECV_START);
    }
    else
    {
        uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);
        rt_thread_delay(uc_wiota_get_subframe_len() >> 1);

        while ((uc_wiota_get_physical_status() != RF_STATUS_IDLE) || (uc_wiota_get_state() != UC_STATUS_NULL))
        {
            count++;
            if (count > 200)
            {
                // 等待超时
                return 1;
            }
            rt_thread_delay(5);
        }
    }
    return 0;
}
#endif // UBOOT_OTA_UPDATE_ASYNC

void uboot_reflash_test(void)
{
    int i;
    int bin_size;
    int reserved_size;
    int ota_size;
    int ota_data_size;
    int write_count = FLASH_WRITE_LEN;
    // OTA分区首地址
    int ota_addr;
    // 模拟的数据文件
    // 05 00 00 1B 01 00 70 00 00 00 00 0F FF 01 00 00 00 10 F2 00 00 70 04 E0 04 6F F9 文件.patch里的顺序
    u8_t ota_data[28] = {0x1B, 0x00, 0x00, 0x05,
                         0x00, 0x70, 0x00, 0x01,
                         0x0F, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x01, 0xFF,
                         0x00, 0xF2, 0x10, 0x00,
                         0xE0, 0x04, 0x70, 0x00,
                         0x00, 0xF9, 0x6F, 0x04}; // 0X00补齐

    ota_data_size = sizeof(ota_data) - 1;

    get_partition_size(&bin_size, &reserved_size, &ota_size);
    ota_addr = bin_size + reserved_size;
    rt_kprintf("bin_size = %d , reserved_size = %d, ota_size = %d, ota_addr = %d\n", bin_size, reserved_size, ota_size, ota_addr);
    // bin_size = 307200 (4B000) , reserved_size = 147456 (24000), ota_size = 61440 (F000), ota_addr = 454656 (6F000)

    if (ota_data_size > ota_size)
    {
        rt_kprintf("ota_data_size > ota_size!!! return");
        return;
    }

#ifdef UBOOT_OTA_UPDATE_ASYNC
    // 异步接收关闭
    wiota_async_recv_ctrl(0);
#else
    // 同步接收关闭
    uc_wiota_suspend_connect();
    rt_thread_mdelay(uc_wiota_get_frame_len() / 1000 + 2);
#endif // UBOOT_OTA_UPDATE_ASYNC

    // 先擦再写
    for (i = 0; i < ota_size; i += FLASH_ERASE_SIZE)
    {
        uc_wiota_flash_erase_4K(ota_addr + i);
    }
    // 写OTA数据
    for (i = 0; i < ota_data_size;)
    {
        uc_wiota_flash_write(&ota_data[i], ota_addr + i, write_count);
        i += write_count;
        write_count = ((ota_data_size - i) >= (int)FLASH_WRITE_LEN) ? FLASH_WRITE_LEN : (ota_data_size - i);
    }

    // 设置OTA文件大小
    set_download_file_size(ota_data_size);
    // 设置uboot工作模式
    set_uboot_mode(BOOT_RUN_OTA_ONLY_UPDATE);

#ifdef UBOOT_OTA_UPDATE_ASYNC
    // 异步接收打开
    wiota_async_recv_ctrl(1);
#else
    // 同步接收打开
    uc_wiota_recover_connect();
#endif // UBOOT_OTA_UPDATE_ASYNC

    // 关闭相关中断
    rt_hw_interrupt_disable();
    // 重启
    boot_riscv_reboot();
}
#endif // WIOTA_IOTE_UBOOT_TEST