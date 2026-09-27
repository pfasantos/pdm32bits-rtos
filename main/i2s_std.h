/** @file i2s_std.h
 *  @brief I2S standard-mode receive channel settings and lifecycle.
 *
 *  The channel uses 32-bit stereo slots with an external data input.
 *  Initialization and shutdown functions currently do not return ESP-IDF errors.
 */
#ifndef I2S_STD_H
#define I2S_STD_H

#include "sdkconfig.h"

#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2s_pdm.h"
#include "driver/gpio.h"

// tags
#define I2S_TAG "i2s"

// macros
#define I2S_BIT_DEPTH I2S_DATA_BIT_WIDTH_32BIT // i2s bit depth
#define I2S_BUFFER_SIZE (2U * CONFIG_PDM_DMA_FRAME_COUNT * I2S_BIT_DEPTH / 8U)

/** Receive channel handle created by vI2SStdInit(); valid until vI2SStdStop(). */
extern i2s_chan_handle_t xRxHandle;

/** @brief Create and configure the I2S RX channel.
 *  @warning The implementation does not check driver return codes. Check the
 *  ESP-IDF log and channel state before relying on a successful setup.
 */
void vI2SStdInit(void);
/** @brief Disable and delete the I2S RX channel.
 *  @pre The channel was created by vI2SStdInit() and enabled for recording.
 */
void vI2SStdStop(void);

#endif // I2S_STD_H
