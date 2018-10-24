# ESP-Audio User Guide

ESP Audio is an audio solution for any [ESP32](https://espressif.com/en) based device, which fully intergrates BT, Wi-Fi, TF-card, External RAM, Music Encoder, Music Decoder, DLNA, Airplay (under developing), DuerOS, Tone, Playlist, Up-sampling, Smartconfig and Blufi all together.  
ESP-Audio provides multiple features for users to develop their own audio project with ESP32 chip.
 
This document introduces the whole system of ESP-Audio.

Refer to [ESP-Audio Design Guideline](docs/ESP-Audio_Design_Guideline.md "ESP-Audio Design Guideline") to learn bugs related to ESP32, memory system, peripheral issues, Wi-Fi awareness as well as hardware design guideline that users should know in an ESP32 audio project.

## Contents
[TOC]

## 1. Getting Started
This section introduces how to set up the environment, initialize and build the ESP-Audio SDK.

### 1.1. Software and Hardware Preparation
ESP-IDF and related environments need to be set up to build the ESP-Audio SDK, generate binaries as well as `make flash`.  

Documents and hardware guides are available by following links.
1. Refer to the [ESP-IDF README](https://github.com/espressif/esp-idf "ESP-IDF doc") on github to set up the environment.
2. Refer to the [ESP-IDF Programming Guide](http://esp-idf.readthedocs.io/en/latest/index.html "ESP-32 IDF system") for more information about ESP-IDF system (including hardware and software).  

    > Please pay attention to [Partition Tables](http://esp-idf.readthedocs.io/en/latest/api-guides/partition-tables.html "Partition Tables Section") section.  
    > This is where you could find flash tone and flash playlist as well as the OTA binaries.
    
### 1.2. ESP-Audio Initialization

This section describes how to initialize ESP-Audio system.  
The general initialization is based on [ESP-IDF menuconfig](http://esp-idf.readthedocs.io/en/latest/get-started/index.html#configure "IDF Configure") system.

#### Audio Sdkconfig <a name="config"></a>
The following *sdkconfig* options need to be set with `make menuconfig` command, as they are particularly required by ESP-Audio. 
 
If you want to skip setting these options, please refer to *sdkconfig* in */esp-audio-app* directory.

- To enable BT audio, following options in "Component config -> Bluetooth" should be set:
    - Set *BLUEDROID_ENABLED* as "yes"
    - Set *CLASSIC_BT_ENABLED* as "yes"

> Check *[BT Audio & Wi-Fi Audio](#BtWifi)* section for details.

- To enable Wi-Fi audio: If BT audio is not enabled, Wi-Fi audio will be automatically enabled.  

    > Pay attention to Wi-Fi menuconfig options, once the PSRAM is in use, then the "Type of WiFi Tx Buffer" option must be set as "STATIC", and please be aware of that each static Tx and Rx buffer occupied 1.6KB internal RAM. If PSRAM is not used, then dynamic buffer should be selected to reduce internal RAM.

> Check *[BT Audio & Wi-Fi Audio](#BtWifi)* section for details.


- To enable PSRAM (external RAM):
    - Set *SPIRAM_SUPPORT* in "Component config -> ESP32-specific"

    > Tips for using PSRAM & RAM:  
    Call `char *buf = heap_caps_malloc(512, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` instead of `malloc(512)` to use PSRAM, and call `char *buf = heap_caps_malloc(512, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)` to use internal RAM.  
    Refer to "components/SystemSal/EspAudioAlloc.c"

    > Refer to [ESP-Audio Design Guideline](docs/ESP-Audio_Design_Guideline.md "ESP-Audio Design Guideline") for more details

- To reduce internal RAM: 
    - Set all the *static buffer* to minimum value in "Component config -> Wi-Fi" if PSRAM is in use, if not then *dynamic buffer* should be selected
    - Set *CONFIG_WL_SECTOR_SIZE* as 512 in "Component config -> Wear Levelling

    > Attention:  
    The smaller the size of sector be, the slower the W /R speed will be, vice versa, but only 512 and 4096 are supported.

    - If PSRAM is used, then *CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST* and *CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY* should be set as "yes" to allocate some memory to PSRAM in order to save internal RAM, and only by this can BT audio and Wi-Fi audio coexsit with enough internal RAM
    - If PSRAM is used, then *CONFIG_WIFI_LWIP_ALLOCATION_FROM_SPIRAM_FIRST* should be set as "yes" to allocate some memory to PSRAM in order to save internal RAM

    > Refer to "RAM system" section in [ESP-Audio Design Guideline](./ESP-Audio_Design_Guideline.md "ESP-Audio Design Guideline") to learn the restriction of RAM system. 

- To enable FATFS:
    - Choose a desirable *OEM Code Pade* and enable *long filename* in "Component config -> FAT Filesystem support"
 
- To support other audio features:
    - Select DLNA, OTA, Duer OS and etc. in "Component config -> ESP Audio App"

#### Selecting Audio Components 
Follow comments in *app_main.c* to select desirable features.  
Device Controller and Services are independent components, and any of them can be commented out to save RAM and to reduce binaries' size.

***

## 2. Guidance of Audio Feature
Refer to *EspAudio.h* to learn more about ESP-Audio APIs.

### 2.1. BT Audio & Wi-Fi Audio <a name="BtWifi"></a>
BT audio is completely integrated, which can connect to any BT supported device with bluetooth and play music of that device via ESP32. Music tone and other audio features are also available.

Wi-Fi audio can connect to a Wi-Fi router and play music online, supporting DLNA, AIRPLAY (under developing), Duer OS, Smartconfig and blufi.

By default, BT and Wi-Fi are independent of each other distinguished by *CLASSIC_BT_ENABLED* in *sdkconfig*. Setting *CLASSIC_BT_ENABLED* or not can generate two different binary files indicating BT audio or Wi-Fi audio, and the two different binaries are stored in two areas of flash which can be switched by APIs.   
Users can also modify this project to support BT and Wi-Fi at the same time generating only one binary, however this requires much more RAM. Please refer to [RAM Table](#RAM) for more details.  

#### BT Audio
BT audio is based on A2DP and AVRCP bluetooth profile, and throw the audio data via `EspAudioRawWrite()`.  

To enable BT audio, please refer to [Audio Sdkconfig](#config) section.

#### Wi-Fi Audio
All the socket processes will be done automatically, thus it is very easy for users to play online music. 

```c
//users can play an online music by a single API
EspAudioPlay("http://iot.espressif.com/file/example.mp3");
```

To enable Wi-Fi audio, please refer to [Audio Sdkconfig](#config) section.

### 2.2. Uri Rules
The uri must conform to a certain rule to be passed to `EspAudioPlay()` and other occasions that need a correct rui.  
Here is the quick usage demo in *components/TerminalService/TerminalControlService.c*, please refer to `raw` prefix commands.

URI format: `schem://[name_rate:code_rate@]host[:port][/path][#fragment]`
- `scheme`
    + `http, https` for http stream reader
    + `i2s` for i2s stream reader, usually from MIC
    + `adc` for adc stream reader, not supported yet
    + `file` for TF-Card file reader
- `name_rate:code_rate`(optional)
    + http stream reader username and password
    + or i2s sample rate and channel number 
- `host`
    + http host name 
    + or i2s filename.extension, such as "record_0.wav"
- `port`: http port (optional)
- `path` 
    + http path 
    + i2s record output path (optional)
- `fragment`
    + for http output stream (defaut is i2s)
    + for i2s output 

> Examples:

> *"http://iot.espressif.com:8008/file/example.wav"* play this wav via i2s 
    
> *"file:///example.mp3"* play this mp3 in TF-Card via i2s
    
> *"i2s://16000:1@record.wav:16#file"* record the data from i2s and save as "record.wav" in TF-Card, and specify the i2s sample rate is 16K, 2 channels, 16bits
    
> *"i2s://16000:1@record.pcm#raw"* record the data from i2s and save to buffer, which can be read by `EspAudioRawRead()`
    
> *"raw://from.pcm/to.pcm#i2s"* play the data in buffer via i2s, calling `EspAudioRawWrite()` to play


### 2.3. ESP-Audio Tone
Actually there is no difference between ESP-Audio-Tone and common music, but tone can automatically interrupt and resume music playing. 

The sources of tone include TF-Card, http, flash.  

If the tone files are stored in flash, then a special partition must be added into [Partition Tables](http://esp-idf.readthedocs.io/en/latest/api-guides/partition-tables.html "Partition Tables Section").  

- To generate tone binary file:
	Run */tools/mk_audio_bin.py* to generate a binary file and a header file (music files should be in the same directory)

    > The rule of music names should be like: 00_XXX.MP3, 01_XXX.WAV, 02_XXX.OGG, 03_XXX.MP3, ......
    
- Add "flashTone,  data,  0x04, 0x200000, 1M," into partition_xx.csv

    > The second param must be data, and the third param must be 0x04, other params are flexible
    
### 2.4. Playlist
Playlist provides a cluster of URIs stored in TF-Card, Flash, RAM. Users can use `EspAudioNext()` or `EspAudioPrev()` to select one piece of music before call `EspAudioPlay(NULL)` to play it. The play mode can be Sequential, One Repeating or Shuffle (random).

The flash playlist only works with a special partition in [Partition Tables](http://esp-idf.readthedocs.io/en/latest/api-guides/partition-tables.html "Partition Tables Section").   

- Add "flashUri, data, 0x05, , 0x1000" to the partition_*.csv

    > The second param must be data, and the third param must be 0x05, other params are flexible

### 2.5. Smartconfig
Wi-Fi audio has a special components named *smartconfig* which provides an easy way to connect to a Wi-Fi router passing SSID and password via a mobile device.  
Set *ENABLE_SMART_CONFIG* as "yes" in "Component config -> ESP Audio App" to enable Smartconfig, but incompatible with Blufi.  
Refer to */components/DeviceController/WifiManager* for details.  
Download [ESP-TOUCH APK](http://espressif.com/zh-hans/support/download/apps "ESP-TOUCH APK") for Android as well as IOS.

### 2.6. Blufi
Similar to *smartconfig*, *blufi* also provides an easy way to connect to a Wi-Fi router passing SSID and password via a mobile device.  
Set *CONFIG_ENABLE_BLE_CONFIG* as "yes" in "Component config -> ESP Audio App" to enable Blufi, but incompatible with smartconfig.  
Refer to */components/DeviceController/WifiManager* for details.  
Download [BluFi APK](http://espressif.com/zh-hans/support/download/apps "BluFi APK") for Android as well as IOS.

### 2.7 I2s Output

#### 2.7.1. External Codec
ESP-Audio project uses external codec to output analog signal to speakers by default, and the driver codes are released and available in "/components/MediaHal/Driver" directory.

#### 2.7.2. Internal DAC
Two 8-bit DAC are integrated into ESP32 chip, so these two DAC can be used to output audio if external codec is not available and low-quality audio is acceptable.  
Refer to "components/MediaHal/MediaHal.c" to learn how to enable i2s-DAC.  
> GPIO25 and GPIO26 are the immutable two IO of ADC output, GPIO25 and GND or GPIO26 and GND should be connected to a speaker. 

***

## 3. Debug

### 3.1. ESP_LOG
All the log printed via UART serial interface is a special log system, where `ESP_LOGI` will print green strings started with "I" indicating this is one piece of information while `ESP_LOGW` will print yellow strings started with "W" indicating this is a warning, and `ESP_LOGE` will print red strings started with "E" indicating this is an error.

> Examples:  
"I (46999) I2S_STREAM: SLIENT": information form ESP_LOGI  
"W (40404) SOFT_CODEC: CodecStop, have no codec instance,0x0": warning form ESP_LOGW  
"E (40569) FLASH_STREAM: Lost flash tone header": error from ESP_LOGE

Pay attention to `ESP_LOGE` log, because those may be fatal errors, but some errors show up just because these features are not selected and should not be taken seriously, for instance, "E (40569) FLASH_STREAM: Lost flash tone header" shows up because flash-tone partition table is not added, and this can be a "normal" error if flash-tone is not wanted.

### 3.2. JTAG, GDB 
If the TF-Card is in SDIO mode, then the [JTAG Debugging](http://esp-idf.readthedocs.io/en/latest/api-guides/jtag-debugging/index.html "JTAG Debugging") can not be used becuase the GPIOs are not incompatible with SDIO, instead [GDBStub](http://esp-idf.readthedocs.io/en/latest/get-started/idf-monitor.html# "IDF Monitor") can be used to support some gdb features via uart.

*ESP32_LyraXX* board has a selector to switch from SDIO to JTAG.

Refer to [JTAG Debugging](http://esp-idf.readthedocs.io/en/latest/api-guides/jtag-debugging/index.html "JTAG Debugging") for more information including guideline on using [gdb](https://sourceware.org/gdb/ "gdb home page").
    
### 3.3. Core Dump
[ESP32 Core Dump](http://esp-idf.readthedocs.io/en/latest/api-guides/core_dump.html "Coredump dubug") is to generate core dumps on unrecoverable software errors. This useful technique allows post-mortem analysis of software state at the moment of failure. 

```c
//following command can be used to analysis the core dump file named info_corefile 
python $IDF_PATH/components/espcoredump/espcoredump.py info_corefile -t b64 -c coredump.txt build/esp32-audio-app.elf
```

JTAG or GDB is more recommended than Coredump.

***

## 4. RAM Table <a name="RAM"></a>

Here is the table contains every component and its memory usage. Choose the components needed and find out how much internal RAM is left.  

The *Components RAM Table* is divided into two parts when PSRAM is used or not.

**The initial spare internal RAM is 290K.**

### 4.1. RAM Table Without PSRAM 
Add all the RAM components needed, and the result is how much ram is needed all together.

|**Components**|**Extra RAM Needed**|
---|:---:|
Wi-Fi |50KB+[^1]
BT |140KB (50KB if only BLE needed)
TF-Card |12KB+[^2]
I2s |Configurable, 8KB for reference[^3]
RingBuffer |Configurable, 30KB for reference[^4]
DLNA |35KB[^5]
Duer OS |TO BE CHECK
Playlist |5KB

*Music RAM Table* is different from the above one. Users only need to select the music needed and the maximum RAM among them is how much extra RAM is needed, because there is at most only one music playing at any time.

|**Music Type**|**Extra RAM Needed**|
---|:---:|
OPUS | -
AAC, M4A |136K(28KB)[^6]
FLAC |126KB
MP3 |52KB (41KB)[^7]
WAV |20KB
OGG |191KB
AMR |23KB

### 4.2. Internal RAM Table With PSRAM 
If PSRAM (external RAM) is in use, then some of the memory will be allocated at PSRAM automatically.

Add all the RAM components needed, and the result is how much internal ram is needed all together.

|**Components**|**Extra Internal RAM Needed**|
---|:---:|
Wi-Fi |50KB+[^1]
BT |95KB (50KB if only BLE needed)
TF-Card |10KB+[^2]
I2s |Configurable, 8KB for reference[^3]
RingBuffer |0KB, all moved into PSRAM[^4]
DLNA |29KB[^5]
Duer OS |TO BE CONFIRMED
Playlist |5KB

*Music RAM Table* is different from the above one. Users only need to select the music needed and the maximum RAM among them is how much extra RAM is needed, because there is at most only one music playing at any time.

|**Music Type**|**Extra Internal RAM Needed**|
---|:---:|
OPUS |38KB
AAC, M4A |28KB
FLAC |11KB
MP3 |12KB
WAV |10KB
OGG |17KB
AMR |12KB

[^1]: According to the Wi-Fi menuconfig each Tx and Rx buffer occupy 1.6KB internal RAM. 50KB RAM is the result of this setting: 5 Rx static buffers and 6 Tx static buffers. If PSRAM is not in use, then the "Type of WiFi Tx Buffer" option should be set as "DYNAMIC" in order to save RAM, in this case, the RAM usage will be far less than 50KB, but users should think of that 50KB, at least, is always needed in case the Wi-Fi is transmitting chunks of data. **[Internal RAM only]**
[^2]: Basically according to the `max_files` param in `sd_card_mount()` function, the RAM needed will increase with a greater number of max_files. 12KB is the RAM needed with 5 max files and 512 bytes *CONFIG_WL_SECTOR_SIZE*. **[Internal RAM only]**
[^3]: According to the I2S configuration, refer to `i2s_config` in *MediaHal.c*. **[Internal RAM only]**
[^4]: According to the Ringbuffer configuration refer to `playerConfig` in *MediaHal.c*
[^5]: 35KB doesn't include the RAM needed when playing a music. For example, it will need extra 52KB when playing MP3.
[^6]: AAC and M4A use the same decoder library. There is a very special aac, m4a library that only needs 28KB RAM while decoding, but at the expense of dropping SBR and PS components of some high quality aac, m4a music. This quality loss is usually acceptable and even can not be discovered. The high efficient aac, m4a library that only uses 28KB RAM is located at */tools/Performance Codec Library/aac m4a/libesp-fdk.a*, replace the one in *components/esp-codec-lib/lib* if the RAM and the efficiency is prior to quality. (Currently users don't have to do this because the high-efficiency version is used by default)
[^7]: There is a special mp3 library that only needs 41KB RAM at the expense of decoding only one mp3 channle, despite there may be two channles. This library is located at */tools/Performance Codec Library/mp3/libesp-stagefright-mono.a*, rename it to *libesp-stagefright.a* and replace the one in *components/esp-codec-lib/lib* as well as the header file named *StageFrightMp3Decoder.h*.
