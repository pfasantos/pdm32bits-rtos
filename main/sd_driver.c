#include "sd_driver.h"

esp_err_t xSdDriverInit(sdmmc_card_t *pxSdCard)
{
    // host SDSPI
    sdmmc_host_t xHost = SDSPI_HOST_DEFAULT();

    // configurando o bus SPI
    spi_bus_config_t xBusConfig = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 8192,
    };

    // inicializa o bus SPI
    esp_err_t xError = spi_bus_initialize(xHost.slot, &xBusConfig, SPI_DMA_CH_AUTO);
    if (xError != ESP_OK)
    {
        ESP_LOGE(SD_TAG, "Falha ao inicializar o bus SPI (%s)", esp_err_to_name(xError));
        return xError;
    }

    // configurando o slot SPI
    sdspi_device_config_t xSlotConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
    xSlotConfig.gpio_cs = SD_PIN_NUM_CS;
    xSlotConfig.host_id = xHost.slot;

    // configurando o mount do SD card
    esp_vfs_fat_sdmmc_mount_config_t xMountConfig = {.format_if_mount_failed = false,
                                                     .max_files = 5};

    // montagem do SD card
    xError =
        esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &xHost, &xSlotConfig, &xMountConfig, &pxSdCard);
    if (xError != ESP_OK && xError != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(SD_TAG, "Falha ao montar o SD card (%s)", esp_err_to_name(xError));
        return xError;
    }
    ESP_LOGI(SD_TAG, "SD card montado com sucesso em %s", SD_MOUNT_POINT);
    return ESP_OK;
}

esp_err_t xSdDriverDeinit(sdmmc_card_t *pxSdCard)
{
    ESP_LOGI(SD_TAG, "Desmontando SD card");
    esp_err_t xError = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, pxSdCard);

    if (xError != ESP_OK)
    {
        ESP_LOGI(SD_TAG, "SD card ainda montado");
    }

    ESP_LOGI(SD_TAG, "SD card desmontado com sucesso.");
    return ESP_OK;
}