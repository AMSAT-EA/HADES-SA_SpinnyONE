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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define CODEC2_FRAME_SIZE 40 

int procesar(char * list_file_name, char * codec2_output_file) ;
int process_file(char * source_file, char * output_file);

int main(int argc, char * argv[]) {

        printf("\n");
        printf("***************************************************************\n");
        printf("*                                                             *\n");
        printf("* CODEC2 frame merging for the HADES-SA (SpinnyONE) satellite *\n");
        printf("*                 AMSAT EA - Free distribution                *\n");
        printf("*                         Version 1.05                        *\n");
        printf("*                    Compiled : %10s                   *\n",__DATE__);
        printf("*                                                             *\n");
        printf("***************************************************************\n");

        printf("\n");
        printf("This software is able to merge CODEC2 frames from:\n");
        printf("SpinnyONE HADES-SA (3)\n\n");
        printf("For available parameters use -help\n");
        
        printf("\n");

        if (argc != 3) {
               
		printf("Please execute %s input_list_file codec2_output_file\n\n", argv[0]);
                printf("The input_list_file must contain the file list to process, each in one line. Check input_sample.txt\n\n");
                return -1;
        
        }

        return procesar(argv[1], argv[2]);
        

}

// input_list_file contiene el nombre del fichero con la lista de ficheros a procesar
// codec2_output_file contiene el fichero codec2 a generar

int procesar(char * input_list_file,  char * codec2_output_file) {

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
		
		if (process_file(buffer, codec2_output_file) != 0) {

               		printf("%s Error while processing file : %s\n", fecha, buffer);

			fclose(f);
                	return -1;

		}


	} while (estado == 1);

	fclose(f);

	printf("%s Finished OK. Generated file : %s\n\n", fecha, codec2_output_file);

        return 0;

}


int process_file(char * source_file, char * output_file) {

       	time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char fecha[64];

	static int first_time = 1;
	static int last_frame = -1;

	char temp[25];
	memset(temp, 0, sizeof(temp));

	strncpy(temp, source_file + strlen(source_file) - 7, 3); // xxx.bin

	if (strlen(temp) > 3) {

              printf("%s No valid frame number in file name : %s\n", fecha, source_file);
              return -1;

	}

	int frame_num = atoi(temp);

	if (frame_num <= last_frame) {

              printf("%s Invalid frame number in file name : %s\n", fecha, source_file);
              return -1;

	}

	int frame_diff = frame_num - last_frame;

	last_frame = frame_num;

        sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	// abrir el fichero de frame

	FILE * frame_file = fopen(source_file, "rb");

	if (frame_file == NULL) {

              printf("%s Error opening file : %s\n", fecha, source_file);
              return -1;

	}	

//        printf("%s Opened file : %s\n", fecha, source_file);

       	FILE * codec2_file = NULL;

	if (first_time) {

		codec2_file  = fopen(output_file, "wb+");
		first_time = 0;

		// grabar cabecera codec2

		uint8_t c2_header[7] = { 0xC0, 0xDE, 0xC2, 0x01, 0x02, 0x08, 0x00 };
		int escritos = fwrite(c2_header, sizeof(c2_header), 1, codec2_file);

		if (escritos != 1) {

                	printf("%s Error writing header to codec2 file : %s\n", fecha, output_file);

                	fclose(frame_file);

                	return -1;

		}


	} else {

		codec2_file = fopen(output_file, "ab");

	}

        if (codec2_file == NULL) {

		printf("%s Error writing to codec2 file : %s\n", fecha, output_file);

                fclose(frame_file);

                return -1;

	}

	// leer frame completa

	uint8_t frame[40];
	memset(frame, 0, sizeof(frame));
	
	int read = fread(&frame, sizeof(frame), 1, frame_file);

	if (read != 1) {

        	printf("%s Error. Could not read %ld bytes from file : %s\n", fecha, sizeof(frame), source_file);

                fclose(frame_file);
                fclose(codec2_file);
		return -1;
	}


	// si solo hay uno de diferencia esta ok, es lo normal

	if (frame_diff > 1) {

		uint8_t blank_frame[40];
		memset(blank_frame, 0, sizeof(blank_frame));

		for (int i = 0; i < frame_diff-1; i++) {

        		// insertar tantos bloques de 40 bytes como frames falten

			int e = fwrite(&blank_frame, sizeof(blank_frame), 1, codec2_file);	

        		if (e != 1) {

                		printf("%s Error. Could not write %ld bytes to file : %s\n", fecha, sizeof(frame), output_file);

                		fclose(frame_file);
                		fclose(codec2_file);

                		return -1;

        		}

			printf("%s Wrote missing frame to zero\n", fecha);

		}

	}

	int estado = fwrite(&frame, sizeof(frame), 1, codec2_file);
	
	if (estado != 1) {

         	printf("%s Error. Could not write %ld bytes to file : %s\n", fecha, sizeof(frame), output_file);

		fclose(frame_file);
		fclose(codec2_file);

              	return -1;

	}

	fclose(frame_file);
	fclose(codec2_file);

	return 0;

}


