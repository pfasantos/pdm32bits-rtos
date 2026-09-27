/** @file main.h
 *  @brief RTOS recorder constants, shared buffers and task entry points.
 */
#ifndef _MAIN_H_
#define _MAIN_H_
#include <string.h>

#include "freertos/FreeRTOS.h"

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

#include "i2s_std.h"
#include "pdm2pcm.h"
#include "sd_driver.h"

//macros
#define REC_TIME_MS     1 * 60 * 1000                    // recording time
#define PDM_BUF_SIZE    BUF_SIZE/4        // store buffer in long array
#define PCM_BUF_SIZE    BUF_SIZE/2        // store buffer in short array

// tags
#define MAIN_TAG  "main"
#define I2S_TAG   "i2s"
#define READ_TAG  "read_task"
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
short st_buffer[PCM_BUF_SIZE];
short data_buffer[PCM_BUF_SIZE];

//firs filtering coefficients (from levy)
short fir_coeffs[FIR_ORDER] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, -1, -1, 4, 0, -9, 4, 34, 34, 4, -9, 0, 4, -1, -1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// reconfiguracao do clock
i2s_std_clk_config_t clk_rec_cfg = I2S_STD_CLK_DEFAULT_CONFIG(75000);

// function declarations
/** @brief Legacy task declaration without a definition in this project.
 *  @param pvParameters Unused task parameter if implemented.
 */
void vTaskStart(void *pvParameters);
/** @brief Read I2S buffers, convert PDM words and queue PCM samples.
 *  @param pvParameters Unused FreeRTOS task argument.
 *  @note Waits for the recording timer's notification before exiting.
 */
void vTaskRead(void *pvParameters);
/** @brief Filter queued samples and write them to the raw file.
 *  @param pvParameters Unused FreeRTOS task argument.
 *  @note Drains queued buffers after the reader notifies it to stop.
 */
void vTaskStore(void *pvParameters);
/** @brief Notify the reader task when the recording interval expires.
 *  @param xTimerHandle Expired FreeRTOS software timer; unused by the callback.
 */
void vRecTimer(TimerHandle_t xTimerHandle);
/** @brief Open the first unused numbered recording path.
 *  @param base_path Path prefix before the numeric suffix.
 *  @param ext Filename extension, including its leading dot.
 *  @param mode Mode passed to fopen().
 *  @return Open file handle, or NULL if fopen() fails.
 *  @note Path construction uses a 128-byte buffer without truncation reporting.
 */
FILE *fopen_unique(const char *base_path, const char *ext, const char *mode);

#endif // _MAIN_H_