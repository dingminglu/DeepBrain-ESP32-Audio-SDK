#ifndef _TS_ANALYSIS_H
#define _TS_ANALYSIS_H
#include <stdio.h>

#define TS_DEC_OK (0)
#define TS_DEC_DONE (1)
#define TS_DEC_FAIL (2)
#define TS_DEC_SYNCERROR (3)
#define TS_DEC_FUTURE_SUPPORT (4)

#define TS_PACKET_SIZE (188)

typedef struct {
    int  b_video; //whether output video stream   -v brazil-bq.264
    int  b_audio; //whether output audio stream  -a brazil-bq.aac
    int  b_nts;   //whether output new ts file   -o brazil-new.ts
} ts_param_t;

typedef struct {
    unsigned char *data_ts; ///the address for ts output data
    int num_ts; ///the number of ts data in data_ts
    unsigned char *data_audio; //the address for audio output data;
    int num_audio; ///the number of audio data in data_audio
    unsigned char *data_video; //the address for video output data;
    int num_video; ///the number of video data in data_video
} ts_dec_data_t;

#define  PROGRAM_STREAM_MAP       0xBC
#define  PRIVATE_STREAM1          0xBD
#define  PADDING_STREAM           0xBE
#define  PRIVATE_STREAM2          0xBF

#define  ECM                      0xF0
#define  EMM                      0xF1
#define  DSMCC_STREAM             0xF2

#define  H222_1_E                 0xF8

#define  PROGRAM_STREAM_DIRECTORY 0xFF

typedef struct {
    unsigned char *data;
    int count;
    int left;
} TS;

typedef struct {
    unsigned char  sync_byte;
    unsigned char  transport_error_indicator;
    unsigned char  payload_unit_start_indicator;

    unsigned char  transport_priority;
    unsigned int   PID;
    unsigned char  transport_scrambling_control;
    unsigned char  adaptation_field_control;
    unsigned char  continuity_counter;
} TsHeader;

typedef struct {
    unsigned char descriptor_tag;
    unsigned char descriptor_length;
    unsigned char service_type;
    unsigned char service_provider_name_length;
    unsigned char service_provider_name[258];
    unsigned char service_name_length;
    unsigned char service_name[258];
} Service_Descriptor;

typedef struct {
    unsigned char  table_id;
    unsigned char  section_syntax_indicator;
    unsigned char  reserved_future_use0;
    unsigned char  reserved0;
    unsigned int   section_length;
    unsigned int   transport_stream_id;
    unsigned char  reserved1;
    unsigned char  version_number;
    unsigned char  current_next_indicator;
    unsigned char  section_number;
    unsigned char  last_section_number;
    unsigned int   original_network_id;
    unsigned char  reserved_future_use1;
    unsigned char  N;
    unsigned int  service_id[12];
    unsigned char reserved_future_useN[12];
    unsigned char EIT_schedule_flag[12];
    unsigned char EIT_present_following_flag[12];
    unsigned char running_status[12];
    unsigned char free_CA_mode[12];
    unsigned char descriptors_loop_length[12];
    unsigned char descriptor_tag[12];

    Service_Descriptor service_descriptor[12];
    unsigned int  CRC_32;
} TSSDT;

typedef struct {
    unsigned char  table_id;
    unsigned char  section_syntax_indicator;
    unsigned char  zero0;
    unsigned char  reserved0;
    unsigned int   section_length;
    unsigned int   transport_stream_id;
    unsigned char  reserved1;
    unsigned char  version_number;
    unsigned char  current_next_indicator;
    unsigned char  section_number;
    unsigned char  last_section_number;
    unsigned char  N;
    unsigned int  program_number[12];
    unsigned char reserved[12];
    unsigned int  network_PID[12];
    unsigned int  program_map_PID[12];
    unsigned int  CRC_32;
} TSPAT;

