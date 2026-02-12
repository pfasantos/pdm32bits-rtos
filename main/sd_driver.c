#include "sd_driver.h"

esp_err_t sdcard_init (void)
{
    //host SDSPI
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    //configurando o bus SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 8192,
    };

    //inicializa o bus SPI
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
        if (ret != ESP_OK){
        ESP_LOGE(SD_DRIVER_TAG, "Falha ao inicializar o bus SPI (%s)", esp_err_to_name(ret));
        return ret;
    }

    //configurando o slot SPI
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = PIN_NUM_CS;
    slot_cfg.host_id = host.slot;

    //configurando o mount do SD card
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
    .format_if_mount_failed = false,
    .max_files = 5
    };

    //montagem do SD card
    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_cfg, &mount_cfg, &card);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE){
        ESP_LOGE(SD_DRIVER_TAG, "Falha ao montar o SD card (%s)", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(SD_DRIVER_TAG, "SD card montado com sucesso em %s", MOUNT_POINT);
    return ESP_OK;
}