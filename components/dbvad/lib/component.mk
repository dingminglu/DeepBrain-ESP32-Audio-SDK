#
# Component Makefile

COMPONENT_ADD_INCLUDEDIRS := include

LIBS := dbvad

COMPONENT_ADD_LDFLAGS := $(COMPONENT_PATH)/lib/libdbvad.a \


COMPONENT_ADD_LINKER_DEPS := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))
