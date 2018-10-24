/* 
 * ESPRESSIF MIT License 
 * 
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD> 
 * 
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case, 
 * it is free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished 
 * to do so, subject to the following conditions: 
 * 
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * 
 */

#ifndef __PLAYLISTNAMEDEF_H__
#define __PLAYLISTNAMEDEF_H__

#define PLAYLIST_NUM_OPENED_LISTS_MAX    4 //how many playlists can be opened simultaneously

typedef enum PlayListId {

    /*!< playlists in flash */
    //the following value must equal the respective flashUri-partition's SubType in partition table, and must >= 3 according to idf partition doc
    PLAYLIST_IN_FLASH_DEFAULT = 5, //reserved by system, do not delete
    PLAYLIST_IN_FLASH_02,
    //add your TF-Card playlist id before `PLAYLIST_IN_CARD_PRIMARY`

    /*!< playlists in TF-card */
    PLAYLIST_IN_CARD_PRIMARY, //reserved by system, do not change
    PLAYLIST_IN_CARD_02,
    PLAYLIST_IN_CARD_03,
    //add your TF-Card playlist id before `PLAYLIST_MAX`

    PLAYLIST_MAX,/* add your sd flash playlist before this */
} PlayListId;

typedef int (*ISCODECLIB)(const char *ext);//for system, do not delete

int getPlayListsNum(PlayListId id);
int getDefaultSdIndex();
int getDefaultFlashIndex();
int getMaxIndex();

#endif /* __PLAYLISTNAMEDEF_H__ */

