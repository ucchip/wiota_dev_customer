#include <uc_spim.h>
#include <uc_gpio.h>

/*********************************************************
 * Function name    :void spim_init(SPI_TypeDef *SPIx,
 *                          SPIM_CFG_Type *SPI_ConfigStruc)
 * Description      :spi initialize function
 * Input Parameter  :SPI_TypeDef *SPIx              :select SPI
 *                   SPIM_CFG_Type *SPI_ConfigStruc :configurating spi struct
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_init(SPI_TypeDef *SPIx, SPIM_CFG_Type *SPI_ConfigStruc)
{
    CHECK_PARAM(PARAM_SPIM(SPIx));

    /*definition bound rate of spi*/
    SPIx->CLKDIV = SPI_ConfigStruc->Clk_rate;
}

/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_send_data_noaddr(int cmd, char *data, int datalen, int useQpi);

/*********************************************************
 * Function name    :void spim_setup_cmd_addr(SPI_TypeDef *SPIx,
                                              int cmd,
                                              int cmdlen,
                                              int addr,
                                              int addrlen)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_setup_cmd_addr(SPI_TypeDef *SPIx, int cmd, int cmdlen, int addr, int addrlen)
{
    int cmd_reg;

    cmdlen *= 8;
    addrlen *= 8;

    /**/
    cmd_reg = cmd << (32 - cmdlen);

    /**/
    SPIx->SPICMD = cmd_reg;

    /**/
    SPIx->SPIADR = addr;

    /**/
    SPIx->SPILEN = (cmdlen & SPILEN_CLR_MASK) |
                   ((addrlen << 8) & SPILEN_ALR_MASK);
}

/*********************************************************
 * Function name    :void spim_setup_dummy(SPI_TypeDef *SPIx,
                                            int dummy_rd,
                                            int dummy_wr)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_setup_dummy(SPI_TypeDef *SPIx, int dummy_rd, int dummy_wr)
{
    /**/
    SPIx->SPIDUM = ((dummy_wr << 16) & SPIDUM_TDR_MASK) |
                   (dummy_rd & SPIDUM_RDR_MASK);
}

/*********************************************************
 * Function name    :void spim_set_datalen(SPI_TypeDef *SPIx,
                                            int datalen)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_set_datalen(SPI_TypeDef *SPIx, int datalen)
{
    volatile int old_len;

    datalen *= 8;

    /**/
    old_len = SPIx->SPILEN;

    /**/
    old_len = ((datalen << 16) & (SPILEN_RLR_MASK | SPILEN_TLR_MASK)) |
              (old_len & (SPILEN_CLR_MASK | SPILEN_ALR_MASK));

    /**/
    SPIx->SPILEN = old_len;
}

/*********************************************************
 * Function name    :void spim_start_transaction(SPI_TypeDef *SPIx,
                                                 int trans_type,
                                                 int csnum)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_start_transaction(SPI_TypeDef *SPIx, int trans_type, int csnum)
{
    /**/
    SPIx->STATUS = ((1 << (csnum + 8)) & SPI_CSN_MASK) |
                   ((1 << trans_type) & SPI_TRA_MASK);
}

