#ifndef _SD_DRIVER_H_
#define _SD_DRIVER_H_

#include "esp_err.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#define SD_DRIVER_TAG "sd_driver"

#define SPI_DMA_CHAN 1
#define MOUNT_POINT "/sdcard"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   22

esp_err_t sdcard_init(void);
#endif // _SD_DRIVER_H_