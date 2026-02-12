#include "i2s_std.h"
#include "sd_driver.h"
#include "main.h"


//TASKS SECTION ----------------------

void vTaskRead (void * pvParameters)
{
    //abre arquivo com nome unico
    audio_file = fopen_unique(MOUNT_POINT "/file", ".raw", "wb");
    if (audio_file == NULL){
        ESP_LOGE(MAIN_TAG, "Falha ao abrir o arquivo");
        return;
    }

    // inicia canal i2s
    i2s_channel_reconfig_std_clock(rx_handle, &clk_rec_cfg);
    i2s_channel_enable(rx_handle);

    while(1)
    {
        // esperar para que o buffer de entrada (in_buffer) seja totalmente preenchido
        if (i2s_channel_read(rx_handle, (void*) rx_buffer, BUF_SIZE, NULL, portMAX_DELAY) == ESP_OK) {
            xQueueSend(xQueueHandle, &rx_buffer, portMAX_DELAY); // enfileirar dados lidos para a tarefa de envio
        } else {
            ESP_LOGE(I2S_TAG, "Erro durante a leitura: errno %d", errno);
            break;
        }
    }

    vTaskDelete(NULL);

}

void vTaskStore (void * pvParameters)
{
    while (1) {
        if((xQueueHandle!=NULL) && (xQueueReceive(xQueueHandle, &st_buffer, 0)==pdTRUE)){ //esperar que dados sejam lidos
            int written = fwrite(st_buffer, 1, BUF_SIZE, audio_file);
            if (written == 0){
                ESP_LOGE(STORE_TAG, "Erro durante a escrita: errno %d", errno);
            }
        }
    }
    
    vTaskDelete(NULL);
}

// FUNCTIONS SECTION --------------------------------

FILE* fopen_unique(const char *base_path, const char *ext, const char *mode) {
    char file_path[128];
    struct stat st;
    int index = 0;

    while (1) {
        // Construct the filename: base_path + "_" + index + ext
        snprintf(file_path, sizeof(file_path), "%s_%d%s", base_path, index, ext);

        // Check if file exists
        if (stat(file_path, &st) == 0) {
            index++;
        } else {
            break;
        }
    }

    ESP_LOGI("FILE_SYS", "Opening file: %s", file_path);
    return fopen(file_path, mode);
}


// MAIN SECTION -----------------------
void app_main(void)
{
    i2s_init();
    sdcard_init();

    //criacao da fila de dados para envio
    xQueueHandle = xQueueCreate(DMA_BUF_NUM, BUF_SIZE*sizeof(char));
    if(xQueueHandle == NULL){ // testar se a criacao da fila falhou
        ESP_LOGE(MAIN_TAG, "Falha em criar fila de dados");
        while(1);
    }

    //criacao das tasks
    BaseType_t xReturnedTask[2];

    xReturnedTask[0] = xTaskCreatePinnedToCore(
        vTaskRead,
        "taskREAD",
        configMINIMAL_STACK_SIZE+2048,
        NULL,
        configMAX_PRIORITIES-1,
        &xTaskReadHandle,
        APP_CPU_NUM
    );

    xReturnedTask[1] = xTaskCreatePinnedToCore(
        vTaskStore,
        "taskSTORE",
        configMINIMAL_STACK_SIZE+2048,
        NULL,
        configMAX_PRIORITIES-1,
        &xTaskStoreHandle,
        PRO_CPU_NUM
    );

    // testar se a criacao das tarefas falhou
    if(xReturnedTask[0] == pdFAIL || xReturnedTask[1] == pdFAIL){
        ESP_LOGE(MAIN_TAG, "Falha em criar tarefas");
        while(1);
}