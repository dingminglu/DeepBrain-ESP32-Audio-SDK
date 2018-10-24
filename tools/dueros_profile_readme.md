## 1. 概述 
- 乐鑫官方开发板默认携带了一个独立的profile在 flash 0x3FE000 的地址
- SDK Versio 0.93 包含及以后的版本支持 flash 和 SDCARD 加载，默认先检查flash profile 区域是否有效，然后检查SDCARD根目录下是否有名为profile的文件，两者都没有 DuerOS 会启动失败

## 2. 更新Profile
### Download Profile To Flash
- 在 SDK根目录的tools下`esp_mk_profile_bin.py` 可以把profile生成为BIN
    + 先在tools目录下创建 `profiles`的文件夹
    + 拷贝你自己的profile到 `profiles`文件夹下
    + `python esp_mk_profile_bin.py` 运行脚本会生成同名的bin文件
- 下载profile的BIN文件到`0x3FE000 (size-4K)`地址,参见`partitions_esp_audio.csv`

### SDCARD Profile
- 拷贝profile文件到SDCARD根目录，并命名为`profile`
- 擦除 flash profile 区域`Offset:0x3FE000, Size:0x1000`

## 3. Profile 量产工具 
乐鑫已开发了支持Profile的量产工具，如有需要请联系乐鑫的商务
