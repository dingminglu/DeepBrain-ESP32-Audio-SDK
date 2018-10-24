#if abc
#define STORAGE_SIZE (FRAME_LEN * 4)

void recordTask(void *pv)
{
    char readPtr[STORAGE_SIZE] = {0};
    char buffer3[STORAGE_SIZE/2] = {0};

    //printf("free heap: %d\r\n", xPortGetFreeHeapSizeCaps(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    void *pChannel;

    int i, ret=0, count = 0;
	//系统启动后只需初始化一次，且只能一次
    YQSW_CreateChannel(&pChannel,"none");
    YQSW_ResetChannel(pChannel);
	
    while(1){
        i2s_read_bytes(0, readPtr, STORAGE_SIZE, portMAX_DELAY);

		//只取双声道中的一个声道数据
		for (i = 0; i < STORAGE_SIZE / 2; i += 2) {
			*((short*)(buffer3 + i)) = *((short*)(readPtr + i * 2));
		}
	
		//获取平台录音数据后（录音参数需设置为 16k 采样，16bit，单声道）
    	//把每一帧的录音数据送到唤醒引擎处理，如果发现了预定义唤醒或命令词，则返回1
		ret = YQSW_ProcessWakeup(pChannel, (int16_t*)(buffer3 + count), FRAME_LEN);
		if (ret == 1){
			int id = YQSW_GetKeywordId(pChannel);
			//成功发现后，需重置下
			YQSW_ResetChannel(pChannel);
			printf("### spot keyword, id = %d\n", id);                   
		}
	
    }
	
    YQSW_DestroyChannel(pChannel);	

    vTaskDelete(NULL);
}

nvs_flash_init();
xTaskCreate(&recordTask, "recordTask", 1024*8, NULL, 5, NULL);
#endif