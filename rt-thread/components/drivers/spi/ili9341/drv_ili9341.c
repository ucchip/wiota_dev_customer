/*
 * driver for lcd(2.8 inch spi module msp2806, chip:ILI9341)
 *
 *
 * Change Logs:
 * Date           Author            Notes
 * 2021-05-22     luoguangqian      first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <drv_spi.h>
#include "littlevgl2rtt.h"
#include <drv_ili9341.h>

#define LCD_RESET_PIN           GET_PIN(F, 10)
#define LCD_BACKLIGHT_PIN       GET_PIN(F, 11)
#define LCD_RS_PIN              GET_PIN(F, 12)

#define LCD_SPI_FREQ_MAX        (45 * 1000 * 1000)

#define LCD_RS_SET  rt_pin_write(LCD_RS_PIN, PIN_LOW)       //设置为命令模式
#define LCD_RS_CLR  rt_pin_write(LCD_RS_PIN, PIN_HIGH)      //设置为非命令模式（数据模式）

#define LCD_RST_SET rt_pin_write(LCD_RESET_PIN, PIN_LOW);   //LCD reset使能
#define LCD_RST_CLR rt_pin_write(LCD_RESET_PIN, PIN_HIGH);  //LCD reset取消

#define LCD_BACKLIGHT_ON    rt_pin_write(LCD_BACKLIGHT_PIN, PIN_HIGH);      //点亮LCD背光
#define LCD_BACKLIGHT_OFF   rt_pin_write(LCD_BACKLIGHT_PIN, PIN_LOW);       //熄灭LCD背光


/* Level 1 Commands -------------- [section] Description */
#define ILI9341_NOP             0x00 /* [8.2.1 ] No Operation / Terminate Frame Memory Write */
#define ILI9341_SWRESET         0x01 /* [8.2.2 ] Software Reset */
#define ILI9341_RDDIDIF         0x04 /* [8.2.3 ] Read Display Identification Information */
#define ILI9341_RDDST           0x09 /* [8.2.4 ] Read Display Status */
#define ILI9341_RDDPM           0x0A /* [8.2.5 ] Read Display Power Mode */
#define ILI9341_RDDMADCTL       0x0B /* [8.2.6 ] Read Display MADCTL */
#define ILI9341_RDDCOLMOD       0x0C /* [8.2.7 ] Read Display Pixel Format */
#define ILI9341_RDDIM           0x0D /* [8.2.8 ] Read Display Image Mode */
#define ILI9341_RDDSM           0x0E /* [8.2.9 ] Read Display Signal Mode */
#define ILI9341_RDDSDR          0x0F /* [8.2.10] Read Display Self-Diagnostic Result */
#define ILI9341_SLPIN           0x10 /* [8.2.11] Enter Sleep Mode */
#define ILI9341_SLPOUT          0x11 /* [8.2.12] Leave Sleep Mode */
#define ILI9341_PTLON           0x12 /* [8.2.13] Partial Display Mode ON */
#define ILI9341_NORON           0x13 /* [8.2.14] Normal Display Mode ON */
#define ILI9341_DINVOFF         0x20 /* [8.2.15] Display Inversion OFF */
#define ILI9341_DINVON          0x21 /* [8.2.16] Display Inversion ON */
#define ILI9341_GAMSET          0x26 /* [8.2.17] Gamma Set */
#define ILI9341_DISPOFF         0x28 /* [8.2.18] Display OFF*/
#define ILI9341_DISPON          0x29 /* [8.2.19] Display ON*/
#define ILI9341_CASET           0x2A /* [8.2.20] Column Address Set */
#define ILI9341_PASET           0x2B /* [8.2.21] Page Address Set */
#define ILI9341_RAMWR           0x2C /* [8.2.22] Memory Write */
#define ILI9341_RGBSET          0x2D /* [8.2.23] Color Set (LUT for 16-bit to 18-bit color depth conversion) */
#define ILI9341_RAMRD           0x2E /* [8.2.24] Memory Read */
#define ILI9341_PTLAR           0x30 /* [8.2.25] Partial Area */
#define ILI9341_VSCRDEF         0x33 /* [8.2.26] Veritcal Scrolling Definition */
#define ILI9341_TEOFF           0x34 /* [8.2.27] Tearing Effect Line OFF */
#define ILI9341_TEON            0x35 /* [8.2.28] Tearing Effect Line ON */
#define ILI9341_MADCTL          0x36 /* [8.2.29] Memory Access Control */
#define     MADCTL_MY           0x80 /*          MY row address order */
#define     MADCTL_MX           0x40 /*          MX column address order */
#define     MADCTL_MV           0x20 /*          MV row / column exchange */
#define     MADCTL_ML           0x10 /*          ML vertical refresh order */
#define     MADCTL_MH           0x04 /*          MH horizontal refresh order */
#define     MADCTL_RGB          0x00 /*          RGB Order [default] */
#define     MADCTL_BGR          0x08 /*          BGR Order */
#define ILI9341_VSCRSADD        0x37 /* [8.2.30] Vertical Scrolling Start Address */
#define ILI9341_IDMOFF          0x38 /* [8.2.31] Idle Mode OFF */
#define ILI9341_IDMON           0x39 /* [8.2.32] Idle Mode ON */
#define ILI9341_PIXSET          0x3A /* [8.2.33] Pixel Format Set */
#define ILI9341_WRMEMCONT       0x3C /* [8.2.34] Write Memory Continue */
#define ILI9341_RDMEMCONT       0x3E /* [8.2.35] Read Memory Continue */
#define ILI9341_SETSCANTE       0x44 /* [8.2.36] Set Tear Scanline */
#define ILI9341_GETSCAN         0x45 /* [8.2.37] Get Scanline */
#define ILI9341_WRDISBV         0x51 /* [8.2.38] Write Display Brightness Value */
#define ILI9341_RDDISBV         0x52 /* [8.2.39] Read Display Brightness Value */
#define ILI9341_WRCTRLD         0x53 /* [8.2.40] Write Control Display */
#define ILI9341_RDCTRLD         0x54 /* [8.2.41] Read Control Display */
#define ILI9341_WRCABC          0x55 /* [8.2.42] Write Content Adaptive Brightness Control Value */
#define ILI9341_RDCABC          0x56 /* [8.2.43] Read Content Adaptive Brightness Control Value */
#define ILI9341_WRCABCMIN       0x5E /* [8.2.44] Write CABC Minimum Brightness */
#define ILI9341_RDCABCMIN       0x5F /* [8.2.45] Read CABC Minimum Brightness */
#define ILI9341_RDID1           0xDA /* [8.2.46] Read ID1 - Manufacturer ID (user) */
#define ILI9341_RDID2           0xDB /* [8.2.47] Read ID2 - Module/Driver version (supplier) */
#define ILI9341_RDID3           0xDC /* [8.2.48] Read ID3 - Module/Driver version (user) */

