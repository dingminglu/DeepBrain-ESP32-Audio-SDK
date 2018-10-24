#
# Component Makefile

COMPONENT_ADD_INCLUDEDIRS := .

LIBS := speech_wakeup

COMPONENT_ADD_LDFLAGS := $(COMPONENT_PATH)/libspeech_wakeup.a \


COMPONENT_ADD_LINKER_DEPS := $(patsubst %,$(COMPONENT_PATH)/lib%.a,$(LIBS))
