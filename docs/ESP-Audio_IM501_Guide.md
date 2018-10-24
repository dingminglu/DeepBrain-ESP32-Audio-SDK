# ESP-Audio DSP Guide
 
This document introduces how to use fortemedia IM501 DSP in ESP-Audio SDK.

## 1. Preparation
Refer to [ESP-Audio README](../README.md "Audio Programming Guide") to learn how to use ESP-Audio SDK before the document.

- PSRAM must be used. Refer to [ESP-Audio Programming Guide](ESP-Audio_Programming_Guide.md "Audio Programming Guide") to learn how to do this.
- The dual core mode must be enabled to support to write the record data to TF-Card if want to test AEC performance.

 > To enable dual core mode: "Reserve memory for two cores" option must be selected and "Run FreeRTOS only on first core" must NOT be selected in menuconfig.

***

## 2. DSP Firmware
The firmware is provided under the path `components/MediaHal/DSP/im501/forte_bin`.

### 2.1. Download The Firmwares
Using the following command to flash the firmware, while the addresses of the firmware should be modified according to [partitions_esp_audio.csv](../partitions_esp_audio.csv "ESP-Audio partitions").

```
python /home/share/IDF/audio-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x310000 components/MediaHal/DSP/im501/forte_bin/iram0.bin 0x330000 components/MediaHal/DSP/im501/forte_bin/dram0.bin 0x350000 components/MediaHal/DSP/im501/forte_bin/dram1.bin
```

### 2.2. Voice Trigger
The default voice trigger words are "ni hao xiao yi".  
Please contact with espressif to learn how to use new trigger words.

