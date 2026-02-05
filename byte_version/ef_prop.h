#ifndef GENERAL_EFEMERIDES_PROP_H
#define GENERAL_EFEMERIDES_PROP_H


/**
 * @struct vector_recef
 * @brief Vector de posición del satélite en coordenadas ECEF
 */
typedef struct { float xe,  ye,   ze;  } vector_recef;
/**
 * @struct vector_lla
 * @brief Vector de posición del satélite en coordenadas geodésicas (latitud, longitud y altitud)
 */
typedef struct { float lat, lon,  alt; } vector_lla;
/**
 * @struct vector_aer
 * @brief Vector de posición del satélite con el sistema de referencia centrado en la estación de tierra o terminal de usuario (azimut, elevación y rango)
 */
typedef struct { float azi, elev, rng; } vector_aer;


#endif
