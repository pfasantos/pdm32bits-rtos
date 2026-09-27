#include "main.h"
#include "driver/i2s_common.h"
#include "i2s_std.h"
#include "pdm2pcm.h"
#include "sd_driver.h"
#include <unistd.h>

// TASKS SECTION --------------------------

void vTaskRead(void *pvParameters)
{
    for (;;)
    {
        if (ulTaskNotifyTake(pdTRUE, 0) != 0)
        {
            break;
        }

        // wait until plPdmBuffer is full
        if (i2s_channel_read(xRxHandle, (void *)plPdmBuffer, I2S_BUFFER_SIZE, NULL,
                             portMAX_DELAY) == ESP_OK)
        {
            process_app_cic(&xCic, &plPdmBuffer, &psPcmBuffer);
            xQueueSend(xPcmQueue, &psPcmBuffer, portMAX_DELAY);
        }
        else
        {
            ESP_LOGE(I2S_TAG, "Erro durante a leitura: errno %d", errno);
            break;
        }
    }
    ESP_LOGI(MAIN_READ_TAG, "Leitura I2S terminada");
    vI2SStdStop();

    xTaskNotifyGive(xStorageTask);
    vTaskDelete(NULL);
}

void vTaskStore(void *pvParameters)
{
    for (;;)
    {
        if ((xPcmQueue != NULL) &&
            (xQueueReceive(xPcmQueue, psStoreBuffer, pdMS_TO_TICKS(500)) == pdTRUE))
        {
            process_new_fir(&psStoreBuffer);

            fwrite(psStoreBuffer, sizeof(short), MAIN_PCM_BUFFER_SIZE, pxAudioFile);
        }
        // iriie what is left when reading ends
        if (ulTaskNotifyTake(pdTRUE, 0) != 0)
        {
            while (uxQueueMessagesWaiting(xPcmQueue) > 0)
            {
                if (xQueueReceive(xPcmQueue, psStoreBuffer, 0) == pdTRUE)
                {
                    process_new_fir(&psStoreBuffer);

                    fwrite(psStoreBuffer, sizeof(short), MAIN_PCM_BUFFER_SIZE, pxAudioFile);
                }
            }
            break;
        }
    }
    fsync(fileno(pxAudioFile));
    fclose(pxAudioFile);
    xSdDriverDeinit(pxSdCard);

    ESP_LOGI(MAIN_STORE_TAG, "Armazenamento encerrado e arquivo salvo");
    vTaskDelete(NULL);
}

// TIMERS SECTION --------------------------

void vMainRecTimer(TimerHandle_t xTimer)
{
    xTaskNotifyGive(xReaderTask);
    ESP_LOGI(MAIN_TIMER_TAG, "Tempo de gravacao acabou.");
}

// FUNCTIONS SECTION ------------------------

FILE *pxMainFopenUnique(const char *pcBasePath, const char *pcExtension, const char *pcMode)
{
    char cFilePath[128];
    struct stat xFileStat;
    int iIndex = 0;

    for (;;)
    {
        // Construct the filename: pcBasePath + "_" + iIndex + pcExtension
        snprintf(cFilePath, sizeof(cFilePath), "%s_%d%s", pcBasePath, iIndex, pcExtension);

        // Check if file exists
        if (stat(cFilePath, &xFileStat) == 0)
        {
            iIndex++;
        }
        else
        {
            break;
        }
    }

    ESP_LOGI("FILE_SYS", "Opening file: %s", cFilePath);
    return fopen(cFilePath, pcMode);
}

// MAIN SETUP SECTION -----------------------

/** @brief Initialize I2S, storage, filters, tasks, queue and recording timer.
 *  The timer ends the reader task; the storage task then drains the queue and
 *  closes the raw sample file.
 *  @warning This function does not check xSdDriverInit() or fwrite() results.
 */
void app_main(void)
{
    vI2SStdInit();
    xSdDriverInit(pxSdCard);

    i2s_channel_enable(xRxHandle);
    vTaskDelay(pdMS_TO_TICKS(5));
    i2s_channel_disable(xRxHandle);
    i2s_channel_reconfig_std_clock(xRxHandle, &xRecordingClockConfig);
    i2s_channel_enable(xRxHandle);

    init_app_cic(&xCic);
    init_app_fir(&xFir);

    pxAudioFile = pxMainFopenUnique(SD_MOUNT_POINT "/file", ".raw", "wb");
    if (pxAudioFile == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha ao abrir o arquivo");
        return;
    }

    xPcmQueue = xQueueCreate(CONFIG_PDM_DMA_BUFFER_COUNT, MAIN_PCM_BUFFER_SIZE * sizeof(short));
    if (xPcmQueue == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha em criar fila de dados");
        for (;;)
            ;
    }

    xRecordingTimer =
        xTimerCreate("REC timer", pdMS_TO_TICKS(CONFIG_PDM_RECORDING_DURATION_SECONDS * 1000U),
                     pdFALSE, (void *)0, vMainRecTimer);

    if (xRecordingTimer == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha ao criar o timer");
        for (;;)
            ;
    }

    BaseType_t xTaskCreateStatus[2];
    xTaskCreateStatus[0] =
        xTaskCreatePinnedToCore(vTaskRead, "taskREAD", configMINIMAL_STACK_SIZE + 4096, NULL,
                                configMAX_PRIORITIES - 3, &xReaderTask, APP_CPU_NUM);

    xTaskCreateStatus[1] =
        xTaskCreatePinnedToCore(vTaskStore, "taskSTORE", configMINIMAL_STACK_SIZE + 4096, NULL,
                                configMAX_PRIORITIES - 3, &xStorageTask, PRO_CPU_NUM);

    // test tasks creation
    for (int iTaskIndex = 0; iTaskIndex < 2; iTaskIndex++)
    {
        if (xTaskCreateStatus[iTaskIndex] == pdFAIL)
        {
            ESP_LOGE(MAIN_TAG, "Erro ao criar a task %d", iTaskIndex);
            for (;;)
                ;
        }
    }

    xTimerStart(xRecordingTimer, 0);
    ESP_LOGI(MAIN_START_TAG, "Gravacao iniciada");
}
