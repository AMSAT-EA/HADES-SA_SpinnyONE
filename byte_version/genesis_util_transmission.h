/*
 *                                                       
 * Project     : GENESIS                                            
 * File        : genesis_util_transmission.h
 *
 * Description : This file includes all the function prototypes
 * Last update : 05 Febreuary 2026                                              
 *                                                                            
*/

#ifndef GENESIS_UTIL_TRANSMISSION
#define GENESIS_UTIL_TRANSMISSION

#include <stdio.h>

#include "genesis_system_status.h"

#define BYTE_SIZE                           	8

#define	PACKET_HEADER_SIZE			35 		// 32 training + 2 de sync + 1 de size

#define PACKET_FREE                         	0x00

#define TELEMETRY_PACKETTYPE_SIZE           	2
#define TELEMETRY_ADDRESS_SIZE              	4
#define MAX_INE 				10

#define PACKET_TRAINING_HEADER 			0xAAAAAAAAAAAAAAAA
#define SSDV_TRAINING_HEADER 			0x5555555555555555
#define SSDV_FIXED_SIZE				250


#define MAX_EPHEMERIS_PACKET_SIZE 512

extern char* 	tm_ina_buf_ptr;
extern uint16_t tm_ina_buf_len;
extern char* 	tm_bw_buf_ptr;
extern uint16_t tm_bw_buf_len;

#define NEBRIJA_PAYLOAD_DATA_SIZE		8
#define TIME_SERIES_DATA_SIZE			30

#define BYTES_FOR_280_BITS 			35
#define PN9_RAW_DATA_SIZE			248
#define SSDV_RAW_DATA_SIZE 			15872

#define CODEC2_HEADER_SIZE 			7

#define SSDV_FILE_HEADER_SIZE			15 // 0x55 SYNC, type (0x66 o 0x67) + 4 de callsign
									   // + 1 Image ID, 2 Packet ID, 1 Width, 1	Height, 1 Flags, 1 MCU Offset, 2 MCU Index
#define SSDV_DATA_SIZE		 		205 // los datos, no campos ssdv
#define SSDV_FRAME_SIZE 			251 // todo lo que venga después del campo size
#define SSDV_FEC_SIZE				32

// structures for the different packet types

// formato de paquete power

typedef struct st_power_packet {

   uint64_t training_1;    			// packet training header 		- 64 bits
   uint64_t training_2;    			// packet training header 		- 64 bits
   uint64_t training_3;    			// packet training header 		- 64 bits
   uint64_t training_4;    			// packet training header 		- 64 bits

   uint16_t sync;
   uint8_t  size;

   uint8_t  packettype_address;

   uint32_t sclock;
   uint8_t  spa;
   uint8_t  spb;
   uint8_t  spc;
   uint8_t  spd;
   uint16_t spi;
   uint16_t vbusadc_vbatadc_hi;    // 12 y 4
   uint16_t vbatadc_lo_vcpuadc_hi; // 8  y 8
   uint16_t vcpuadc_lo_vbus2;	 	// 4  y 12
   uint16_t vbus3_vbat2_hi;		// 12 y 4
   uint16_t vbat2_lo_ibat_hi;		// 8  y 8  -- iepsi2c son 16 bits
   uint16_t ibat_lo_icpu_hi;	 	// 8  y 8
   uint16_t icpu_lo_ipl;		 	// 4  y 12
   uint8_t  peaksignal;
   uint8_t  modasignal;
   uint8_t  lastcmdsignal;
   uint8_t  lastcmdnoise;

   uint16_t checksum;

} __attribute__((packed)) power_packet;


// formato de paquete temp

typedef struct st_temp_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;

    uint8_t  packettype_address;

    // se mandan sin signo porque van en incrementos de 0.5C con LUT

    uint32_t sclock;
    uint8_t  tpa;
    uint8_t  tpb;
    uint8_t  tpc;
    uint8_t  tpd;
    uint8_t  tpe;
    uint8_t  teps;
    uint8_t  ttx;
    uint8_t  ttx2;
    uint8_t  trx;
    uint8_t  tcpu;
    
    uint16_t checksum;

} __attribute__((packed)) temp_packet ;


// formato de paquete status

typedef struct st_status_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;

    uint8_t  size;
    uint8_t  packettype_address;

    uint32_t sclock;
    uint32_t uptime;
    uint16_t nrun;
    uint8_t  npayload;
    uint8_t  nwire;
    uint8_t  ntransponder;
    uint8_t  npayloaderrors_lastreset;
    uint8_t  bate_mote;
    uint8_t  subsystems_status;

    uint8_t  nTasksNotExecuted;
    uint8_t  antennaDeployed;
    uint8_t  nExtEepromErrors;

    uint8_t  failed_task_id;
    uint8_t  nIOT;

    uint8_t  strfwd0;
    uint16_t strfwd1;
    uint16_t strfwd2;
    uint8_t  strfwd3;

    uint8_t rx_percentage;
    uint8_t telemetry_percentage;
    uint8_t transponder_percentage;
    uint8_t ptt_hp_percentage;
    uint8_t ptt_lp_percentage;
    uint8_t ple_percentage;
    uint8_t bwe_percentage;
    uint8_t vbat_higher_than_vbus_percentage;
    uint8_t payload_frames;
    uint8_t payload_params;
    uint8_t payload_current_img_id;

    uint16_t checksum;


} __attribute__((packed)) status_packet;


