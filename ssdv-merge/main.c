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

#include "main.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>


int procesar(char * list_file_name, char * ssdv_output_file) ;
int process_file(char * source_file, char * output_file);

int main(int argc, char * argv[]) {

        printf("\n");
        printf("*************************************************************\n");
        printf("*                                                           *\n");
        printf("* SSDV frame merging for the HADES-SA (SpinnyONE) satellite *\n");
        printf("*               AMSAT EA - Free distribution                *\n");
        printf("*                       Version 1.05                        *\n");
        printf("*                  Compiled : %10s                   *\n",__DATE__);
        printf("*                                                           *\n");
        printf("*************************************************************\n");

        printf("\n");
        printf("This software is able to merge SSDV frames from:\n");
        printf("SpinnyONE HADES-SA (3)\n\n");
        printf("For available parameters use -help\n");
        
        printf("\n");

        if (argc != 3) {
               
		printf("Please execute %s  input_list_file ssv_output_file\n\n", argv[0]);
                printf("The input_list_file must contain the file list to process, each in one line. Check input_sample.txt\n\n");
                return -1;
        
        }

        return procesar(argv[1], argv[2]);
        

}

// input_list_file contiene el nombre del fichero con la lista de ficheros a procesar
// ssdv_output_file contiene el fichero ssdv a generar

int procesar(char * input_list_file,  char * ssdv_output_file) {

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char fecha[64];

        sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        printf("%s Opening file with file list : %s\n", fecha, input_list_file);

	FILE * f = fopen(input_list_file, "r");

	if (f == NULL) {

		printf("%s Error opening list file : %s\n", fecha, input_list_file);
		return -1;
	}

	char buffer[512];

	int estado = 0;

	do {

        	memset(buffer, 0, sizeof(buffer));

		estado = fscanf(f, "%s", buffer);

		if (strlen(buffer) < 1) break;

		printf("%s Read : %s\n", fecha, buffer);
		
		if (process_file(buffer, ssdv_output_file) != 0) {

               		printf("%s Error while processing file : %s\n", fecha, buffer);

			fclose(f);
                	return -1;

		}


	} while (estado == 1);

	fclose(f);

	printf("%s Finished OK. Generated file : %s\n\n", fecha, ssdv_output_file);

        return 0;

}


int process_file(char * source_file, char * output_file) {

       	time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char fecha[64];

	static int first_time = 1;

        sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	// abrir el fichero de frame

	FILE * frame_file = fopen(source_file, "rb");

	if (frame_file == NULL) {

              printf("%s Error opening list file : %s\n", fecha, source_file);
              return -1;

	}	

//        printf("%s Opened file : %s\n", fecha, source_file);

       	FILE * ssdv_file = NULL;

	if (first_time) {

		ssdv_file  = fopen(output_file, "wb+");
		first_time = 0;

	} else {

		ssdv_file = fopen(output_file, "ab");

	}

        if (ssdv_file == NULL) {

		printf("%s Error writing to ssdv file : %s\n", fecha, output_file);

                fclose(frame_file);

                return -1;

	}

	// leer frame completa

/*typedef struct ssdv_frame {

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
*/
	ssdv_file_frame frame;
	
	int read = fread(&frame, sizeof(frame), 1, frame_file);

	if (read != 1) {

        	printf("%s Error. Could not read %ld bytes from file : %s\n", fecha, sizeof(frame), source_file);

                fclose(frame_file);
                fclose(ssdv_file);
		return -1;
	}

	if (frame.training != 0x55) {

               	printf("%s Error. Training does not match (%d). It should be 0x55. Remove file from list : %s\n", fecha, frame.training, source_file);

                fclose(frame_file);
                fclose(ssdv_file);

                return -1;

	}

        if (frame.ssdv_type!= 0x66) {

                printf("%s Error. SSDV Type does not match (%d). It should be 0x66. Remove file from list : %s\n", fecha, frame.training, source_file);

                fclose(frame_file);
                fclose(ssdv_file);

                return -1;

        }

	int estado = fwrite(&frame, sizeof(ssdv_file_frame), 1, ssdv_file);
	
	if (estado != 1) {

         	printf("%s Error. Could not write %ld bytes to file : %s\n", fecha, sizeof(frame), output_file);

		fclose(frame_file);
		fclose(ssdv_file);

              	return -1;

	}

	fclose(frame_file);
	fclose(ssdv_file);

	return 0;

}