/* Level 2 Commands -------------- [section] Description */
#define ILI9341_IFMODE          0xB0 /* [8.3.1 ] Interface Mode Control */
#define ILI9341_FRMCTR1         0xB1 /* [8.3.2 ] Frame Rate Control (In Normal Mode/Full Colors) */
#define ILI9341_FRMCTR2         0xB2 /* [8.3.3 ] Frame Rate Control (In Idle Mode/8 colors) */
#define ILI9341_FRMCTR3         0xB3 /* [8.3.4 ] Frame Rate control (In Partial Mode/Full Colors) */
#define ILI9341_INVTR           0xB4 /* [8.3.5 ] Display Inversion Control */
#define ILI9341_PRCTR           0xB5 /* [8.3.6 ] Blanking Porch Control */
#define ILI9341_DISCTRL         0xB6 /* [8.3.7 ] Display Function Control */
#define ILI9341_ETMOD           0xB7 /* [8.3.8 ] Entry Mode Set */
#define ILI9341_BLCTRL1         0xB8 /* [8.3.9 ] Backlight Control 1 - Grayscale Histogram UI mode */
#define ILI9341_BLCTRL2         0xB9 /* [8.3.10] Backlight Control 2 - Grayscale Histogram still picture mode */
#define ILI9341_BLCTRL3         0xBA /* [8.3.11] Backlight Control 3 - Grayscale Thresholds UI mode */
#define ILI9341_BLCTRL4         0xBB /* [8.3.12] Backlight Control 4 - Grayscale Thresholds still picture mode */
#define ILI9341_BLCTRL5         0xBC /* [8.3.13] Backlight Control 5 - Brightness Transition time */
#define ILI9341_BLCTRL7         0xBE /* [8.3.14] Backlight Control 7 - PWM Frequency */
#define ILI9341_BLCTRL8         0xBF /* [8.3.15] Backlight Control 8 - ON/OFF + PWM Polarity*/
#define ILI9341_PWCTRL1         0xC0 /* [8.3.16] Power Control 1 - GVDD */
#define ILI9341_PWCTRL2         0xC1 /* [8.3.17] Power Control 2 - step-up factor for operating voltage */
#define ILI9341_VMCTRL1         0xC5 /* [8.3.18] VCOM Control 1 - Set VCOMH and VCOML */
#define ILI9341_VMCTRL2         0xC7 /* [8.3.19] VCOM Control 2 - VCOM offset voltage */
#define ILI9341_NVMWR           0xD0 /* [8.3.20] NV Memory Write */
#define ILI9341_NVMPKEY         0xD1 /* [8.3.21] NV Memory Protection Key */
#define ILI9341_RDNVM           0xD2 /* [8.3.22] NV Memory Status Read */
#define ILI9341_RDID4           0xD3 /* [8.3.23] Read ID4 - IC Device Code */
#define ILI9341_PGAMCTRL        0xE0 /* [8.3.24] Positive Gamma Control */
#define ILI9341_NGAMCTRL        0xE1 /* [8.3.25] Negative Gamma Correction */
#define ILI9341_DGAMCTRL1       0xE2 /* [8.3.26] Digital Gamma Control 1 */
#define ILI9341_DGAMCTRL2       0xE3 /* [8.3.27] Digital Gamma Control 2 */
#define ILI9341_IFCTL           0xF6 /* [8.3.28] 16bits Data Format Selection */