typedef struct {
    unsigned char  table_id;
    unsigned char  section_syntax_indicator;
    unsigned char  zero0;
    unsigned char  reserved0;
    unsigned int   section_length;
    unsigned int  program_number;
    unsigned char  reserved1;
    unsigned char  version_number;
    unsigned char  current_next_indicator;
    unsigned char  section_number;
    unsigned char  last_section_number;
    unsigned char  reserved2;
    unsigned int   PCR_PID;
    unsigned char  reserved3;
    unsigned int   program_info_length;
    unsigned char  N;
    unsigned char  N1;
    unsigned char  stream_type[12];
    unsigned char  reservedN1_0[12];
    unsigned int   elementary_PID[12];
    unsigned char  reservedN1_1[12];
    unsigned int   ES_info_length[12];
    unsigned int  CRC_32;
} TSPMT;

typedef struct {
    unsigned char adaptation_field_length;
    unsigned char discontinuity_indicator;
    unsigned char random_access_indicator;
    unsigned char elementary_stream_priority_indicator;
    unsigned char PCR_flag;
    unsigned char OPCR_flag;
    unsigned char splicing_point_flag;
    unsigned char transport_private_data_flag;
    unsigned char adaptation_field_extension_flag;
    unsigned long long  program_clock_reference_base;
    unsigned char     reverse0;
    unsigned long long  program_clock_reference_extension;
    unsigned long long  PCR;
    unsigned int  original_program_clock_reference_base;
    unsigned char reverse1;
    unsigned int  riginal_program_clock_reference_extension;
    unsigned char splice_countdown;

} TSADT;

typedef struct {
    unsigned int   packet_start_code_prefix;
    unsigned char  stream_id;
    unsigned int   PES_packet_length;
    unsigned char  onezero;
    unsigned char  PES_scrambling_control;
    unsigned char  PES_priority;
    unsigned char  data_alignment_indicator;
    unsigned char  copyright;
    unsigned char  original_or_copy;
    unsigned char  PTS_DTS_flags;
    unsigned char  ESCR_flag;
    unsigned char  ES_rate_flag;
    unsigned char  DSM_trick_mode_flag;
    unsigned char  additional_copy_info_flag;
    unsigned char  PES_CRC_flag;
    unsigned char  PES_extension_flag;
    unsigned char  PES_header_data_length;
    unsigned char    zero0010;
    unsigned long long PTS3230;
    unsigned char    marker_bit0;
    unsigned long long PTS2915;
    unsigned char    marker_bit1;
    unsigned long long PTS1400;
    unsigned char    marker_bit2;
    unsigned char    zero0011;
    unsigned char    marker_bit3;
    unsigned char    marker_bit4;
    unsigned char    marker_bit5;
    unsigned char    zero0001;
    unsigned long long DTS3230;
    unsigned char    marker_bit6;
    unsigned long long DTS2915;
    unsigned char    marker_bit7;
    unsigned long long DTS1400;
    unsigned char    marker_bit8;
    unsigned long long PTS;
    unsigned long long DTS;
    unsigned char    marker_bit9;
    unsigned char    reserved;

} PES;

typedef struct {
    ts_param_t     *m_param;

    TS            m_ts;
    TsHeader      m_tsHeader;
    TSSDT         m_sdt;
    TSPAT         m_pat;
    TSPMT         m_pmt;
    TSADT         m_adt;
    PES           m_pes;

    TsHeader      m_tsNewHeader;
    TS            m_newTs;

    unsigned int  m_AudioPID;
    unsigned int  m_VideoPID;
    unsigned int  m_PATPID;
    unsigned int  m_PMTPID;

    unsigned char pointer_field;

    int           m_tsConter;

    int g_errors; ///count the error number
} ts_anlysis_t;

int ts_init(ts_anlysis_t *ts_anlysis, ts_param_t *param, unsigned char *data_ts, unsigned char *data_newts);

int ts_decoding(ts_anlysis_t *ts_anlysis, ts_dec_data_t *ts_dec_data);

#endif