#include "i2s_std.h"
#include "sd_driver.h"
#include "main.h"

// TASKS SECTION ----------------------

void vTaskStart(void *pvParameters)
{
    // inicia canal i2s
    i2s_channel_reconfig_std_clock(rx_handle, &clk_rec_cfg);
    i2s_channel_enable(rx_handle);

    vTaskResume(xTaskReadHandle);
    vTaskResume(xTaskStoreHandle);
    xTimerStart(xRecTimerHandle, 0);
    ESP_LOGI(START_TAG, "Gravacao iniciada");

    vTaskSuspend(xTaskStartHandle);
}

void vTaskRead(void *pvParameters)
{
    while (1)
    {
        if (ulTaskNotifyTake(pdTRUE, 0) != 0)
        {
            break;
        }

        // esperar para que o buffer de entrada (in_buffer) seja totalmente preenchido
        if (i2s_channel_read(rx_handle, (void *)rx_buffer, BUF_SIZE, NULL, portMAX_DELAY) == ESP_OK)
        {
            xQueueSend(xQueueHandle, &rx_buffer, portMAX_DELAY); // enfileirar dados lidos para a tarefa de envio
        }
        else
        {
            ESP_LOGE(I2S_TAG, "Erro durante a leitura: errno %d", errno);
            break;
        }
    }

    i2s_stop();

    xTaskNotifyGive(xTaskStoreHandle);
    vTaskDelete(NULL);
}

void vTaskStore(void *pvParameters)
{
    while (1)
    {
        if ((xQueueHandle != NULL) && (xQueueReceive(xQueueHandle, st_buffer, pdMS_TO_TICKS(500)) == pdTRUE))
        {
            fwrite(st_buffer, 1, BUF_SIZE, audio_file);
        }
        // leitura parou, escrever oq sobrou
        if (ulTaskNotifyTake(pdTRUE, 0) != 0)
        {
            while (uxQueueMessagesWaiting(xQueueHandle) > 0)
            {
                if (xQueueReceive(xQueueHandle, st_buffer, 0) == pdTRUE)
                {
                    fwrite(st_buffer, 1, BUF_SIZE, audio_file);
                }
            }
            break;
        }
    }
    fsync(fileno(audio_file));
    fclose(audio_file);
    sdcard_deinit(card);

    ESP_LOGI(STORE_TAG, "Armazenamento encerrado e arquivo salvo.");
    vTaskDelete(NULL);
}

// TIMERS SECTION ----------------------------------

void vRecTimer(TimerHandle_t xTimerHandle)
{
    xTaskNotifyGive(xTaskReadHandle);
    ESP_LOGI("timer", "Tempo de gravacao acabou.");
}

// FUNCTIONS SECTION --------------------------------

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

    // abre arquivo com nome unico
    audio_file = fopen_unique(MOUNT_POINT "/file", ".raw", "wb");
    if (audio_file == NULL)
    {
        ESP_LOGE(MAIN_TAG, "Falha ao abrir o arquivo");
        return;
    }

    // criacao da fila de dados para envio
    xQueueHandle = xQueueCreate(DMA_BUF_NUM, BUF_SIZE * sizeof(char));
    if (xQueueHandle == NULL)
    { // testar se a criacao da fila falhou
        ESP_LOGE(MAIN_TAG, "Falha em criar fila de dados");
        while (1)
            ;
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
        while (1)
            ;
    }

    // criacao das tasks
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

    // testar se a criacao das tarefas falhou
    for (int i = 0; i < 3; i++)
    {
        if (xReturnedTask[i] == pdFAIL)
        {
            ESP_LOGE(MAIN_TAG, "Erro ao criar a task %d", i);
            while (1)
                ;
        }
    }

    vTaskSuspend(xTaskReadHandle);
    vTaskSuspend(xTaskStoreHandle);
}