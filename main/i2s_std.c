#include "i2s_std.h"

i2s_chan_handle_t rx_handle;

void i2s_init()
{
    // configurações do canal
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_AUTO,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = DMA_BUF_NUM,   // quantidade de buffers do DMA
        .dma_frame_num = DMA_BUF_SIZE, // tamanho dos buffers do DMA
        .auto_clear = false,
    };

    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    // configuracoes de clock slot e gpio
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(8000), // inicia num clock menor
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(BIT_DEPTH, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_21,
            .ws = I2S_GPIO_UNUSED,
            .dout = I2S_GPIO_UNUSED,
            .din = GPIO_NUM_4,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // inicializar o canal i2s no modo std
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ESP_LOGI(I2S_TAG, "Canal I2S iniciado.");
}

void i2s_stop()
{
    i2s_channel_disable(rx_handle);
    i2s_del_channel(rx_handle);
    ESP_LOGI(I2S_TAG, "Canal I2S desativado.");
}