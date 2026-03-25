#include "i2s_std.h"
#include "sd_driver.h"
#include "pdm2pcm.h"
#include "main.h"

// TASKS SECTION --------------------------

void vTaskStart(void *pvParameters)
{
    // start in smaller clock and rise after 5ms
    i2s_channel_enable(rx_handle);
    vTaskDelay(pdMS_TO_TICKS(5));
    i2s_channel_disable(rx_handle);
    i2s_channel_reconfig_std_clock(rx_handle, &clk_rec_cfg);
    i2s_channel_enable(rx_handle);

    init_app_cic(&cic);
    init_app_fir(&fir);

    vTaskResume(xTaskReadHandle);
    vTaskResume(xTaskStoreHandle);
    xTimerStart(xRecTimerHandle, 0);
    ESP_LOGI(START_TAG, "Gravacao iniciada");

    vTaskSuspend(xTaskStartHandle);
}

void vTaskRead(void *pvParameters)
{
    static uint32_t read_count = 0;
    while (1)
    {
        if (ulTaskNotifyTake(pdTRUE, 0) != 0)
        {
            break;
        }

        // wait untill rx_buffer is full 
        if (i2s_channel_read(rx_handle, (void *)rx_buffer, I2S_BUF_SIZE_BYTES, NULL, portMAX_DELAY) == ESP_OK)
        {
            read_count++;
            xQueueSend(xQueueHandle, &rx_buffer, portMAX_DELAY); // enfileirar dados lidos para a tarefa de envio
        }
        else
        {
            ESP_LOGE(I2S_TAG, "Erro durante a leitura: errno %d", errno);
            break;
        }
    }
    ESP_LOGI(READ_TAG, "Leitura I2S terminada: %lu blocos ", read_count);
    i2s_stop();

    xTaskNotifyGive(xTaskStoreHandle);
    vTaskDelete(NULL);
}

void vTaskStore(void *pvParameters)
{
    static uint32_t write_count = 0;
    while (1)
    {
        if ((xQueueHandle != NULL) && (xQueueReceive(xQueueHandle, st_buffer, pdMS_TO_TICKS(500)) == pdTRUE))
        {
            process_app_cic(&cic, &st_buffer, &data_buffer);
            process_app_fir(&fir, fir_coeffs, &data_buffer);
            fwrite(data_buffer, sizeof(short), PCM_BUF_SIZE, audio_file);
            write_count++;
        }
        // write what is left when reading ends 
        if (ulTaskNotifyTake(pdTRUE, 0) != 0)
        {
            while (uxQueueMessagesWaiting(xQueueHandle) > 0)
            {
                if (xQueueReceive(xQueueHandle, st_buffer, 0) == pdTRUE)
                {
                    process_app_cic(&cic, &st_buffer, &data_buffer);
                    process_app_fir(&fir, fir_coeffs, &data_buffer);
                    fwrite(data_buffer, sizeof(short), PCM_BUF_SIZE, audio_file);
                    write_count++;
                }
            }
            break;
        }
    }
    fsync(fileno(audio_file));
    fclose(audio_file);
    sdcard_deinit(card);

    ESP_LOGI(STORE_TAG, "Armazenamento encerrado e arquivo salvo: %lu blocos gravados", write_count);
    vTaskDelete(NULL);
}

// TIMERS SECTION --------------------------

void vRecTimer(TimerHandle_t xTimerHandle)
{
    xTaskNotifyGive(xTaskReadHandle);
    ESP_LOGI(TIMER_TAG, "Tempo de gravacao acabou.");
}

// FUNCTIONS SECTION ------------------------

FILE *fopen_unique(const char *base_path, const char *ext, const char *mode)
{
    char file_path[128];
    struct stat st;
    int index = 0;

    while (1)
    {
        // Construct the filename: base_path + "_" + index + ext
        snprintf(file_path, sizeof(file_path), "%s_%d%s", base_path, index, ext);

        // Check if file exists
        if (stat(file_path, &st) == 0)
        {
            index++;
        }
        else
        {
            break;
        }
    }

    ESP_LOGI("FILE_SYS", "Opening file: %s", file_path);
    return fopen(file_path, mode);
}

// MAIN SETUP SECTION -----------------------

void app_main(void)
{
    i2s_init();
    sdcard_init(card);

    audio_file = fopen_unique(MOUNT_POINT "/file", ".raw", "wb");
    if (audio_file == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha ao abrir o arquivo");
        return;
    }

    xQueueHandle = xQueueCreate(DMA_BUF_NUM, I2S_BUF_SIZE_BYTES);
    if (xQueueHandle == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha em criar fila de dados");
        while (1);
    }

    xRecTimerHandle = xTimerCreate(
        "REC timer",
        pdMS_TO_TICKS(REC_TIME_MS),
        pdFALSE,
        (void *)0,
        vRecTimer);

    if (xRecTimerHandle == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha ao criar o timer");
        while (1);
    }

    BaseType_t xReturnedTask[3];
    xReturnedTask[0] = xTaskCreatePinnedToCore(
        vTaskRead,
        "taskREAD",
        configMINIMAL_STACK_SIZE + 4096,
        NULL,
        configMAX_PRIORITIES - 3,
        &xTaskReadHandle,
        APP_CPU_NUM);

    xReturnedTask[1] = xTaskCreatePinnedToCore(
        vTaskStore,
        "taskSTORE",
        configMINIMAL_STACK_SIZE + 4096,
        NULL,
        configMAX_PRIORITIES - 3,
        &xTaskStoreHandle,
        PRO_CPU_NUM);

    xReturnedTask[2] = xTaskCreatePinnedToCore(
        vTaskStart,
        "taskSTART",
        configMINIMAL_STACK_SIZE + 1024,
        NULL,
        configMAX_PRIORITIES - 2,
        &xTaskStartHandle,
        APP_CPU_NUM);
    
    // test tasks creation
    for (int i = 0; i < 3; i++)
    {
        if (xReturnedTask[i] == pdFAIL)
        {
            ESP_LOGE(MAIN_TAG, "Erro ao criar a task %d", i);
            while (1);
        }
    }

    vTaskSuspend(xTaskReadHandle);
    vTaskSuspend(xTaskStoreHandle);
}