/* Extended Commands --------------- [section] Description*/
#define ILI9341_PWCTRLA         0xCB /* [8.4.1] Power control A */
#define ILI9341_PWCTRLB         0xCF /* [8.4.2] Power control B */
#define ILI9341_TIMECTRLA_INT   0xE8 /* [8.4.3] Internal Clock Driver timing control A */
#define ILI9341_TIMECTRLA_EXT   0xE9 /* [8.4.4] External Clock Driver timing control A */
#define ILI9341_TIMECTRLB       0xEA /* [8.4.5] Driver timing control B (gate driver timing control) */
#define ILI9341_PWSEQCTRL       0xED /* [8.4.6] Power on sequence control */
#define ILI9341_GAM3CTRL        0xF2 /* [8.4.7] Enable 3 gamma control */
#define ILI9341_PUMPRATIO       0xF7 /* [8.4.8] Pump ratio control */

/* config */
#define ILI9341_GAMMA           1
#define ILI9341_TEARING         0

static uint16_t lcd_height = LCD_HEIGHT;
static uint16_t lcd_width  = LCD_WIDTH;
struct rt_semaphore lcd_lock;
static struct rt_spi_device* lcd_dev = RT_NULL;

static void lcd_gpio_init(void)
{
    rt_pin_mode(LCD_RESET_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_BACKLIGHT_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_RS_PIN, PIN_MODE_OUTPUT);

    LCD_RST_SET;
    LCD_BACKLIGHT_OFF;
}

static inline void lcd_reset(void)
{
    LCD_RST_SET;
    rt_thread_mdelay(10);
    LCD_RST_CLR;
    rt_thread_mdelay(10);
}

static inline void lcd_write_cmd(rt_uint8_t cmd)
{
    LCD_RS_SET;
    rt_spi_transfer(lcd_dev, &cmd, RT_NULL, 1);
}

static inline void lcd_write_data(const void* data, rt_size_t length)
{
    LCD_RS_CLR;
    rt_spi_transfer(lcd_dev, data, RT_NULL, length);
}

static inline void lcd_write_data_1_byte(rt_uint8_t data)
{
    lcd_write_data(&data, 1);
}

