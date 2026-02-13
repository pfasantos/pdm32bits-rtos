#ifndef _I2S_STD_H_
#define _I2S_STD_H_

#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2s_pdm.h"
#include "driver/gpio.h"

#define I2S_TAG "i2s"

#define BIT_DEPTH I2S_DATA_BIT_WIDTH_32BIT
#define DMA_BUF_NUM 16                           // quantidade de buffers do DMA
#define DMA_BUF_SIZE 511                         // tamanho em amostras dos buffers do DMA
#define BUF_SIZE 2 * DMA_BUF_SIZE *BIT_DEPTH / 8 // tamanho em bytes dos buffers de transmissao

extern i2s_chan_handle_t rx_handle;

void i2s_init();
void i2s_stop();

#endif // _I2S_STD_H_