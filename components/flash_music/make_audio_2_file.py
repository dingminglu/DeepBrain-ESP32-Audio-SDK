import sys
import os
import struct


if __name__=="__main__":
    
    ftype = {"mp3":0}
    file_list = os.listdir("./")
    file_list.sort() 
    #print file_list
    
    file_list = [x for x in file_list if ((x.endswith(".wav")) or (x.endswith(".mp3")))]
    fnum = len(file_list)
    
    print "file_list:", file_list
    print "fnum: ", fnum

    f_include = open("array_music.h", "wb")
    f_source = open("array_music.c", "wb")

    include_file = "#ifndef __ARRAY_MUSIC_H__\r\n"
    include_file += "#define __ARRAY_MUSIC_H__\r\n"
    include_file += " #include <stdio.h>\r\n"
    include_file += "#include <string.h>\r\n"
    include_file += "\r\ntypedef struct\r\n"
    include_file += "{\r\n"
    include_file += "    int audio_len;\r\n"
    include_file += "    char *audio_data;\r\n"
    include_file += "    char *audio_name;\r\n"
    include_file += "} FLASH_MUSIC_DATA_T;\r\n"
    include_file += "\r\ntypedef enum\r\n{\r\n"
    include_file_array = "\r\nFLASH_MUSIC_DATA_T flash_music_array[] = {\r\n"
    f_source.write("#include \"array_music.h\"\r\n")
    
    file_array = "\r\nFLASH_MUSIC_DATA_T flash_music_array[] = {\r\n"
    for fname in file_list:
        f = open(fname,'rb')
        print "fname:", fname
		
	#define source file and include file name
        
        include_file_name = fname.split(".")[0].lower() + ".h"
        source_file_name = fname.split(".")[0].lower() + ".c"
        source_file = "\r\nconst char flash_music_" + fname.split(".")[0].lower() + "[]={\r\n"
        include_file += "    FLASH_MUSIC_"+ fname.split(".")[0].upper() +",\r\n"
        
		
        print "include_file_name:", include_file_name
        print "source_file_name:", source_file_name
        #print "include_file:", include_file
        
        data= bytearray(f.read())
        file_array += "    {"+ str(len(data)) +", flash_music_"+ fname.split(".")[0].lower() +", \"array:///"+fname.split(".")[0].lower()+".mp3\"},\r\n"
        for idx in range(len(data)):
            _d = data[idx]
            source_file += "0x%02X"%(_d)
            
            if idx != (len(data) - 1):
                source_file += ","
            
            if (idx+1)%32 == 0:
                source_file += "\r\n"
                
            if idx == (len(data) - 1):
                source_file += "};\r\n"
            
        #print "source_file:", source_file
        f.close()
        
        f_source.write(source_file)
        
        pass
    
    file_array += "};\r\n\r\n"
    f_source.write(file_array)
    
    function_str = "\r\nint get_flash_music_max_subscript()\r\n{\r\n    return sizeof(flash_music_array) / sizeof(FLASH_MUSIC_DATA_T) - 1;\r\n}\r\n"
    function_str += "\r\nchar *get_flash_music_data(int index)\r\n{\r\n    return flash_music_array[index].audio_data;\r\n}\r\n"
    function_str += "\r\nchar *get_flash_music_name(int index)\r\n{\r\n    return flash_music_array[index].audio_name + strlen(\"array:///\");\r\n}\r\n"
    function_str += "\r\nchar *get_flash_music_name_ex(int index)\r\n{\r\n    return flash_music_array[index].audio_name;\r\n}\r\n"
    function_str += "\r\nint get_flash_music_size(int index)\r\n{\r\n    return flash_music_array[index].audio_len;\r\n}\r\n"
    f_source.write(function_str)
    
    include_file += "}FLASH_MUSIC_INDEX_T;\r\n\r\n"
    include_file += "\r\nchar *get_flash_music_data(int index);\r\n"
    include_file += "\r\nchar *get_flash_music_name(int index);\r\n"
    include_file += "\r\nchar *get_flash_music_name_ex(int index);\r\n"
    include_file += "\r\nint get_flash_music_size(int index);\r\n"
    include_file += "\r\nint get_flash_music_max_subscript();\r\n"
    include_file += "\r\n#endif /*__ARRAY_MUSIC_H__*/\r\n"
    f_include.write(include_file)
    
    f_include.close()
    f_source.close();
        