static inline void lcd_write_data_1_pixel(rt_uint16_t color)
{
    rt_uint8_t send_data[2];

    send_data[1] = color & 0x00FF;
    send_data[0] = (color >> 8) & 0x00FF;
    lcd_write_data(send_data, 2);
}

static inline void lcd_write_pixel(const void* data, rt_size_t length)
{
    rt_uint16_t (* color)[1];

    color = (rt_uint16_t (*)[])data;
    for (rt_uint32_t i = 0; i < length; i++)
    {
        lcd_write_data_1_pixel(*color[i]);
    }
}

/*Set rotation of the screen - changes x0 and y0*/
static inline void lcd_set_rotation(uint8_t degrees)
{
    lcd_write_cmd(ILI9341_MADCTL);

    switch (degrees)
    {
        case ROTATE_0_DEGREES:
            lcd_write_data_1_byte(MADCTL_BGR | MADCTL_MX | MADCTL_MV | MADCTL_MY);
            lcd_width  = LCD_WIDTH;
            lcd_height = LCD_HEIGHT;
            break;
        case ROTATE_90_DEGREES:
            lcd_write_data_1_byte(MADCTL_BGR | MADCTL_MY);
            lcd_width  = LCD_HEIGHT;
            lcd_height = LCD_WIDTH;
            break;
        case ROTATE_180_DEGREES:
            lcd_write_data_1_byte(MADCTL_BGR | MADCTL_MV);
            lcd_width  = LCD_WIDTH;
            lcd_height = LCD_HEIGHT;
            break;
        case ROTATE_270_DEGREES:
            lcd_write_data_1_byte(MADCTL_BGR | MADCTL_MX);
            lcd_width  = LCD_HEIGHT;
            lcd_height = LCD_WIDTH;
            break;
        default:
            return;     //EXIT IF SCREEN ROTATION NOT VALID!
    }

    lcd_write_cmd(ILI9341_CASET);
    lcd_write_data_1_byte(0x00);
    lcd_write_data_1_byte(0x00);
    lcd_write_data_1_byte(lcd_width >> 8);
    lcd_write_data_1_byte((lcd_width & 0xFF) - 1);

    lcd_write_cmd(ILI9341_PASET);
    lcd_write_data_1_byte(0x00);
    lcd_write_data_1_byte(0x00);
    lcd_write_data_1_byte(lcd_height >> 8);
    lcd_write_data_1_byte((lcd_height & 0xFF) - 1);
}

static void lcd_set_rect(uint16_t x_star, uint16_t y_star, uint16_t x_end, uint16_t y_end)
{
    lcd_write_cmd(ILI9341_CASET);
    lcd_write_data_1_byte(x_star >> 8);
    lcd_write_data_1_byte(0xFF & x_star);
    lcd_write_data_1_byte(x_end >> 8);
    lcd_write_data_1_byte(0xFF & x_end);

    lcd_write_cmd(ILI9341_PASET);
    lcd_write_data_1_byte(y_star >> 8);
    lcd_write_data_1_byte(0xFF & y_star);
    lcd_write_data_1_byte(y_end >> 8);
    lcd_write_data_1_byte(0xFF & y_end);

    lcd_write_cmd(ILI9341_RAMWR);
}

static void lcd_flush(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2, void* data)
{
    lcd_set_rect(x1, y1, x2, y2);
    lcd_write_data(data, (x2 - x1 + 1) * (y2 - y1 + 1) * 2);
}

RT_UNUSED static void lcd_clear(uint16_t color)
{
    uint16_t i, j;

    lcd_set_rect(0, 0, lcd_width - 1, lcd_height - 1);

    for (i = 0; i < lcd_width; i++)
    {
        for (j = 0; j < lcd_height; j++)
        {
            lcd_write_data_1_pixel(color);
        }
    }
}

void lcd_on(rt_uint8_t on)
{
    if (on)
    {
        lcd_write_cmd(ILI9341_DISPON);
        rt_thread_mdelay(20);
        LCD_BACKLIGHT_ON;
    }
    else
    {
        lcd_write_cmd(ILI9341_DISPOFF);
        rt_thread_mdelay(20);
        LCD_BACKLIGHT_OFF;
    }
}

