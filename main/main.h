#ifndef _MAIN_H_
#define _MAIN_H_
#include <string.h>

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>

#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define FSYNC_THRESHOLD_BYTES DMA_BUF_SIZE *DMA_BUF_NUM
#define REC_TIME_MICROSECONDS 5*1000000

// tags
#define MAIN_TAG "main"
#define I2S_TAG "i2s"
#define STORE_TAG "store task"

//handles
QueueHandle_t xQueueHandle;
TaskHandle_t xTaskReadHandle;
TaskHandle_t xTaskStoreHandle;

FILE *audio_file;
char rx_buffer[BUF_SIZE];
char st_buffer[BUF_SIZE];

i2s_std_clk_config_t clk_rec_cfg = I2S_STD_CLK_DEFAULT_CONFIG(78125);

#endif // _MAIN_H_