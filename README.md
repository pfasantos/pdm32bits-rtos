# pdm32bits-rtos

ESP-IDF firmware for an ESP32-S3 that reads a microphone data stream through I2S standard-mode RX, applies a two-stage CIC and a short PCM post-filter, and writes raw 16-bit samples to a microSD card. It uses separate FreeRTOS reader and storage tasks.

## Requirements

- ESP-IDF environment with `idf.py` available.
- An ESP32-S3 target (`CONFIG_IDF_TARGET="esp32s3"` in the committed `sdkconfig`).
- A microphone and microSD interface wired to the pins below. The final IC report for the project identifies the tested microphone as an SPH0641LU4H-1; the complete electrical wiring is not specified in this repository.
- A microSD card with a FAT filesystem. The mount configuration does not format a card when mounting fails.

## Connections configured in source

| Function | ESP32-S3 GPIO | Source |
| --- | ---: | --- |
| I2S bit clock | 5 | `main/i2s_std.c` |
| I2S data input | 4 | `main/i2s_std.c` |
| microSD SPI MISO | 10 | `main/sd_driver.h` |
| microSD SPI MOSI | 12 | `main/sd_driver.h` |
| microSD SPI clock | 9 | `main/sd_driver.h` |
| microSD chip select | 11 | `main/sd_driver.h` |

The I2S configuration leaves MCLK, WS and data output unused. These are software pin settings, not a complete electrical wiring diagram.

## Build and run

From the repository root in an ESP-IDF shell:

```sh
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with the serial device for your board. The committed configuration targets `esp32s3`; if you create a new configuration, select it with `idf.py set-target esp32s3` before building.

On startup, `app_main()` initializes I2S, mounts the SD card at `/sdcard`, initializes the filters, and creates `/sdcard/file_0.raw` (or the next unused number). The reader task receives I2S data, converts it with `process_app_cic()`, and places sample buffers on a FreeRTOS queue. The storage task applies `process_new_fir()` and writes those buffers to the file. A one-shot software timer requests the reader to stop after `REC_TIME_MS` (60 seconds by default); the reader then notifies the storage task to drain the queue and close the file.

The configured I2S clock starts at 8,000 and is reconfigured to 75,000 before recording. The source uses 32-bit stereo I2S slots. The output file has no WAV header: `fwrite()` stores `short` samples as raw native-endian bytes. The microphone model, effective output sample rate, and channel interpretation are not established by the repository, so those properties should be measured before importing the file as audio.

## Tested hardware and research context

The author's final IC report describes an ESP32-S3, an SPH0641LU4H-1 PDM microphone and a 32 GB SanDisk Extreme A1 microSD card. It reports functional capture of test tones from 20 to 100 kHz, software CIC/FIR conversion, and recording sizes close to the expected volume in the stated experiments. These are results from that experimental setup, not a calibrated microphone response or a guarantee for every board and card. The report did not provide a complete pin-by-pin power and selection wiring diagram; the GPIO table above comes from this repository's source.

## Resource and behavior notes

- The I2S DMA configuration uses 24 descriptors of 511 frames. The queue holds up to 24 items of 4,088 bytes each, or about 96 KiB of queued sample data, plus task stacks, DMA buffers and filter state.
- The two tasks are pinned to different ESP32-S3 cores. Both request `configMINIMAL_STACK_SIZE + 4096` stack units.
- SD writes can block the storage task; a full queue can then block the reader task. The code does not report queue latency or lost samples.
- `sdcard_init()` returns an error on SPI or mount failure, but `app_main()` does not check it. The card pointer is passed by value, so the caller's `card` variable is not set by the mount function; the later unmount call should not be assumed to have a valid card pointer. File writes are also not checked for short writes.

## Source and API documentation

- `main/i2s_std.h`: I2S receive channel settings and lifecycle.
- `main/sd_driver.h`: SPI pins and SD mount/unmount API.
- `main/pdm2pcm.h`: filter structures and conversion API.
- `main/main.h`: recording constants and task entry points.

Generate the Doxygen HTML reference from the repository root with `doxygen Doxyfile`. Open `build/html/index.html`. Generated files stay under the ignored `build/` directory.
