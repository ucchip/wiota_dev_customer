#include <drv_gpio.h>

#ifdef RT_USING_PIN

#if !defined(BSP_USING_GPIO)
#error "Please define at least one BSP_USING_GPIO"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

#include <rthw.h>

static struct rt_pin_irq_hdr pin_irq_hdr_tab[] = {
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
};
static uint32_t pin_irq_enable_mask = 0;

#define ITEM_NUM(items) sizeof(items) / sizeof(items[0])

static void uc8x88_pin_write(rt_device_t dev, rt_base_t pin, rt_base_t value)
{
    if (pin == 0xff)
    {
        return;
    }

    gpio_set_pin_value(UC_GPIO, pin, (GPIO_VALUE)value);
}

static int uc8x88_pin_read(rt_device_t dev, rt_base_t pin)
{
    int value;

    value = PIN_LOW;
    if (pin == 0xff)
    {
        return value;
    }

    value = gpio_get_pin_value(UC_GPIO, pin);

    return value;
}

static void uc8x88_pin_mode(rt_device_t dev, rt_base_t pin, rt_base_t mode)
{
    if (pin == 0xff)
    {
        return;
    }

    gpio_set_pin_mux(UC_GPIO_CFG, pin, GPIO_FUNC_0);

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        /* output setting */
        gpio_set_pin_pupd(UC_GPIO_CFG, pin, GPIO_PUPD_UP);
        gpio_set_pin_direction(UC_GPIO, pin, GPIO_DIR_OUT);
        break;
    case PIN_MODE_INPUT:
        /* input setting */
        gpio_set_pin_pupd(UC_GPIO_CFG, pin, GPIO_PUPD_NONE);
        gpio_set_pin_direction(UC_GPIO, pin, GPIO_DIR_IN);
        break;
    case PIN_MODE_INPUT_PULLUP:
        /* input setting: pull up. */
        gpio_set_pin_pupd(UC_GPIO_CFG, pin, GPIO_PUPD_UP);
        gpio_set_pin_direction(UC_GPIO, pin, GPIO_DIR_IN);
        break;
    case PIN_MODE_OUTPUT_OD:
        /* output setting: od. */
        gpio_set_pin_pupd(UC_GPIO_CFG, pin, GPIO_PUPD_NONE);
        gpio_set_pin_direction(UC_GPIO, pin, GPIO_DIR_OUT);
        break;
    default:
        RT_ASSERT(0);
        break;
    }
}

rt_inline rt_int32_t bit2bitno(rt_uint32_t bit)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        if ((0x01 << i) == bit)
        {
            return i;
        }
    }
    return -1;
}

static rt_err_t uc8x88_pin_attach_irq(struct rt_device *device, rt_int32_t pin,
                                      rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    irqindex = pin;
    if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_hdr_tab))
    {
        return RT_ENOSYS;
    }

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == pin &&
        pin_irq_hdr_tab[irqindex].hdr == hdr &&
        pin_irq_hdr_tab[irqindex].mode == mode &&
        pin_irq_hdr_tab[irqindex].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    if (pin_irq_hdr_tab[irqindex].pin != -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EBUSY;
    }
    pin_irq_hdr_tab[irqindex].pin = pin;
    pin_irq_hdr_tab[irqindex].hdr = hdr;
    pin_irq_hdr_tab[irqindex].mode = mode;
    pin_irq_hdr_tab[irqindex].args = args;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t uc8x88_pin_dettach_irq(struct rt_device *device, rt_int32_t pin)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    irqindex = pin;
    if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_hdr_tab))
    {
        return RT_ENOSYS;
    }

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    pin_irq_hdr_tab[irqindex].pin = -1;
    pin_irq_hdr_tab[irqindex].hdr = RT_NULL;
    pin_irq_hdr_tab[irqindex].mode = 0;
    pin_irq_hdr_tab[irqindex].args = RT_NULL;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t uc8x88_pin_irq_enable(struct rt_device *device, rt_base_t pin,
                                      rt_uint32_t enabled)
{
    rt_base_t level;
    rt_int32_t irqindex = -1;

    if (enabled == PIN_IRQ_ENABLE)
    {
        irqindex = pin;
        if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_hdr_tab))
        {
            return RT_ENOSYS;
        }

        level = rt_hw_interrupt_disable();

        if (pin_irq_hdr_tab[irqindex].pin == -1)
        {
            rt_hw_interrupt_enable(level);
            return RT_ENOSYS;
        }

        switch (pin_irq_hdr_tab[irqindex].mode)
        {
        case PIN_IRQ_MODE_RISING:
            gpio_set_irq_type(UC_GPIO, pin, GPIO_IT_RISE_EDGE);
            break;
        case PIN_IRQ_MODE_FALLING:
            gpio_set_irq_type(UC_GPIO, pin, GPIO_IT_FALL_EDGE);
            break;
        case PIN_IRQ_MODE_HIGH_LEVEL:
            gpio_set_irq_type(UC_GPIO, pin, GPIO_IT_HIGH_LEVEL);
            break;
        case PIN_IRQ_MODE_LOW_LEVEL:
            gpio_set_irq_type(UC_GPIO, pin, GPIO_IT_LOW_LEVEL);
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            return -RT_EINVAL;
            break;
        }
        gpio_set_irq_en(UC_GPIO, pin, 1);

        // int_enable();
        gpio_int_enable();

        pin_irq_enable_mask |= pin;

        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        level = rt_hw_interrupt_disable();

        gpio_set_irq_en(UC_GPIO, pin, 0);

        pin_irq_enable_mask &= ~pin;

        if (pin_irq_enable_mask == 0)
        {
            gpio_int_disable();
        }

        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_ENOSYS;
    }

    return RT_EOK;
}

const static struct rt_pin_ops uc8x88_pin_ops = {
    uc8x88_pin_mode,
    uc8x88_pin_write,
    uc8x88_pin_read,
    uc8x88_pin_attach_irq,
    uc8x88_pin_dettach_irq,
    uc8x88_pin_irq_enable,
};

rt_inline void pin_irq_hdr(int irqno)
{
    if (irqno < 0 || irqno >= ITEM_NUM(pin_irq_hdr_tab))
    {
        return;
    }

    if (pin_irq_hdr_tab[irqno].hdr)
    {
        pin_irq_hdr_tab[irqno].hdr(pin_irq_hdr_tab[irqno].args);
    }
}

void gpio_handler(void)
{
    rt_interrupt_enter();

    uint32_t irq_status = 0;
    irq_status = gpio_get_irq_status(UC_GPIO);
    pin_irq_hdr(bit2bitno(irq_status));
    
    rt_interrupt_leave();
}

int rt_hw_pin_init(void)
{
    return rt_device_pin_register("pin", &uc8x88_pin_ops, RT_NULL);
}
INIT_BOARD_EXPORT(rt_hw_pin_init);

#endif /* RT_USING_PIN */
