#ifndef _I2S_STD_H_
#define _I2S_STD_H_

#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2s_pdm.h"
#include "driver/gpio.h"

//tags
#define I2S_TAG "i2s"

//macros
#define BIT_DEPTH I2S_DATA_BIT_WIDTH_32BIT                 // i2s bit depth
#define DMA_BUF_NUM 16                                     // quantity of dma buffers 
#define DMA_BUF_SIZE 511                                   // number of samples of dma buffer 
#define I2S_BUF_SIZE_BYTES 2 * DMA_BUF_SIZE *BIT_DEPTH / 8 // size in bytes of i2s buffer 

extern i2s_chan_handle_t rx_handle;

void i2s_init();
void i2s_stop();

#endif // _I2S_STD_H_