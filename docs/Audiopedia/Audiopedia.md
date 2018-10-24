# TO BE DONE...

#### Audio Stream config
Audio Stream is a basic idea that embedded different streams in ESP-Audio system, which can be catalogued into output stream and input stream.

Users will see all the streams in `stuct playerConfig`.
- IN_STREAM_I2S, this is recording stream that records data from mic.
- IN_STREAM_HTTP, this http stream can support fetching data from a website and play it.
- IN_STREAM_LIVE, supports m3u8 stream playing.
- IN_STREAM_FLASH, support flash music playing. Refer to [Flash Music](#Flash_Music) for details.
- IN_STREAM_SDCARD, support TF-Card playing. Refer to [SDCardManager](#SDCardManager) section.
***
- OUT_STREAM_I2S,  

