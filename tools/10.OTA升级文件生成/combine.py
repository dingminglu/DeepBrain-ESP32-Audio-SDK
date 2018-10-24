# -*- coding: cp936 -*-
import re
import struct
import time

def get_info(s_src, s_name):
	match_obj = re.search(s_name + "\s+\".+\"", s_src)
	match_obj = re.search("\".+", match_obj.group(0))
	match_obj = re.search("\w+.*\w+", match_obj.group(0))
	
	return match_obj.group()	

if __name__ == "__main__":
	
	#打开头文件
	f_head  = open("../../components/userconfig/userconfig.h", "rb")
	str_head = f_head.read()
	
	name    = get_info(str_head, "PROJECT_NAME")
	version = get_info(str_head, "ESP32_FW_VERSION")
	reserve = ""

	print "PROJECT_NAME     : ", name
	print "ESP32_FW_VERSION : ", version
	ss = struct.pack("64s32s160s", name, version, reserve)
	
	combine_name = name[0:17] + version + '.bin'

	#打开MVA文件
	f_mva = open("../../build/esp32-audio-app.bin", "rb") 

	#创建combine.MVA文件
	f_combine = open(combine_name, "wb")

	#写头
	f_combine.write(ss)

	#写MVA文件
	for line in f_mva:
		f_combine.writelines(line)

	f_combine.flush()
		
	f_head.close
	f_mva.close
	f_combine.close

	print "合并完成"	
	
	time.sleep(2)
