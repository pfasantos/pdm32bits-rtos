/** @file sd_driver.h
 *  @brief SPI pin assignments and FAT microSD mount lifecycle.
 */
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

#define PIN_NUM_MISO 10 
#define PIN_NUM_MOSI 12
#define PIN_NUM_CLK  9
#define PIN_NUM_CS   11

/** @brief Initialize the SPI bus and mount a FAT microSD card at MOUNT_POINT.
 *  @param card Current pointer value passed to the mount helper.
 *  @return ESP_OK after a successful mount or ESP_ERR_INVALID_STATE from the
 *  mount call; otherwise the first SPI or mount error returned by ESP-IDF.
 *  @warning The argument is passed by value. The card pointer assigned by
 *  esp_vfs_fat_sdspi_mount() is not returned to the caller, so passing the
 *  caller's card variable does not initialize it.
 */
esp_err_t sdcard_init(sdmmc_card_t *card);
/** @brief Request unmount of the FAT microSD card.
 *  @param card Card pointer to pass to the ESP-IDF unmount function.
 *  @return ESP_OK as currently implemented, including when unmount fails.
 *  @warning A valid card pointer is required for an effective unmount.
 */
esp_err_t sdcard_deinit(sdmmc_card_t *card);
#endif // _SD_DRIVER_H_