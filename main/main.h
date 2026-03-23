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

//macros
#define REC_TIME_MS     5 * 1000                    // recording time
#define PDM_BUF_SIZE        I2S_BUF_SIZE_BYTES/4    // store buffer in long array
#define PCM_BUF_SIZE    I2S_BUF_SIZE_BYTES/2        // store buffer in short array

// tags
#define MAIN_TAG  "main"
#define I2S_TAG   "i2s"
#define STORE_TAG "store_task"
#define START_TAG "start_task"
#define TIMER_TAG "timer"

// handles
QueueHandle_t xQueueHandle;
TimerHandle_t xRecTimerHandle;
TaskHandle_t xTaskReadHandle;
TaskHandle_t xTaskStoreHandle;
TaskHandle_t xTaskStartHandle;

//flags
volatile BaseType_t read_flag;
volatile BaseType_t st_flag;

// cartao e arquivo
sdmmc_card_t *card;
FILE *audio_file;

//filtering structures
app_cic_t cic;
app_fir_t fir;

// buffers de gravacao
long rx_buffer[PDM_BUF_SIZE];
long st_buffer[PDM_BUF_SIZE];
short data_buffer[PCM_BUF_SIZE];

//firs filtering coefficients (from levy)
short fir_coeffs[FIR_ORDER] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, -1, -1, 4, 0, -9, 4, 34, 34, 4, -9, 0, 4, -1, -1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// reconfiguracao do clock
i2s_std_clk_config_t clk_rec_cfg = I2S_STD_CLK_DEFAULT_CONFIG(78125);

// function declarations
void vTaskStart(void *pvParameters);
void vTaskRead(void *pvParameters);
void vTaskStore(void *pvParameters);
void vRecTimer(TimerHandle_t xTimerHandle);
FILE *fopen_unique(const char *base_path, const char *ext, const char *mode);

#endif // _MAIN_H_