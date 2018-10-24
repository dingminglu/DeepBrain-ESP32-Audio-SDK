#ifndef _FDK_MACRO_H_
#define _FDK_MACRO_H_

#include "EspAudioAlloc.h"
#include <stdlib.h>

#define FDKAAC_CODEC_CONFIG_INFO_SHOW

#define OVERLAP_MEMORY_SIZE_REDUCE
//#define RVLC_HCR_DISABLE
//#define ERROR_CONCEAL_DISABLE
//#define FORCE_DEC_MONO_CHANNEL

//#define FDKAAC_LIMITER_ENABLE

#define FKDAAC_MAX_CHANNEL (2)

#if !defined(RVLC_HCR_DISABLE) && !defined(ERROR_CONCEAL_DISABLE) && !defined(FORCE_DEC_MONO_CHANNEL)
#define FDKAAC_AAC_PLUS  ///FDKAAC_AAC_PLUS case, no function cut is allowed
#endif

//#define MEM_FDKAAC_ALL_IN_INRAM ///put all memory into inram. Because some ram exceed 10k, so calloc will autiomaticlly put the big ram into psram
#define MEM_FDKAAC_ALL_IN_PSRAM /////put all memory into PSRAM, slowest

#define FDK_AAC_SBR_MEM_SHARE

///FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM and FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM can only define one
///otherwise the speed is slow
///define FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM speed is faster than define FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM
///however, if define FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM, the memory move to psram is about double compare with define FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM

//#define FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM

//#define FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM
//#define FDKAAC_CONCEALSPECTRAL_MEMORY_IN_PSRAM

//#define FDKAAC_PS_DEC_MEMORY_IN_PSRAM

#endif