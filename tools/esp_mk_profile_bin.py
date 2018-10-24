'''
profile.bin format is
magic(4B little-edian)len(4B with little-edian) + profile

usage:
put the profile under the profiles directory
$ python mk_profile_bin.py
'''

import sys
import os
import struct

if __name__=="__main__":

    sub_dir = "./profiles/"
    file_list = os.listdir(sub_dir)

    fnum = len(file_list)

    print "file_list:", file_list
    print "fnum: ", fnum

    for fname in file_list:
        f = open(sub_dir + fname, 'rb')
        print "fname:", fname

        profile_len = os.path.getsize(sub_dir + fname)
        print "file len:", profile_len
        bin_file = ''
        bin_file = struct.pack("<BBBBBBBB", 0xA5, 0x5A, 0x5A, 0xA5, profile_len&0xFF, profile_len>>8&0xFF, 0, 0)

        profile_bin = f.read()
        f.close()

        gen_name = fname + '.bin'
        bin_file += profile_bin
        f = open(sub_dir + gen_name, "wb")
        f.write(bin_file)
        print "----------"
        print "bin len:",len(bin_file)
        print "----------"
        f.close()
        pass