static void lcd_init(void)
{
    uint8_t data[15];

    /* init lcd pin */
    lcd_gpio_init();

    /* reset lcd */
    lcd_reset();

    /* startup sequence */
    lcd_write_cmd(ILI9341_PWCTRLB);
    data[0] = 0x00;
    data[1] = 0x83;
    data[2] = 0x30;
    lcd_write_data(data, 3);

    lcd_write_cmd(ILI9341_PWSEQCTRL);
    data[0] = 0x64;
    data[1] = 0x03;
    data[2] = 0x12;
    data[3] = 0x81;
    lcd_write_data(data, 4);

    lcd_write_cmd(ILI9341_TIMECTRLA_INT);
    data[0] = 0x85;
    data[1] = 0x01;
    data[2] = 0x79;
    lcd_write_data(data, 3);

    lcd_write_cmd(ILI9341_PWCTRLA);
    data[0] = 0x39;
    data[1] = 0x2c;
    data[2] = 0x00;
    data[3] = 0x34;
    data[4] = 0x02;
    lcd_write_data(data, 5);

    lcd_write_cmd(ILI9341_PUMPRATIO);
    data[0] = 0x20;
    lcd_write_data(data, 1);

    lcd_write_cmd(ILI9341_TIMECTRLB);
    data[0] = 0x00;
    data[1] = 0x00;
    lcd_write_data(data, 2);

    /* power control */
    lcd_write_cmd(ILI9341_PWCTRL1);
    data[0] = 0x26;
    lcd_write_data(data, 1);

    lcd_write_cmd(ILI9341_PWCTRL2);
    data[0] = 0x11;
    lcd_write_data(data, 1);

    /* VCOM */
    lcd_write_cmd(ILI9341_VMCTRL1);
    data[0] = 0x35;
    data[1] = 0x3e;
    lcd_write_data(data, 2);

    lcd_write_cmd(ILI9341_VMCTRL2);
    data[0] = 0xbe;
    lcd_write_data(data, 1);

    /* set orientation */
    lcd_set_rotation(ROTATE_0_DEGREES);

    /* 16 bit pixel */
    lcd_write_cmd(ILI9341_PIXSET);
    data[0] = 0x55;
    lcd_write_data(data, 1);

    /* frame rate */
    lcd_write_cmd(ILI9341_FRMCTR1);
    data[0] = 0x00;
    data[1] = 0x1b;
    lcd_write_data(data, 2);

#if ILI9341_GAMMA
    /* gamma curve set */
    lcd_write_cmd(ILI9341_GAMSET);
    lcd_write_cmd(0x01);

    /* positive gamma correction */
    lcd_write_cmd(ILI9341_PGAMCTRL);
    data[0]  = 0x1f;
    data[1]  = 0x1a;
    data[2]  = 0x18;
    data[3]  = 0x0a;
    data[4]  = 0x0f;
    data[5]  = 0x06;
    data[6]  = 0x45;
    data[7]  = 0x87;
    data[8]  = 0x32;
    data[9]  = 0x0a;
    data[10] = 0x07;
    data[11] = 0x02;
    data[12] = 0x07;
    data[13] = 0x05;
    data[14] = 0x00;
    lcd_write_data(data, 15);

    /* negative gamma correction */
    lcd_write_cmd(ILI9341_NGAMCTRL);
    data[0]  = 0x00;
    data[1]  = 0x25;
    data[2]  = 0x27;
    data[3]  = 0x05;
    data[4]  = 0x10;
    data[5]  = 0x09;
    data[6]  = 0x3a;
    data[7]  = 0x78;
    data[8]  = 0x4d;
    data[9]  = 0x05;
    data[10] = 0x18;
    data[11] = 0x0d;
    data[12] = 0x38;
    data[13] = 0x3a;
    data[14] = 0x1f;
    lcd_write_data(data, 15);
#endif

    //lcd_write_cmd(ILI9341_RAMWR);

#if ILI9341_TEARING
    /* tearing effect off */
    lcd_write_cmd(ILI9341_TEOFF);

    /* tearing effect on */
    lcd_write_cmd(ILI9341_TEON);
#endif

    /* entry mode set */
    lcd_write_cmd(ILI9341_ETMOD);
    lcd_write_cmd(0x07);

    /* display function control */
    lcd_write_cmd(ILI9341_DISCTRL);
    data[0] = 0x0a;
    data[1] = 0x82;
    data[2] = 0x27;
    data[3] = 0x00;
    lcd_write_data(data, 4);

    //lcd_clear(0xFFFF);

    /* exit sleep mode */
    lcd_write_cmd(ILI9341_SLPOUT);

    rt_thread_mdelay(100);

    /* set display off */
    lcd_on(0);
}

