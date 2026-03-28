/*************************************************************************************/
/*                                                                                   */
/* License information / Informacion de licencia                                     */
/*                                                                                   */
/* Todos los contenidos de la web de AMSAT EA se distribuyen                         */
/* bajo licencia Creative Commons CC BY 4.0 Internacional                            */
/* (distribución, modificacion u y uso libre mientras se cite AMSAT EA como fuente). */
/*                                                                                   */
/* All the contents of the AMSAT EA website are distributed                          */
/*  under a Creative Commons CC BY 4.0 International license                         */
/* (free distribution, modification and use crediting AMSAT EA as the source).       */
/*                                                                                   */
/* AMSAT EA 2023                                                                     */
/* https://www.amsat-ea.org/proyectos/                                               */
/*                                                                                   */
/*************************************************************************************/

#include <stdint.h>


#define SSDV_TRAINING_HEADER                                    0x5555555555555555
#define SSDV_FIXED_SIZE                                                 250
#define SSDV_RAW_DATA_SIZE                              15872
#define SSDV_FILE_HEADER_SIZE           15 // 0x55 SYNC, type (0x66 o 0x67) + 4 de callsign
#define SSDV_DATA_SIZE                          205 // los datos, no campos ssdv
#define SSDV_FRAME_SIZE                         251 // todo lo que venga después del campo size
#define SSDV_FEC_SIZE                           32


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

