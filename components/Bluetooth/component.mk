#
# Component Makefile
#
ifdef CONFIG_BT_ENABLED
COMPONENT_ADD_INCLUDEDIRS :=	bluedroid/bta/include			\
				bluedroid/bta/sys/include		\
				bluedroid/device/include		\
				bluedroid/embdrv/sbc/decoder/include	\
                bluedroid/btif/include          \
				bluedroid/osi/include			\
				bluedroid/stack/avct/include		\
				bluedroid/stack/avrc/include		\
				bluedroid/stack/avdt/include		\
				bluedroid/stack/a2dp/include		\
				bluedroid/stack/include			\
				bluedroid/utils/include			\
				bluedroid/api/include			\
				include


COMPONENT_SRCDIRS := 	bluedroid/bta/av			\
			bluedroid/bta/ar			\
			bluedroid/bta/sys			\
			bluedroid/btif				\
			bluedroid/osi				\
			bluedroid/embdrv/sbc/decoder/srce			\
			bluedroid/stack/avct			\
			bluedroid/stack/avrc			\
			bluedroid/stack/avdt			\
			bluedroid/stack/a2dp			\
			bluedroid/utils				\
			bluedroid/api			\
            bluedroid/btc/profile/std/gap       \
			.

CFLAGS += -DBTA_AR_INCLUDED=TRUE -DBTA_AV_INCLUDED=TRUE -DBTA_AV_SINK_INCLUDED=TRUE -DAVRC_METADATA_INCLUDED=TRUE -DLOG_LEVEL=5
else
COMPONENT_SRCDIRS := null
endif
