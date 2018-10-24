#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_ADD_INCLUDEDIRS := . include

# COMPONENT_SRCDIRS :=  .

LIBS := vad esp_wakenet nn_model_nihaoxiaozhi recorder_eng

COMPONENT_ADD_LDFLAGS +=  -L$(COMPONENT_PATH)/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))
