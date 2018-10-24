#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
# VERBOSE = 1
PROJECT_NAME := esp32-audio-app
COMPONENT_ADD_LDFLAGS = -Wl,--Map=map.txt

include $(IDF_PATH)/make/project.mk