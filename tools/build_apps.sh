#!/bin/bash

echo "build audio apps"

CONFIG_FOLDER="tools"
SDK_CONFIG="sdkconfig"
SDK_CONFIG_DEFAULTS="sdkconfig.defaults"

CONFIG_FILES=`ls $CONFIG_FOLDER | grep .defaults`

for CONFIG in $CONFIG_FILES;
do
    APP_NAME=`echo $CONFIG | awk -F '.' '{print $1}'`
    echo "building $APP_NAME"
    rm -f $SDK_CONFIG
    cp -f $CONFIG_FOLDER/$CONFIG $SDK_CONFIG_DEFAULTS
    make defconfig || exit 1
    make || exit 1
    make print_flash_cmd | tail -n 1 > build/download.config

    BIN_FOLDER="app_bin/$APP_NAME/"
    mkdir -p $BIN_FOLDER
    cp tools/audio-esp.bin $BIN_FOLDER
    cp build/*.bin $BIN_FOLDER
    cp build/*.elf $BIN_FOLDER
    cp build/*.map $BIN_FOLDER
    cp build/download.config $BIN_FOLDER
    mkdir $BIN_FOLDER/bootloader
    cp build/bootloader/*.bin $BIN_FOLDER/bootloader
    # also copy sdkconfig to dest bin folder
    cp sdkconfig $BIN_FOLDER/sdkconfig
done
