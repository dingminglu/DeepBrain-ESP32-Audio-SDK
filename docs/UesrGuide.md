# Basic
- Base SDK PATH :https://github.com/espressif/esp-idf

# Player Version 0.9-rc1 [Date:2018-05-25]
- DuerOS 3.0 使用点按 REC 进行交互
- 支持了LyraT_V4.3, 编译时根据`userconfig.h`中的`LYRA_T_4_3`进行配置
- [LyraT 使用手册](https://www.espressif.com/sites/default/files/documentation/esp32-lyrat_user_guide_cn.pdf)

# Player Version 0.6 [Date:2017-12-22]
- Rec按键，按住，录音写入SDcard，松开回放刚才录音的内容
- Specific IDF provided

# Player Version 0.5 [Date:2017-09-01]
- 参见0.5.0

# Player Version 0.5.0-rc2 [Date:2017-08-04]
- 参见0.5.0

# Player Version 0.5.0-rc1 [Date:2017-07-20]
- 参见0.5.0

# Player Version 0.5.0 [Date:2017-06-13]
- Play按键支持了短按play-pause
- Mode按键，短按会切换列表播放模式（单曲-列表-随机），长按在烧写了BT和WIFI Bin时可以实现模式切换；
- Set按键，长按进入配网模式
- Rec按键，按下进入录音模式，松开回放刚才录音的内容

# Player Version 0.2.0 [Date:2017-05-12]
## Download
### Lyra32T_V2 refer to Player Version 0.1.0
### Lyra32T_V2.1
- Hold boot button and press reset enter download mode
### Lyra32T_V3.1
- Hold boot button and press reset enter download mode


# Player Version 0.1.3 [Date:2017-03-28]

## Hardware guide
### Lyra32T_V2 使用方法参加【Date:2017-03-03版本】
### LyraT_V3.1(板子背面右下角标注) 使用方法如下：
#### 下载方法：
- 1.下载工具点下载按钮或者 make flash
- 2.按住 Boot 键，点按RST 按键
- 3.开始下载后松开 Boot 按键
#### 支持按键：
- 1.Play 短按：播放
- 2.Play 长按：录音并回放
- 3.Set 长按：配网
- 4.Vol+ 短按：增加音量
- 5.Vol+ 长按：下一曲
- 6.Vol- 短按：减小音量
- 7.Vol- 长按：上一曲

## Known issues
- 1.BT连接时会有一定几率reset

## 编译方法
- 1. git clone https://github.com/espressif/esp-idf，切换到commit 8ee6f8227e1ff3dd601a7005f7cc7564809082a8
- 2. git apply 8ee6f82_Ver0_1_3.patch
- 3. make menuconifg enable WIFI 或者BT
- 4. Enable psram


# Player Version 0.1.0 [Date:2017-03-03]
## 使用方法
- 1.取消JP3条线帽，
- 2.使用下载工具或者Make flash下载BT.Bin或者WIFI.bin partition.bin bootloader.bin
- 3.若需要播放SD Card的歌曲，下载完成需要连接JP3
- 4.Lyra32T_V2 默认支持的Touch有Rec，Vol＋，Vol－

## 编译方法
- 1. git clone https://github.com/espressif/esp-idf，切换到commit IDF：c62ae777c262aca1ad7b70c953e4c9ddde5df764
- 2. git apply EspAudioVers01IDF.patch
- 3. make menuconifg enable WIFI 或者BT
- 4. Enable psram；

## 已知问题
- 1.有的板子会有一定噪音；

## Lyra32T_V2硬件有如下Note：
- 1.移除Q2三极管，不使用自动下载模式，下载时按住BOOT按钮
- 2.移除R118可以使能SET touch，支持smartconfig
- 3.移除C17电容
