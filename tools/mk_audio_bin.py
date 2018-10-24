import sys
import os
import struct


if __name__=="__main__":

    ftype = {"mp3":0,
             "wav":1}
    file_list = os.listdir("./")
    #print file_list
    
    #file_list = [x for x in file_list if ((not x.endswith(".py")) and (not x.endswith(".bin")) and (not x.endswith(".h")))]
    file_list = [x for x in file_list if ((x.endswith(".wav")) or (x.endswith(".mp3")))]
    fnum = len(file_list)
    head_len = 8 + 64 * fnum
    
    print "file_list:", file_list
    print "fnum: ", fnum
    print "header_len:", head_len
    
    bin_file = ''
    bin_file = struct.pack("<HHI",0x2053,fnum,0x0)
    
    source_file = '/*This is tone file*/\r\n\r\nconst char* tone_uri[] = {\r\n'
    include_file = '#ifndef __TONEURI_H__\r\n'+ '#define __TONEURI_H__\r\n\r\n'
    include_file += 'extern const char* tone_uri[];\r\n\r\n'
    include_file += 'typedef enum {\r\n'

    song_bin = ''
    
    cur_addr = head_len
    songIdx = 0
    for fname in file_list:
        f = open(fname,'rb')
        print "fname:", fname
        
        #songIdx = int(fname.split("_")[0])
        fileType = fname.split(".")[-1].lower()
        
        print "song index: ", songIdx
        print "file type:", fileType
        
        source_file += '   ' + '"'+ "flsh:///"+ str(songIdx) + '_' + fname + '"'+ ',\r\n'
        include_file += '    ' + 'TONE_TYPE_' + fname.split(".")[0].upper() + ',\r\n'
        songLen = os.path.getsize(fname)
        print "file len:", songLen
        
        songAddr = cur_addr
        print "songAddr:", songAddr
        RFU = [0]*12
        
        
        print "songLen:", songLen
        bin_file += struct.pack("<BBBBII"  ,0x28            #file tag
                                           ,songIdx         #song index
                                           ,ftype[fileType]
                                           ,0x0             #songVer
                                           ,songAddr
                                           ,songLen          #song length
                                           )
        bin_file += struct.pack("<12I",*RFU)
        bin_file += struct.pack("<I",0x0 #CRC
                                )
                                           #,*RFU
                                           #,0x0 #INFO CRC 
                                           #)
        
        song_bin += f.read()
        songIdx += 1
        pad = '\xff'*((4-songLen%4)%4)
        song_bin+=pad
        cur_addr += (songLen + ((4-songLen%4)%4))
        
        print "----------"
        print "bin len:",len(bin_file)
        print "----------"
        f.close()
        pass
    
    bin_file += song_bin
    f= open("audio-esp.bin", "wb")
    f.write(bin_file)
    f.close()      
    
    source_file += "};"

    _func_str = """

int getToneUriMaxIndex()
{
    return sizeof(tone_uri) / sizeof(char*) - 1;
}
                """
    source_file += _func_str

    f= open("toneuri.c", "wb")
    f.write(source_file)
    f.close()

    _end_str = """    TONE_TYPE_MAX,
} ToneType;

int getToneUriMaxIndex();

#endif /* __TONEURI_H__ */
"""

    include_file += _end_str
    f= open("toneuri.h", "wb")
    f.write(include_file)
    f.close()       
        
    #data= bytearray(bin_file)
    #for idx in range(len(data)):
    #    _d = data[idx]
    #    print "%02x"%_d,
    #    if (idx+1)%16==0:
    #        print ""
    #print "bin len:", len(data)
        