/*********************************************************
 * Function name    :int spim_get_status(SPI_TypeDef *SPIx)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
int spim_get_status(SPI_TypeDef *SPIx)
{
    volatile int status;
    status = SPIx->STATUS;
    return status;
}

void spim_data_big_little_endian_change(char *data, uint32_t num_words)
{
    volatile uint32_t i;
    volatile char temp;

    for (i = 0; i < num_words; i++)
    {
        temp = data[4 * i];
        data[4 * i] = data[4 * i + 3];
        data[4 * i + 3] = temp;

        temp = data[4 * i + 1];
        data[4 * i + 1] = data[4 * i + 2];
        data[4 * i + 2] = temp;
    }
}

/*********************************************************
 * Function name    :void spim_write_fifo(SPI_TypeDef *SPIx,
                                          int *data,
                                          int datalen)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_write_fifo(SPI_TypeDef *SPIx, char *wdata, uint32_t datalen)
{
    volatile uint32_t num_words, remain_len;
    volatile uint32_t i;

    remain_len = datalen % 4;

    datalen *= 8;

    num_words = (datalen >> 5) & SPILEN_NUM_MASK;

    spim_data_big_little_endian_change(wdata, num_words);

    for (i = 0; i < num_words; i++)
    {
        while ((((SPIx->STATUS) >> 24) & SPI_TRA_MASK) >= 8)
        {
            ;
        }
        SPIx->TXFIFO = ((uint32_t *)wdata)[i];
    }

    if (remain_len != 0)
    {
        volatile union spim_data send_data = {0};

        for (i = 0; i < remain_len; i++)
        {
            send_data.data8[3 - (i % 4)] = wdata[num_words * 4 + i];
        }

        while ((((SPIx->STATUS) >> 24) & SPI_TRA_MASK) >= 8)
        {
            ;
        }
        SPIx->TXFIFO = send_data.data32;
    }
}

/*********************************************************
 * Function name    :void spim_read_fifo(SPI_TypeDef *SPIx,
                                         int *data,
                                         int datalen)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spim_read_fifo(SPI_TypeDef *SPIx, char *rdata, uint32_t datalen)
{
    volatile uint32_t num_words, remain_len;
    volatile uint32_t i;

    remain_len = datalen % 4;

    datalen *= 8;

    num_words = (datalen >> 5) & SPILEN_NUM_MASK;

    for (i = 0; i < num_words; i++)
    {
        while ((((SPIx->STATUS) >> 16) & SPI_TRA_MASK) == 0)
        {
            ;
        }
        ((uint32_t *)rdata)[i] = SPIx->RXFIFO;
    }

    spim_data_big_little_endian_change(rdata, num_words);

    if (remain_len != 0)
    {
        volatile union spim_data recv_data = {0};

        while ((((SPIx->STATUS) >> 16) & SPI_TRA_MASK) == 0)
        {
            ;
        }
        recv_data.data32 = SPIx->RXFIFO;

        for (i = 0; i < remain_len; i++)
        {
            rdata[num_words * 4 + i] = recv_data.data8[(remain_len - 1) - (i % remain_len)];
        }
    }

    while ((((SPIx->STATUS) >> 16) & SPI_TRA_MASK) != 0)
    {
        i = SPIx->RXFIFO;
    }
}

void spim_write_read_fifo(SPI_TypeDef *SPIx, char *wdata, char *rdata, uint32_t datalen)
{
    volatile uint32_t num_words, remain_len;
    volatile uint32_t i;

    remain_len = datalen % 4;

    datalen *= 8;

    num_words = (datalen >> 5) & SPILEN_NUM_MASK;

    spim_data_big_little_endian_change(wdata, num_words);

    for (i = 0; i < num_words; i++)
    {
        while ((((SPIx->STATUS) >> 24) & SPI_TRA_MASK) >= 8)
        {
            ;
        }
        SPIx->TXFIFO = ((uint32_t *)wdata)[i];

        while ((((SPIx->STATUS) >> 16) & SPI_TRA_MASK) == 0)
        {
            ;
        }
        ((uint32_t *)rdata)[i] = SPIx->RXFIFO;
    }

    if (remain_len != 0)
    {
        volatile union spim_data send_data = {0};
        volatile union spim_data recv_data = {0};

        for (i = 0; i < remain_len; i++)
        {
            send_data.data8[3 - (i % 4)] = wdata[num_words * 4 + i];
        }

        while ((((SPIx->STATUS) >> 24) & SPI_TRA_MASK) >= 8)
        {
            ;
        }
        SPIx->TXFIFO = send_data.data32;

        while ((((SPIx->STATUS) >> 16) & SPI_TRA_MASK) == 0)
        {
            ;
        }
        recv_data.data32 = SPIx->RXFIFO;

        for (i = 0; i < remain_len; i++)
        {
            rdata[num_words * 4 + i] = recv_data.data8[(remain_len - 1) - (i % remain_len)];
        }
    }

    while ((((SPIx->STATUS) >> 16) & SPI_TRA_MASK) != 0)
    {
        i = SPIx->RXFIFO;
    }
}

void spim_transmit_send(SPI_TypeDef *SPIx, char *data, uint32_t datalen)
{
    spim_setup_cmd_addr(SPIx, 0, 0, 0, 0);
    spim_set_datalen(SPIx, datalen);
    spim_setup_dummy(SPIx, 0, 0);
    spim_start_transaction(SPIx, SPIM_CMD_WR, SPIM_CSN0);
    spim_write_fifo(SPIx, data, datalen);
    SPIM_WAIT(SPIx);
}

void spim_transmit_recv(SPI_TypeDef *SPIx, char *data, uint32_t datalen)
{
    spim_setup_cmd_addr(SPIx, 0, 0, 0, 0);
    spim_set_datalen(SPIx, datalen);
    spim_setup_dummy(SPIx, 0, 0);
    spim_start_transaction(SPIx, SPIM_CMD_RD, SPIM_CSN0);
    spim_read_fifo(SPIx, data, datalen);
    SPIM_WAIT(SPIx);
}

void spim_transmit_send_recv(SPI_TypeDef *SPIx, char *wdata, char *rdata, uint32_t datalen)
{
    spim_setup_cmd_addr(SPIx, 0, 0, 0, 0);
    spim_set_datalen(SPIx, datalen);
    spim_setup_dummy(SPIx, 0, 0);
    spim_start_transaction(SPIx, SPIM_CMD_WR, SPIM_CSN0);
    spim_write_read_fifo(SPIx, wdata, rdata, datalen);
    SPIM_WAIT(SPIx);
}

/* last function in spi lib, linked to bootstrap code.
 * calling this cause cache to fill 2nd block, so we have
 * 2 blocks of code in cache */