static rt_err_t ili9341_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t ili9341_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t ili9341_read(rt_device_t dev, rt_off_t pos, void* buf, rt_size_t size)
{
    return 0;
}

static rt_size_t ili9341_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    return 0;
}

rt_err_t ili9341_control(rt_device_t dev, int cmd, void* args)
{
    rt_sem_take(&lcd_lock, RT_WAITING_FOREVER);

    switch (cmd)
    {
        case RTGRAPHIC_CTRL_RECT_UPDATE:
        {
            rect_fill_t* info = (rect_fill_t*)args;
            lcd_flush(info->rect.x, info->rect.y, info->rect.x + info->rect.width - 1, info->rect.y + info->rect.height - 1, info->data);
            break;
        }
        case RTGRAPHIC_CTRL_GET_INFO:
        {
            struct rt_device_graphic_info* info = (struct rt_device_graphic_info*)args;

            RT_ASSERT(info != RT_NULL);
            info->pixel_format  = RTGRAPHIC_PIXEL_FORMAT_RGB565;
            info->bits_per_pixel = 16;
            info->width         = lcd_width;
            info->height        = lcd_height;
            info->framebuffer   = (void*)RT_NULL;
            break;
        }
        case RTGRAPHIC_CTRL_POWERON:
            lcd_on(1);
            break;
        case RTGRAPHIC_CTRL_POWEROFF:
            lcd_on(0);
            break;
    }

    rt_sem_release(&lcd_lock);

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops ili9341_ops =
{
    RT_NULL,
    ili9341_open,
    ili9341_close,
    ili9341_read,
    ili9341_write,
    ili9341_control,
};
#endif

static void blit_line(const char* pixel, int x, int y, rt_size_t size)
{
    lcd_set_rect(x, y, x + size - 1, y);
    lcd_write_data(pixel, size * 2);
}

static struct rt_device_graphic_ops graphic_ops =
{
    .blit_line = blit_line,
};

static int hw_ili9341_lcd_init(void)
{
    struct rt_device* device;

    device = rt_malloc(sizeof(struct rt_device));
    if (device == RT_NULL)
    {
        return -1;
    }
    rt_memset(device, 0, sizeof(struct rt_device));

    __HAL_RCC_GPIOF_CLK_ENABLE();
    rt_hw_spi_device_attach("spi5", LCD_USING_SPI_NAME, GPIOF, GPIO_PIN_6);
    lcd_dev = (struct rt_spi_device*)rt_device_find(LCD_USING_SPI_NAME);
    if (!lcd_dev)
    {
        rt_kprintf("no %s!\n", LCD_USING_SPI_NAME);
    }
    else
    {
        struct rt_spi_configuration cfg;

        cfg.data_width = 8;
        cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
        cfg.max_hz = LCD_SPI_FREQ_MAX;
        rt_spi_configure(lcd_dev, &cfg);
    }

    lcd_init();

    rt_sem_init(&lcd_lock, "lcd_flush_lock", 1, RT_IPC_FLAG_FIFO);
    /* set device type */
    device->type = RT_Device_Class_Graphic;
    /* initialize device interface */
#ifdef RT_USING_DEVICE_OPS
    device->ops = &ili9341_ops;
#else
    device->init = RT_NULL;
    device->open = ili9341_open;
    device->close = ili9341_close;
    device->read = ili9341_read;
    device->write = ili9341_write;
    device->control = ili9341_control;
    device->user_data = &graphic_ops;
#endif
    /* register to device manager */
    rt_device_register(device, "lcd", RT_DEVICE_FLAG_RDWR);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(hw_ili9341_lcd_init);

