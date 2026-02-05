/**
 * @file   db.h
 * @author Celia
 * @date   2024/04/22
 * @brief  Funciones y estructuras para la gestión de las bases de datos
 *
 */
#ifndef DB_H
#define DB_H
#include <stdio.h>

/**
 * @struct tle_t
 * @brief  Estructura de datos TLE (Two-Line Element) 
 * Todos los parámetros de la estructura han sido preprocesados anteriormente desde el formato TLE para tomar únicamente los datos que necesita el propagador SGP4
 */
typedef struct { uint32_t epoch; float xndt2o, xndd6o, bstar, xincl, xnodeo, eo, omegao, xmo, xno; } tle_t;

#define SIZE_FOV  20 ///< Tamaño de la tabla \ref fov
/**
 * @struct _node
 * @brief Estructura de nodos en FOV (Field Of View)
 */
struct _node{
  int  add;   ///< Dirección de red del nodo
  int  t;     ///< Tiempo/reloj de ultima vez escuchado/agregado
  unsigned char seq:1; ///< Número de secuencia, varía entre 0 y 1. Es un campo independiente por flujo
  int  Q;     ///< Calidad del enlace
};

#define SIZE_ZIC 9 ///< Tamaño de la tabla \ref zic
/**
 * @struct _zic
 * @brief Estructura de zonas de interés comercial
 */
struct _zic{
  int lat;   ///< Latitud del centro
  int lon;   ///< Longitud del centro
  int radio; ///< Radio del área
  unsigned char flag; ///< Etiqueta (0 = no transmisión; 1 = alta potencia)
};

#define SIZE_GS 4 ///< Tamaño de la tabla \ref gs
/**
 * @struct _gs
 * @brief Estructura de estaciones terrenas
 */
struct _gs{
  int add; ///< Address, direccion de red
  int lat; ///< Latitud
  int lon; ///< Longitud
};


//PUBLIC
extern struct _node  fov[SIZE_FOV]; 
extern struct _zic   zic[SIZE_ZIC]; 
extern struct _gs    gs [SIZE_GS]; 
extern int           first;
extern int           ptx;

#endif /*DB_H*/
