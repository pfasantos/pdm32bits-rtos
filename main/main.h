/** @file main.h
 *  @brief RTOS recorder constants, shared buffers and task entry points.
 */
#ifndef MAIN_H
#define MAIN_H

#include "sdkconfig.h"
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

// macros
#define MAIN_PDM_BUFFER_SIZE I2S_BUFFER_SIZE / 4 // store buffer in long array
#define MAIN_PCM_BUFFER_SIZE I2S_BUFFER_SIZE / 2 // store buffer in short array

// tags
#define MAIN_TAG "main"
#define MAIN_READ_TAG "read_task"
#define MAIN_STORE_TAG "store_task"
#define MAIN_START_TAG "start_task"
#define MAIN_TIMER_TAG "timer"

// handles
QueueHandle_t xPcmQueue;
TimerHandle_t xRecordingTimer;
TaskHandle_t xReaderTask;
TaskHandle_t xStorageTask;
TaskHandle_t xStartTask;

// flags
volatile BaseType_t xReadFlag;
volatile BaseType_t xStoreFlag;

// cartao e arquivo
sdmmc_card_t *pxSdCard;
FILE *pxAudioFile;

// filtering structures
app_cic_t xCic;
app_fir_t xFir;

// buffers de gravacao
long plPdmBuffer[MAIN_PDM_BUFFER_SIZE];
short psStoreBuffer[MAIN_PCM_BUFFER_SIZE];
short psPcmBuffer[MAIN_PCM_BUFFER_SIZE];

// firs filtering coefficients (from levy)
short psFirCoefficients[FIR_ORDER] = {
    0, 0, 0, 0,  0,  0, 0, 0,  0, 0,  0,  0, 0,  0, 0, 0,  0,  0, 0, 0, 0, 0,
    0, 0, 1, -1, -1, 4, 0, -9, 4, 34, 34, 4, -9, 0, 4, -1, -1, 1, 0, 0, 0, 0,
    0, 0, 0, 0,  0,  0, 0, 0,  0, 0,  0,  0, 0,  0, 0, 0,  0,  0, 0, 0,
};

// reconfiguracao do clock
i2s_std_clk_config_t xRecordingClockConfig =
    I2S_STD_CLK_DEFAULT_CONFIG(CONFIG_PDM_I2S_RECORD_RATE_HZ);

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
 *  @param xTimer Expired FreeRTOS software timer; unused by the callback.
 */
void vMainRecTimer(TimerHandle_t xTimer);
/** @brief Open the first unused numbered recording path.
 *  @param pcBasePath Path prefix before the numeric suffix.
 *  @param pcExtension Filename extension, including its leading dot.
 *  @param pcMode Mode passed to fopen().
 *  @return Open file handle, or NULL if fopen() fails.
 *  @note Path construction uses a 128-byte buffer without truncation reporting.
 */
FILE *pxMainFopenUnique(const char *pcBasePath, const char *pcExtension, const char *pcMode);

#endif // MAIN_H
