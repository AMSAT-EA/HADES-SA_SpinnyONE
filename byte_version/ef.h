#ifndef GENERAL_EFEMERIDES_H
#define GENERAL_EFEMERIDES_H

#define PACKEF __attribute__((packed, scalar_storage_order("big-endian")))

#include "db.h"
#include "ef_prop.h"

/**
 * @struct _sat
 * @brief Estructura con información del satélite que contiene parámetros y valores del TLE que permiten propagar su posición.
 */
typedef struct {
	uint16_t adr;   ///< dirección de red del satélite (1..127)
	uint32_t ful;   ///< frecuencia de uplink 
	uint32_t fdl;   ///< frecuencia de downlink
	tle_t tle; ///< estructura /ref tle_t
} PACKEF _sat;

/**
 * @struct _frm
 * @brief Paquete de efemérides con bits necesarios para la identificación del tipo de paquete, la información del satélite y últimos valores de latitud, longitud y altitud calculados
 */
typedef struct {
	uint64_t train1; ///< secuencia de entrenamiento (0xAAAAAAAA)
	uint64_t train2; ///< secuencia de entrenamiento (0xAAAAAAAA)
	uint16_t sync;   ///< secuencia de sincronia     (0xBF35)
	uint8_t size;    // size de datos desde este
	uint8_t len;     ///< byte de longitud/type      (0xC8) -> definido en /ref EFTYPE, paquete_ef=12=0xC satelite=8
	uint32_t utc;    ///< hora utc
	_sat sat;   ///< estructura /ref _sat
	int16_t lat;    ///< latitud
	int16_t lon;    ///< longitd
	uint16_t alt;    ///< altitud
	uint8_t cnt;     ///< contador, solo hub lo incrementa
	uint16_t crc;    ///< checksum, comprueba integridad de los datos
}
PACKEF _frm;

void EF_reset(void);
int  EF_in(_frm * p);
int  EF_out(uint8_t *buf);
void EF_prop(void);
void EF_dump1(void);
void EF_dump2(void);
void EF_test(void);
int  EF_txenable(void);
int  EF_paenable(void);
int  EF_iotsleep(void);

#define NSAT 16 + 1   //direccion satelite=posicion en tabla, posicion 0 siempre en blanco
#endif