// formato de paquete para estadísticas power

typedef struct st_power_ranges_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;

    uint8_t  size;
    uint8_t  packettype_address;

    uint32_t sclock;
    uint16_t minvbusadc_vbatadc_hi;
    uint16_t minvbatadc_lo_vcpuadc_hi;
    uint8_t  minvcpuadc_lo_free;
    uint8_t  minvbus2;
    uint8_t  minvbus3;
    uint8_t  minvbat2;
    uint8_t  minibat;
    uint8_t  minicpu;
    uint8_t  minipl;
    uint16_t maxvbusadc_vbatadc_hi;
    uint16_t maxvbatadc_lo_vcpuadc_hi;
    uint8_t  maxvcpuadc_lo_free;
    uint8_t  maxvbus2;
    uint8_t  maxvbus3;
    uint8_t  maxvbat2;
    uint8_t  maxibat;
    uint8_t  maxicpu;
    uint8_t  maxipl;
    uint8_t  ibat_rx_charging;
    uint8_t  ibat_rx_discharging;
    uint8_t  ibat_tx_low_power_charging;
    uint8_t  ibat_tx_low_power_discharging;
    uint8_t  ibat_tx_high_power_charging;
    uint8_t  ibat_tx_high_power_discharging;

    uint16_t checksum;

} __attribute__((packed)) power_ranges_packet;


// formato de paquete para estadísticas power

typedef struct st_temp_ranges_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;

    uint8_t  packettype_address;

    // es correcto como uint8, se envían con LUT en incrementos de 0.5C

    uint32_t sclock;
    uint8_t  mintpa;
    uint8_t  mintpb;
    uint8_t  mintpc;
    uint8_t  mintpd;
    uint8_t  mintpe;
    uint8_t  minteps;
    uint8_t  minttx;
    uint8_t  minttx2;
    uint8_t  mintrx;
    uint8_t  mintcpu;
    uint8_t  maxtpa;
    uint8_t  maxtpb;
    uint8_t  maxtpc;
    uint8_t  maxtpd;
    uint8_t  maxtpe;
    uint8_t  maxteps;
    uint8_t  maxttx;
    uint8_t  maxttx2;
    uint8_t  maxtrx;
    uint8_t  maxtcpu;

    uint16_t checksum;

} __attribute__((packed)) temp_ranges_packet;


typedef struct st_deploy_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;

    uint8_t  packettype_address;

    uint8_t * raw_data_pointer;
    uint8_t  raw_data_size;

    uint16_t checksum;

} __attribute__((packed)) deploy_packet;


typedef struct st_extendedpower_ine_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;

    uint8_t  packettype_address;

    uint8_t *   raw_data_pointer;
    uint8_t  raw_data_size;

    uint16_t checksum;

} __attribute__((packed)) extendedpower_ine_packet;


typedef struct st_time_series_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;

    uint8_t  packettype_address;

    uint32_t clock;
    uint8_t  variable;

    uint8_t data[TIME_SERIES_DATA_SIZE];

    uint16_t checksum;

} __attribute__((packed)) time_series_packet;


typedef struct codec2_frame {

    uint16_t sync;
    uint8_t  size;
    uint8_t  packettype_address;
    uint8_t  frame_number;						// 2 bytes
    uint8_t  codec2_bits[BYTES_FOR_280_BITS];						// 2 bytes

} __attribute__((packed)) codec2_frame_st;


typedef struct pn9_frame {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;
    uint8_t  packettype_address;

    uint8_t raw_data[PN9_RAW_DATA_SIZE];

} __attribute__((packed)) pn9_frame_st;


/*
Sync1	0x55	0x55 - may be preceded by one or more sync bytes             -
	8	SSDV Type	0x66	"0x66 - normal mode (224 byte packet + 32 byte fec)
0x67 - no-fec mode (256 byte packet)     -"
	16	Sync2 part 1 (sync)	0xBF35	0xBF35
	8	Sync2 part 2 (size)	size	Fixed, 250
	4	type	-	Tipo de paquete: 10
	4	address	-	direccion origen: 3 para HADES-SA, 9 para GENESIS-M

*/

typedef struct ssdv_frame {

    uint8_t  training;
    uint8_t  ssdv_type;
    uint16_t sync_part1;
    uint8_t  size;
    uint8_t  packettype_address;
    uint8_t  imageID;
    uint16_t packetID;
    uint8_t  width;
    uint8_t  height;
    uint8_t  flags;
    uint8_t  mcuOffset;
    uint16_t mcuIndex;
    uint8_t  data[SSDV_DATA_SIZE];
    uint32_t checksum;
    uint8_t  FEC[SSDV_FEC_SIZE];

} __attribute__((packed)) ssdv_file_frame;


typedef struct bbs_packet {

    uint64_t training_1;    			// packet training header 		- 64 bits
    uint64_t training_2;    			// packet training header 		- 64 bits
    uint64_t training_3;    			// packet training header 		- 64 bits
    uint64_t training_4;    			// packet training header 		- 64 bits

    uint16_t sync;
    uint8_t  size;

    uint8_t packettype_address;
    uint8_t callsigns[5*6];
    uint8_t messages[5*7];
    uint8_t num_codec2_frames[5];

    uint16_t checksum;

} __attribute__((packed)) bbs_packet_st;


uint8_t celsius_to_tx(float celsius);

#endif
