#
# Component Makefile
#
# This Makefile should, at the very least, just include $(SDK_PATH)/Makefile. By default,
# this will take the sources in the src/ directory, compile them and link them into
# lib(subdirectory_name).a in the build directory. This behaviour is entirely configurable,
# please read the SDK documents if you need to do this.
#

COMPONENT_ADD_INCLUDEDIRS := include/esp-opus include/esp-amr include/esp-amrwbenc include/esp-fdk include/esp-flac \
                            include/esp-aac include/esp-ogg include/esp-share include/esp-stagefright include/esp-tremor include/audio_signal_process

COMPONENT_SRCDIRS := .

LIBS := esp-opus esp-mp3 esp-flac esp-tremor esp-aaac \
esp-ogg-container esp-amr esp-share esp-amrwbenc esp-aac audio_signal_process

COMPONENT_ADD_LDFLAGS :=  -L$(COMPONENT_PATH)/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))