void spim_no_op()
{
    return;
}

/*********************************************************
 * Function name    :void UDMA_Spim_Tx(SPI_TypeDef *SPIx,
 *                                     uint32_t *source_addr,
                                       uint16_t size)
 * Description      :spi send data with dma
 * Input Parameter  :SPI_TypeDef *SPIx      :select SPI
 * Outout Parameter :uint32_t *source_addr  :source addr
 *                   uint16_t size          :data size
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void Udma_Spim_Tx(SPI_TypeDef *SPIx, uint32_t *source_addr, uint16_t size)
{
    DMA_CFG_Type dma_cfg;
    dma_cfg.channel_chan_select = CHAN1_DIS;

    size *= 2;

    udma_init(UC_DMA, &dma_cfg);

    DMACHAN_CFG_Type dma_chan_cfg;
    dma_chan_cfg.channel_reset = ENABLE;
    dma_chan_cfg.channel_enable = ENABLE;
    dma_chan_cfg.channel_s_increment = MEMORY;
    dma_chan_cfg.channel_d_increment = FIFO_ADDR;
    dma_chan_cfg.channel_mode = HANDSHACK_MODE;
    dma_chan_cfg.channel_pause = DISABLE;
    dma_chan_cfg.channel_endof = DISABLE;
    dma_chan_cfg.channel_size = size;

    uint32_t s_addr = (uint32_t)source_addr;
    uint32_t d_addr = (uint32_t) & (SPIx->TXFIFO);
    udma_chan_set_source_addr(UC_DMA_CHAN1, s_addr);
    udma_chan_set_destina_addr(UC_DMA_CHAN1, d_addr);
    udma_chan_init(UC_DMA_CHAN1, &dma_chan_cfg);
}

/*********************************************************
 * Function name    :void UDMA_Spim_Rx(SPI_TypeDef  *SPIx,
 *                                     uint32_t     *dest_addr,
                                       uint16_t     size)
 * Description      :spi receive data with dma
 * Input Parameter  :SPI_TypeDef *SPIx      :select SPI
 * Outout Parameter :uint32_t *source_addr  :Distination addr
 *                   uint16_t size          :data size
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void Udma_Spim_Rx(SPI_TypeDef *SPIx, uint32_t *dest_addr, uint16_t size)
{
    DMA_CFG_Type dma_cfg;
    dma_cfg.channel_chan_select = CHAN1_SOR;

    size *= 2;

    udma_init(UC_DMA, &dma_cfg);

    DMACHAN_CFG_Type dma_chan_cfg;
    dma_chan_cfg.channel_reset = ENABLE;
    dma_chan_cfg.channel_enable = ENABLE;
    dma_chan_cfg.channel_s_increment = FIFO_ADDR;
    dma_chan_cfg.channel_d_increment = MEMORY;
    dma_chan_cfg.channel_mode = HANDSHACK_MODE;
    dma_chan_cfg.channel_pause = DISABLE;
    dma_chan_cfg.channel_endof = DISABLE;
    dma_chan_cfg.channel_size = size;

    uint32_t s_addr = (uint32_t) & (SPIx->RXFIFO);
    uint32_t d_addr = (uint32_t)dest_addr;
    udma_chan_set_source_addr(UC_DMA_CHAN1, s_addr);
    udma_chan_set_destina_addr(UC_DMA_CHAN1, d_addr);
    udma_chan_init(UC_DMA_CHAN1, &dma_chan_cfg);
}