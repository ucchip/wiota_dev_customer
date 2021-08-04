// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
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
void spim_init(SPI_TypeDef* SPIx, SPIM_CFG_Type* SPI_ConfigStruc)
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
void spim_send_data_noaddr(int cmd, char* data, int datalen, int useQpi);


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
void spim_setup_cmd_addr(SPI_TypeDef* SPIx, int cmd, int cmdlen, int addr, int addrlen)
{
    int cmd_reg;

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
void spim_setup_dummy(SPI_TypeDef* SPIx, int dummy_rd, int dummy_wr)
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
void spim_set_datalen(SPI_TypeDef* SPIx, int datalen)
{
    volatile int old_len;

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
void spim_start_transaction(SPI_TypeDef* SPIx, int trans_type, int csnum)
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
int spim_get_status(SPI_TypeDef* SPIx)
{
    volatile int status;
    status = SPIx->STATUS;
    return status;
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
void spim_write_fifo(SPI_TypeDef* SPIx, int* data, int datalen)
{

    volatile int num_words, i;
    num_words = (datalen >> 5) & SPILEN_NUM_MASK;

    if ( (datalen & SPILEN_DATA_MASK) != 0)
    { num_words++; }
//    printf("num word is %d\r\n", num_words);
    for (i = 0; i < num_words; i++)
    {
        while ((( (SPIx->STATUS) >> 24) & SPI_TRA_MASK) >= 8);
        SPIx->TXFIFO = data[i];
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
void spim_read_fifo(SPI_TypeDef* SPIx, int* data, int datalen)
{

    volatile int num_words;
    /* must use register for, i,j*/
    register int i, j;
    num_words = (datalen >> 5) & SPILEN_NUM_MASK;

    if ( (datalen & SPILEN_DATA_MASK) != 0)
    { num_words++; }
    i = 0;
    while (1)
    {
        do
        {
            j = (((SPIx->STATUS) >> 16) & SPI_TRA_MASK);
        } while (j == 0);
        do
        {
            data[i++] = SPIx->RXFIFO;
            j--;
        } while (j);
        if (i >= num_words)
        {
            break;
        }
    }

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
void Udma_Spim_Tx(SPI_TypeDef* SPIx, uint32_t* source_addr, uint16_t size)
{
    DMA_CFG_Type dma_cfg;
    dma_cfg.channel_chan_select = CHAN1_DIS;

    udma_init(UC_DMA, &dma_cfg);

    DMACHAN_CFG_Type dma_chan_cfg;
    dma_chan_cfg.channel_reset          = ENABLE;
    dma_chan_cfg.channel_enable         = ENABLE;
    dma_chan_cfg.channel_s_increment    = MEMORY;
    dma_chan_cfg.channel_d_increment    = FIFO_ADDR;
    dma_chan_cfg.channel_mode           = HANDSHACK_MODE;
    dma_chan_cfg.channel_pause          = DISABLE;
    dma_chan_cfg.channel_endof          = DISABLE;
    dma_chan_cfg.channel_size           = size;

    uint32_t s_addr = (uint32_t)source_addr;
    uint32_t d_addr = (uint32_t)&(SPIx->TXFIFO);
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
void Udma_Spim_Rx(SPI_TypeDef* SPIx, uint32_t* dest_addr, uint16_t size)
{
    DMA_CFG_Type dma_cfg;
    dma_cfg.channel_chan_select = CHAN1_SOR;

    udma_init(UC_DMA, &dma_cfg);

    DMACHAN_CFG_Type dma_chan_cfg;
    dma_chan_cfg.channel_reset          = ENABLE;
    dma_chan_cfg.channel_enable         = ENABLE;
    dma_chan_cfg.channel_s_increment    = FIFO_ADDR;
    dma_chan_cfg.channel_d_increment    = MEMORY;
    dma_chan_cfg.channel_mode           = HANDSHACK_MODE;
    dma_chan_cfg.channel_pause          = DISABLE;
    dma_chan_cfg.channel_endof          = DISABLE;
    dma_chan_cfg.channel_size           = size;

    uint32_t s_addr = (uint32_t)&(SPIx->RXFIFO);
    uint32_t d_addr = (uint32_t)dest_addr;
    udma_chan_set_source_addr(UC_DMA_CHAN1, s_addr);
    udma_chan_set_destina_addr(UC_DMA_CHAN1, d_addr);
    udma_chan_init(UC_DMA_CHAN1, &dma_chan_cfg);
}