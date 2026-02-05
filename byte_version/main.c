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
/* AMSAT EA 2026                                                                     */
/* https://www.amsat-ea.org/proyectos/                                               */
/*                                                                                   */
/*************************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h> 
#include <math.h>
#include <stdio.h>

#define nzones 34
#define pi 3.1415926535898

#include "genesis_crc.h"
#include "genesis_scrambler.h"
#include "genesis_util_transmission.h"
#include "sun.h"
#include "ef.h"
#include "ina.h"

#define SAT_ID_HADES_SA	 '3'

#define RX_BUFFER_SIZE 512
#define MAX_DATA_SIZE  300

#define TEMP_BUFFER_SIZE 1024
#define PN9_RAW_DATA_SIZE 248

const char name[MAXINA][5]={"SPA ","SPB ","SPC ","SPD ","SUN ","BAT ","BATP","BATN","CPU ","PL  ","SIM "};

FILE * f;
FILE * f_dat;

void procesar(char *);
char * source_desc(uint8_t sat_id);
char* overflying(double lat, double lon);
int16_t hex2int(char c);

time_t start_t; // para ficheros .dat
time_t parameter_start_t;
time_t diff_time = 0; // diferencia entre la hora local y la hora como parametro

int main(int argc, char * argv[]) {

	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);
	char fecha[64];

        printf("\n");
        printf("****************************************************\n");
        printf("*                                                  *\n");
        printf("* HADES-SA (SpinnyONE) Satellite Telemetry Decoder *\n");
        printf("*           AMSAT EA - Free distribution           *\n");
        printf("*               Version 1.00 (Bytes)               *\n");
        printf("*            Compilation : %10s             *\n",__DATE__);
        printf("*                                                  *\n");
        printf("****************************************************\n");

        printf("\n");
        printf("This software is able to decodify already unscrambled and CRC checked byte telemetry sequences from:\n");
        printf("HADES-SA (SpinnyONE) satellite (ID 3)\n\n");

        printf("Intended for its use with Andy UZ7HO's Soundmodem Windows demodulator software\n");
        printf("More information in http://www.amsat-ea.org\n");
        printf("\n");

        if (argc != 2) {

                printf("Please specify the file to decodify. File must contain bytes in text separated by spaces\n");
                printf("Sample: 13 D3 26 00 00 00 00 00 00 00 00 E0 B2 6D 0B 00 30 30 3E 00 00 01 00 00 20 00 00 00 00 0C 40\n");
                printf("\n");

                return 1;

        }


        sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	printf("%s Running...\n", fecha);

	procesar(argv[1]);

	return 0;

}


// esta funcion comprueba que el sat_id sea correcto mostrando el mensaje
// correspondiente o error en caso contrario

int set_satellite_id(char satellite_id, uint8_t * sat_id_as_int) {

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char fecha[64];

        sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	if (satellite_id != SAT_ID_HADES_SA) {

        	printf("%s Error: Satellite id must be %c\n", fecha, SAT_ID_HADES_SA);
                return 1;
        }

        printf("%s Satellite id specified [%c]\n", fecha, satellite_id);

        *sat_id_as_int=satellite_id -'0';

	return 0;

}


// devuelve el tamaño, incluyendo checksum para el tipo de paquete (bytes utiles de Excel) (todo menos training y sync)
// 0 indica que se debe usar el campo size

uint16_t telemetry_packet_size(uint8_t satellite_id, uint8_t tipo_paquete) {

	uint16_t bytes_utiles[] = { 2, 31, 17, 41, 35, 27, 135, 45, 31, 123, 251, 37, 64, 249, 38, 73 };

	/*

	0 - not used
	1 - power
	2 - temps
	3 - status
	4 - power ranges
	5 - temp ranges
        7 - tera payload
        8 - antenna deploy
        9 - ine
       10 - SSDV
       11 - CODEC2 voice
       12 - efemerides
       13 - PN9
       14 - time series
       15 - bbs
	
	*/

	if (tipo_paquete > sizeof(bytes_utiles)-1) return 1;

	return bytes_utiles[tipo_paquete]; // utiles

}


void visualiza_powerpacket(uint8_t sat_id, power_packet * packet) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  
    fprintf(f,"*** Power packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    fprintf(f,"sat_id          : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"sclock          : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);
    fprintf(f,"spi             : %4d mW (total instant power)\n", packet->spi << 1);
    fprintf(f,"spa             : %4d mW (last 3 mins peak)\n", packet->spa << 1);
    fprintf(f,"spb             : %4d mW (last 3 mins peak)\n", packet->spb << 1);
    fprintf(f,"spc             : %4d mW (last 3 mins peak)\n", packet->spc << 1);
    fprintf(f,"spd             : %4d mW (last 3 mins peak)\n\n", packet->spd << 1);

    // 32 bits o desborda con las multiplicacion
    
    uint32_t vbus1 = (packet->vbusadc_vbatadc_hi >> 4);
             vbus1 = vbus1 * 1400;
	     vbus1 = vbus1 / 1000;

    uint16_t vbus2 = (packet->vcpuadc_lo_vbus2 & 0x0fff);
             vbus2 = vbus2 * 4;

    uint16_t vbus3 = (packet->vbus3_vbat2_hi >> 4);
    	     vbus3 = vbus3 * 4;

    fprintf(f,"vbus1		: %4d mV bus voltage read in CPU.ADC\n"  , vbus1);  
    fprintf(f,"vbus2       	: %4d mV bus voltage read in EPS.I2C\n"  , vbus2);
    fprintf(f,"vbus3       	: %4d mV bus voltage read in CPU.I2C\n\n", vbus3); 

    uint32_t vbat1 = (packet->vbusadc_vbatadc_hi << 8) & 0x0f00;
	     vbat1 = vbat1 | ((packet->vbatadc_lo_vcpuadc_hi >> 8) & 0x00ff);
    	     vbat1 = vbat1 * 1400;
	     vbat1 = vbat1 / 1000;

    fprintf(f,"vbat1		: %4d mV bat voltage read in EPS.ADC\n"  ,vbat1); 

    uint16_t vbat2 = (packet->vbus3_vbat2_hi << 8) & 0x0f00;
             vbat2 = (vbat2 | (packet->vbat2_lo_ibat_hi >> 8));
             vbat2 = vbat2 *4;  

    fprintf(f,"vbat2		: %4d mV bat voltage read in EPS.I2C\n\n", vbat2);

    uint16_t ibat = packet->vbat2_lo_ibat_hi  << 8;
             ibat = ibat | (packet->ibat_lo_icpu_hi >> 8);

    if (ibat & 0x0800) ibat = ibat |0x0f000;

    int16_t ibats = (int16_t) ibat;

    float ii;
    fprintf(f,"vbus1-vbat1	: %4d mV \n", vbus1-vbat1);
    ii =vbus1;
    ii-=vbat1; 
    ii/=0.310;
    fprintf(f,"vbus3-vbus2	: %4d mV \n\n",vbus3-vbus2);
    ii =vbus3;
    ii-=vbus2; 
    ii/=0.280;
    
    uint32_t vcpu_temp = (packet->vbatadc_lo_vcpuadc_hi << 4) & 0x0ff0;
             vcpu_temp = vcpu_temp | (packet->vcpuadc_lo_vbus2 >> 12);

    uint32_t vcpu = 1210*4096/vcpu_temp;

    uint16_t icpu = (packet->ibat_lo_icpu_hi << 4) & 0x0ff0;
             icpu = icpu | (packet->icpu_lo_ipl >> 12);

    // la cpu sale siempre negativa, chip al reves, multiplicamos por -1
    int16_t icpus = (int16_t) icpu;

    // si bit 12 esta a 1 ponemos el resto tambien para signo ok
    if (icpus & 0x800) icpus = (icpus | 0xf000)* -1; 

    uint16_t ipl = (packet->icpu_lo_ipl & 0x0fff);

    int16_t ipls = (int16_t) ipl;
    
    if (ipls & 0x0800) ipls = (ipls | 0xf000);

    fprintf(f,"vcpu		: %4d mV\n\n", vcpu); 
    fprintf(f,"icpu		: %4d mA @DCDCinput\n",  icpus);

    int32_t iii;
    iii =(icpus*vbus3);
    iii/=vcpu;
    fprintf(f,"icpu		: %4d mA @DCDCoutput (estimation)\n", iii);

    fprintf(f,"ipl		: %4d mA (Last payload current)\n",   ipls);
    if (ibats == 0) fprintf(f,"ibat		: %4d mA\n\n", ibats);
    else
    if (ibats > 0) fprintf(f,"ibat		: %4d mA (Current flowing out from the battery)\n\n", ibats); // 8 y 8
		else fprintf(f,"ibat 		: %4d mA (Current flowing into the battery)\n\n", ibats);

    fprintf(f,"peaksignal	: %4d dB\n",  packet->peaksignal);
    fprintf(f,"modasignal	: %4d dB\n",  packet->modasignal);
    fprintf(f,"lastcmdsignal	: %4d dB\n",  packet->lastcmdsignal);
    fprintf(f,"lastcmdnoise	: %4d dB\n",  packet->lastcmdnoise);

    fprintf(f_dat, "%ld %ld %d %d %d %d %d %d ", t, start_t, packet->sclock, packet->spa << 1, packet->spb << 1, packet->spc << 1, packet->spd << 1, packet->spi << 1);
    fprintf(f_dat, "%d %d %d %d %d %d %d %d %d ", vbus1, vbat1, vcpu, vbus2, vbus3, vbat2, ibats, icpus, ipls);
    fprintf(f_dat, "%d %d %d %d\n", packet->peaksignal, packet->modasignal, packet->lastcmdsignal, packet->lastcmdnoise);

}


void visualiza_temppacket(uint8_t sat_id, temp_packet * packet) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Temp packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    fprintf(f,"sat_id : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"sclock : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);

    if (packet->tpa   == 255) fprintf(f,"tpa    : \n"); else fprintf(f,"tpa    : %+5.1f degC temperature in SPA.I2C\n", ((float)packet->tpa/2)-40.0);
    if (packet->tpb   == 255) fprintf(f,"tpb    : \n"); else fprintf(f,"tpb    : %+5.1f degC temperature in SPB.I2C\n", ((float)packet->tpb/2)-40.0);
    if (packet->tpc   == 255) fprintf(f,"tpc    : \n"); else fprintf(f,"tpc    : %+5.1f degC temperature in SPC.I2C\n", ((float)packet->tpc/2)-40.0);
    if (packet->tpd   == 255) fprintf(f,"tpd    : \n"); else fprintf(f,"tpd    : %+5.1f degC temperature in SPD.I2C\n", ((float)packet->tpd/2)-40.0);
    if (packet->teps  == 255) fprintf(f,"teps   : \n"); else fprintf(f,"teps   : %+5.1f degC temperature in EPS.I2C\n", ((float)packet->teps/2)-40.0);
    if (packet->ttx   == 255) fprintf(f,"ttx    : \n"); else fprintf(f,"ttx    : %+5.1f degC temperature in  TX.I2C\n", ((float)packet->ttx/2)-40.0);
    if (packet->ttx2  == 255) fprintf(f,"ttx2   : \n"); else fprintf(f,"ttx2   : %+5.1f degC temperature in  TX.NTC\n", ((float)packet->ttx2/2)-40.0);
    if (packet->trx   == 255) fprintf(f,"trx    : \n"); else fprintf(f,"trx    : %+5.1f degC temperature in  RX.NTC\n", ((float)packet->trx/2)-40.0);
    if (packet->tcpu  == 255) fprintf(f,"tcpu   : \n"); else fprintf(f,"tcpu   : %+5.1f degC temperature in CPU.ADC\n", ((float)packet->tcpu/2)-40.0);

    //fprintf(f,"checksum    : %04X\n", packet->checksum);

    fprintf(f_dat, "%ld %ld %+5.1f %+5.1f %+5.1f %+5.1f %+5.1f %+5.1f %+5.1f %+5.1f %+5.1f %+5.1f\n", t, start_t, ((float)packet->tpa/2)-40.0, ((float)packet->tpb/2)-40.0, ((float)packet->tpc/2)-40.0,
		((float)packet->tpd/2)-40.0, ((float)packet->tpe/2)-40.0, ((float)packet->teps/2)-40.0, ((float)packet->ttx/2)-40.0, ((float)packet->ttx2/2)-40.0, ((float)packet->trx/2)-40.0, ((float)packet->tcpu/2)-40.0);


}


typedef enum reset_cause_e
    {
        RESET_CAUSE_UNKNOWN = 0,
        RESET_CAUSE_LOW_POWER_RESET,
        RESET_CAUSE_WINDOW_WATCHDOG_RESET,
        RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET,
        RESET_CAUSE_SOFTWARE_RESET,
        RESET_CAUSE_POWER_ON_POWER_DOWN_RESET,
        RESET_CAUSE_EXTERNAL_RESET_PIN_RESET,
        RESET_CAUSE_BROWNOUT_RESET,
		
} reset_cause_t;


const char * reset_cause_get_name(reset_cause_t reset_cause) {

        const char * reset_cause_name = "TBD";

        switch (reset_cause)
        {
            case RESET_CAUSE_UNKNOWN:
                reset_cause_name = "Unknown";
                break;
            case RESET_CAUSE_LOW_POWER_RESET:
                reset_cause_name = "Low Power Reset";
                break;
            case RESET_CAUSE_WINDOW_WATCHDOG_RESET:
                reset_cause_name = "Window Watchdog reset";
                break;
            case RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET:
                reset_cause_name = "Independent watchdog reset";
                break;
            case RESET_CAUSE_SOFTWARE_RESET:
                reset_cause_name = "Software reset";
                break;
            case RESET_CAUSE_POWER_ON_POWER_DOWN_RESET:
                reset_cause_name = "Power-on reset (POR) / Power-down reset (PDR)";
                break;
            case RESET_CAUSE_EXTERNAL_RESET_PIN_RESET:
                reset_cause_name = "External reset pin reset";
                break;
            case RESET_CAUSE_BROWNOUT_RESET:
                reset_cause_name = "Brownout reset (BOR)";
                break;
        }

        return reset_cause_name;
}


const char * battery_status(uint8_t status) {


   switch(status) {

	case 0:
		return "Fully charged (3925 mV or higher) - All transmissions high power";
	case 1:
		return "High charge (Between 3800 mV and 3925 mV)";
	case 2:
		return "Charged (Between 3550 mV and 3800 mV) - Transmissions in low power";
        case 3:
                return "Low charge (Between 3300 mV and 3550 mV) - Limited transmissions and in low power";
	case 4:
		return "Very low charge (Between 3200 mV and 3300 mV) - Limited transmissions and in low power";
	case 5:
                return "Extremely low charge (Between 2500 mV and 3200 mV) - Limited transmissions and in  low power";
	case 6:
                return "Battery damaged (Below 2500 mV) - Battery disconnected";

	default:
		return "Unknown";

   }
	

}


const char * transponder_mode(uint8_t mode) {

        switch(mode) {


                case 0:
                        return "         Disabled";
                case 1:
                        return "         Enabled in FM mode";
                case 2:
                        return "         Enabled in FSK regenerative mode";
                default:
                        return "         Unknown";

        }

}


void visualiza_statuspacket(uint8_t sat_id, status_packet * packet) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Status packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;
    uint16_t daysup = 0, hoursup = 0, minutesup = 0, secondsup = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    uint32_t segundos_up  = packet->uptime;

    daysup    = (segundos_up / 86400);
    hoursup   = (segundos_up % 86400)/3600;
    minutesup = (segundos_up % 3600)/60;
    secondsup = (segundos_up % 60);

    fprintf(f,"sat_id              : %10d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"sclock              : %10d seconds satellite has been active (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);
    fprintf(f,"uptime              : %10d seconds since the last CPU reset  (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->uptime, daysup, hoursup, minutesup, secondsup);
    fprintf(f,"nrun                : %10d times satellite CPU was started\n",packet->nrun);
    fprintf(f,"npayload            : %10d times payload was activated\n",packet->npayload);
    fprintf(f,"nwire               : %10d times antenna deployment was tried\n", packet->nwire);
    fprintf(f,"ntransponder        : %10d times transponder was activated\n", packet->ntransponder); 

    uint8_t payloadFails = packet->npayloaderrors_lastreset >> 4;

    if (payloadFails == 0)
    fprintf(f,"nPayloadsFails 	    :  	       OK\n"); else
      fprintf(f,"nPayloadFails       : %10d\n", packet->npayloaderrors_lastreset >> 4);
    fprintf(f,"last_reset_cause    : %10d %s\n",packet->npayloaderrors_lastreset & 0x0F, reset_cause_get_name(packet->npayloaderrors_lastreset & 0x0F));
    fprintf(f,"bate (battery)      : %10X %s\n",packet->bate_mote >> 4, battery_status(packet->bate_mote >> 4));

    int e0 = (packet->subsystems_status & 1);  // enabled / disabled
    int e1 = (packet->subsystems_status >> 1) & 1; // working, not working

    if (e0 == 1 && e1 == 1) fprintf(f,"power amplifier     :          OK\n");
	else if (e0 == 1 && e1 == 0) fprintf(f,"power amplifier     :          FAIL\n");
		else if (e0 == 0) fprintf(f,"power amplifier     :          Disabled\n");

    if (((packet->subsystems_status >> 2) & 1) == 1) fprintf(f,"high speed uplink   :          OK\n");
	else fprintf(f,"high speed uplink   :          Disabled\n");

    if (((packet->subsystems_status >> 3) & 1) == 1) fprintf(f,"tx temp compensat   :          OK\n");
	else fprintf(f,"tx temp compensat   :          Disabled\n");	

    if (((packet->subsystems_status >> 4) & 1) == 1) fprintf(f,"RX board            :          OK\n");
        else fprintf(f,"RX board            :          Disabled\n");

    if (((packet->subsystems_status >> 5) & 1) == 1) fprintf(f,"Variable messaging  :          OK\n");
                                                else fprintf(f,"Variable messaging  :          Disabled\n");

    fprintf(f,"mote (transponder)  : %s\n", transponder_mode(packet->bate_mote & 0x0f));

    if (packet->nTasksNotExecuted == 0)
    fprintf(f,"nTasksNotExecuted   :          OK\n");
                else
    fprintf(f,"nTasksNotExecuted   : %10d Some tasks missed their execution time\n",packet->nTasksNotExecuted);

    if (packet->nExtEepromErrors == 0)
    fprintf(f,"nExtEepromErrors    :          OK\n");
                else
    fprintf(f,"nExtEepromErrors    : %10d EEPROM Fail\n",packet->nExtEepromErrors);

    uint8_t ANTENNA_NOT_DEPLOYED = 0;
    uint8_t ANTENNA_DEPLOYED     = 1;

    if (sat_id == 2 || sat_id == 13 || sat_id == 3) {

	ANTENNA_NOT_DEPLOYED = 1; // en HADES-ICM va al reves
	ANTENNA_DEPLOYED     = 0;

    }

    if (packet->antennaDeployed == ANTENNA_NOT_DEPLOYED)
    fprintf(f,"antennaDeployed     :          KO (Antenna not yet deployed)\n");
		else
    	if (packet->antennaDeployed == 2)
    		fprintf(f,"antennaDeployed     :          UNKNOWN\n");
    	else if (packet->antennaDeployed == ANTENNA_DEPLOYED)
    fprintf(f,"antennaDeployed     :          OK (Antenna has been deployed)\n");

    if (packet->nTasksNotExecuted == 0) fprintf(f,"last_failed_task_id :          OK\n");
    	else fprintf(f,"last_failed_task_id :          Q%dT%d\n", packet->failed_task_id >> 6, packet->failed_task_id & 0b00111111);

    fprintf(f,"strfwd0 (id)        : %10X (%d)\n",packet->strfwd0, packet->strfwd0);
    fprintf(f,"strfwd1 (key)       : %10X (%d)\n",packet->strfwd1, packet->strfwd1);
    fprintf(f,"strfwd2 (value)     : %10X (%d)\n",packet->strfwd2, packet->strfwd2);
    fprintf(f,"strfwd3 (num_tcmds) : %10X (%d)\n",packet->strfwd3, packet->strfwd3);


    fprintf(f,"rx_percent          : %10d%%\n", packet->rx_percentage);
    fprintf(f,"telemetry_percent   : %10d%%\n", packet->telemetry_percentage);
    fprintf(f,"transponder_percent : %10d%%\n", packet->transponder_percentage);
    fprintf(f,"ptt_hp_percent      : %10d%%\n", packet->ptt_hp_percentage);
    fprintf(f,"ptt_lp_percent      : %10d%%\n", packet->ptt_lp_percentage);
    fprintf(f,"ple_percent         : %10d%%\n", packet->ple_percentage);
    fprintf(f,"bwe_percent         : %10d%%\n", packet->bwe_percentage);
    fprintf(f,"vbat_higher_vbus_p  : %10d%%\n", packet->vbat_higher_than_vbus_percentage);
    fprintf(f,"payload frames      : %10d\n",   packet->payload_frames);
    fprintf(f,"payload params      : %10d\n",   packet->payload_params);
    fprintf(f,"payload current img : %10d\n",   packet->payload_current_img_id);

    fprintf(f_dat, "%ld %ld %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",

	 t,
	 start_t,
	 packet->sclock,
	 packet->uptime,
	 packet->nrun,
	 packet->npayload,
	 packet->nwire,
	 packet->ntransponder,
	 packet->npayloaderrors_lastreset >> 4,
	 packet->npayloaderrors_lastreset & 0x0F,
	 packet->bate_mote >> 4, 



	 packet->bate_mote & 0x0f,
	 packet->nTasksNotExecuted,
	 packet->antennaDeployed,
	 packet->nExtEepromErrors,
	 packet->failed_task_id >> 6,
	 packet->nIOT,
	 packet->strfwd0,
	 packet->strfwd1,
	 packet->strfwd2,
	 packet->strfwd3,
	 packet->rx_percentage,
 	 packet->telemetry_percentage,
 	 packet->transponder_percentage,
	 packet->ptt_hp_percentage,
 	 packet->ptt_lp_percentage,
	 packet->ple_percentage,
 	 packet->bwe_percentage,
	 packet->vbat_higher_than_vbus_percentage,
	 packet->payload_frames);

}


void visualiza_powerrangespacket(uint8_t sat_id, power_ranges_packet * packet) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Power ranges packet received on local time %s ***\n", fecha);

    int8_t minicpus;
    int8_t maxicpus;
	
    maxicpus = packet->maxicpu;
    minicpus = packet->minicpu;

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    uint32_t minvcpu_temp = (packet->minvbatadc_lo_vcpuadc_hi << 4) & 0x0ff0;
             minvcpu_temp = minvcpu_temp | (packet->minvcpuadc_lo_free >> 4);

    uint32_t maxvcpu_temp = (packet->maxvbatadc_lo_vcpuadc_hi << 4) & 0x0ff0;
             maxvcpu_temp = maxvcpu_temp | (packet->maxvcpuadc_lo_free >> 4);

    fprintf(f,"sat_id         : %d (%s)\n", sat_id, source_desc(sat_id));

    fprintf(f,"sclock         : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);

    fprintf(f,"minvbus1       : %4d mV ADC\n", 1400*(packet->minvbusadc_vbatadc_hi >> 4)/1000);
    fprintf(f,"minvbat1       : %4d mV ADC\n", 1400*(((packet->minvbusadc_vbatadc_hi << 8) & 0x0f00) | ((packet->minvbatadc_lo_vcpuadc_hi >> 8) & 0x00ff))/1000);
    fprintf(f,"minvcpu        : %4d mV ADC\n", 1210*4096/minvcpu_temp);
    fprintf(f,"minvbus2       : %4d mV I2C\n", packet->minvbus2*16*4);
    fprintf(f,"minvbus3       : %4d mV I2C\n", packet->minvbus3*16*4);
    fprintf(f,"minvbat2       : %4d mV I2C\n", packet->minvbat2*16*4);
    fprintf(f,"minibat        : %4d mA I2C (Max current flowing into the battery)\n", -1*(packet->minibat));
    fprintf(f,"minicpu        : %4d mA I2C\n", minicpus);
    fprintf(f,"minipl         : %4d mA I2C (Payload current)\n", packet->minipl);
    fprintf(f,"maxvbus1       : %4d mV ADC\n", 1400*(packet->maxvbusadc_vbatadc_hi >> 4)/1000);
    fprintf(f,"maxvbat1       : %4d mV ADC\n", 1400*(((packet->maxvbusadc_vbatadc_hi << 8) & 0x0f00) | ((packet->maxvbatadc_lo_vcpuadc_hi >> 8) & 0x00ff))/1000);
    fprintf(f,"maxvcpu        : %4d mV ADC\n", 1210*4096/maxvcpu_temp);
    fprintf(f,"maxvbus2       : %4d mV I2C\n", packet->maxvbus2*16*4);
    fprintf(f,"maxvbus3       : %4d mV I2C\n", packet->maxvbus3*16*4);	
    fprintf(f,"maxvbat2       : %4d mV I2C\n", packet->maxvbat2*16*4);		
    fprintf(f,"maxibat        : %4d mA I2C (Max current flowing out from the battery)\n", packet->maxibat);
    fprintf(f,"maxicpu        : %4d mA I2C\n", maxicpus);	
    fprintf(f,"maxipl         : %4d mA I2C (Payload current)\n", packet->maxipl << 2);		
    fprintf(f,"\n");
    fprintf(f,"ibat_tx_off_charging           : %4d mA I2C\n", packet->ibat_rx_charging);
    fprintf(f,"ibat_tx_off_discharging        : %4d mA I2C\n", packet->ibat_rx_discharging);	
    fprintf(f,"ibat_tx_low_power_charging     : %4d mA I2C\n", packet->ibat_tx_low_power_charging);			
    fprintf(f,"ibat_tx_low_power_discharging  : %4d mA I2C\n", packet->ibat_tx_low_power_discharging);	
    fprintf(f,"ibat_tx_high_power_charging    : %4d mA I2C\n", packet->ibat_tx_high_power_charging);			
    fprintf(f,"ibat_tx_high_power_discharging : %4d mA I2C\n", packet->ibat_tx_high_power_discharging);

    fprintf(f_dat, "%ld %ld %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	t,
        start_t,
	packet->sclock,
  	1400*(packet->minvbusadc_vbatadc_hi >> 4)/1000,
	1400*(((packet->minvbusadc_vbatadc_hi << 8) & 0x0f00) | ((packet->minvbatadc_lo_vcpuadc_hi >> 8) & 0x00ff))/1000,
	1210*4096/minvcpu_temp,
     	packet->minvbus2*16*4,
	packet->minvbus3*16*4,
	packet->minvbat2*16*4,
	-1*(packet->minibat),
	minicpus,
 	packet->minipl,
 	1400*(packet->maxvbusadc_vbatadc_hi >> 4)/1000,
	1400*(((packet->maxvbusadc_vbatadc_hi << 8) & 0x0f00) | ((packet->maxvbatadc_lo_vcpuadc_hi >> 8) & 0x00ff))/1000,
	1210*4096/maxvcpu_temp,
 	packet->maxvbus2*16*4,
	packet->maxvbus3*16*4,
	packet->maxvbat2*16*4,
	packet->maxibat,
	maxicpus,
	packet->maxipl << 2,
	packet->ibat_rx_charging,
	packet->ibat_rx_discharging,
	packet->ibat_tx_low_power_charging,
	packet->ibat_tx_low_power_discharging,
	packet->ibat_tx_high_power_charging,
	packet->ibat_tx_high_power_discharging);

  
}


void visualiza_temprangespacket(uint8_t sat_id, temp_ranges_packet * packet) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Temperature ranges packet received on local time %s ***\n", fecha);

    uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;

    days      = (packet->sclock / 86400); // segundos que dan para dias enteros
    hours     = (packet->sclock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes   = (packet->sclock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds   = (packet->sclock % 60);

    fprintf(f,"sat_id         : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"sclock         : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->sclock, days, hours, minutes, seconds);

    if (packet->mintpa   == 255) fprintf(f,"mintpa         : \n"); else fprintf(f,"mintpa         : %+5.1f C MCP Temperature outside panel A\n", ((float)packet->mintpa/2)-40.0);
    if (packet->mintpb   == 255) fprintf(f,"mintpb         : \n"); else fprintf(f,"mintpb         : %+5.1f C MCP Temperature outside panel B\n", ((float)packet->mintpb/2)-40.0);
    if (packet->mintpc   == 255) fprintf(f,"mintpc         : \n"); else fprintf(f,"mintpc         : %+5.1f C MCP Temperature outside panel C\n", ((float)packet->mintpc/2)-40.0);
    if (packet->mintpd   == 255) fprintf(f,"mintpd         : \n"); else fprintf(f,"mintpd         : %+5.1f C MCP Temperature outside panel D\n", ((float)packet->mintpd/2)-40.0);
    if (packet->mintpe   == 255) fprintf(f,"mintpe         : \n"); else fprintf(f,"mintpe         : \n");
    if (packet->minteps  == 255) fprintf(f,"minteps        : \n"); else fprintf(f,"minteps        : %+5.1f C TMP100 Temperature EPS\n", ((float)packet->minteps/2)-40.0);
    if (packet->minttx   == 255) fprintf(f,"minttx         : \n"); else fprintf(f,"minttx         : %+5.1f C TMP100 Temperature TX\n", ((float)packet->minttx/2)-40.0);
    if (packet->minttx2  == 255) fprintf(f,"minttx2        : \n"); else fprintf(f,"minttx2        : %+5.1f C ADC Temperature TX\n", ((float)packet->minttx2/2)-40.0);
    if (packet->mintrx   == 255) fprintf(f,"mintrx         : \n"); else fprintf(f,"mintrx         : %+5.1f C ADC Temperature RX\n", ((float)packet->mintrx/2)-40.0);
    if (packet->mintcpu  == 255) fprintf(f,"mintcpu        : \n"); else fprintf(f,"mintcpu        : %+5.1f C INT Temperature CPU\n", ((float)packet->mintcpu/2)-40.0);
    if (packet->maxtpa   == 255) fprintf(f,"maxtpa         : \n"); else fprintf(f,"maxtpa         : %+5.1f C MCP Temperature outside panel A\n", ((float)packet->maxtpa/2)-40.0);
    if (packet->maxtpb   == 255) fprintf(f,"maxtpb         : \n"); else fprintf(f,"maxtpb         : %+5.1f C MCP Temperature outside panel B\n", ((float)packet->maxtpb/2)-40.0);
    if (packet->maxtpc   == 255) fprintf(f,"maxtpc         : \n"); else fprintf(f,"maxtpc         : %+5.1f C MCP Temperature outside panel C\n", ((float)packet->maxtpc/2)-40.0);
    if (packet->maxtpd   == 255) fprintf(f,"maxtpd         : \n"); else fprintf(f,"maxtpd         : %+5.1f C MCP Temperature outside panel D\n", ((float)packet->maxtpd/2)-40.0);
    if (packet->maxtpe   == 255) fprintf(f,"maxtpe         : \n"); else fprintf(f,"maxtpe         : \n");
    if (packet->maxteps  == 255) fprintf(f,"maxteps        : \n"); else fprintf(f,"maxteps        : %+5.1f C TMP100 Temperature EPS\n", ((float)packet->maxteps/2)-40.0);
    if (packet->maxttx   == 255) fprintf(f,"maxttx         : \n"); else fprintf(f,"maxttx         : %+5.1f C TMP100 Temperature TX\n", ((float)packet->maxttx/2)-40.0);
    if (packet->maxttx2  == 255) fprintf(f,"maxttx2        : \n"); else fprintf(f,"maxttx2        : %+5.1f C ADC Temperature TX\n", ((float)packet->maxttx2/2)-40.0);
    if (packet->maxtrx   == 255) fprintf(f,"maxtrx         : \n"); else fprintf(f,"maxtrx         : %+5.1f C ADC Temperature RX\n", ((float)packet->maxtrx/2)-40.0);
    if (packet->maxtcpu  == 255) fprintf(f,"maxtcpu        : \n"); else fprintf(f,"maxtcpu        : %+5.1f C INT Temperature CPU\n", ((float)packet->maxtcpu/2)-40.0);

    fprintf(f_dat, "%ld %ld %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
    t,
    start_t,
    packet->sclock,
    (float)(packet->mintpa/2)-40.0,
    (float)(packet->mintpb/2)-40.0,
    (float)(packet->mintpc/2)-40.0,
    (float)(packet->mintpd/2)-40.0,
    0.0,
    (float)(packet->minteps/2)-40.0,
    (float)(packet->minttx/2)-40.0,
    (float)(packet->minttx2/2)-40.0,
    (float)(packet->mintrx/2)-40.0,
    (float)(packet->mintcpu/2)-40.0,
    (float)(packet->maxtpa/2)-40.0,
    (float)(packet->maxtpb/2)-40.0,
    (float)(packet->maxtpc/2)-40.0,
    (float)(packet->maxtpd/2)-40.0,
    0.0,
    (float)(packet->maxteps/2)-40.0,
    (float)(packet->maxttx/2)-40.0,
    (float)(packet->maxttx2/2)-40.0,
    (float)(packet->maxtrx/2)-40.0,
    (float)(packet->maxtcpu/2)-40.0);


}


void visualiza_deploypacket(uint8_t sat_id, TLMBW * tlmbw) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Antenna deployment packet received on local time %s ***\n", fecha);

    fprintf(f,"sat_id                                     : %d (%s)\n", sat_id, source_desc(sat_id));

    fprintf(f, "estado pulsador inicio:fin:ahora           : %d:%d:%d\n",tlmbw->state_begin,tlmbw->state_end,tlmbw->state_now);
    fprintf(f, "tension bateria en circuito abierto Voc/mV : %d\n",tlmbw->v1oc);
    fprintf(f, "caida de tension deltaV/mV                 : %u\n",tlmbw->v1);
    fprintf(f, "corriente quemado Ibr/mA                   : %u\n",tlmbw->i1);
    fprintf(f, "corriente quemado Ibr,pk/mA                : %u\n",tlmbw->i1pk);
    fprintf(f, "resistencia interna bateria Rbat/mohm      : %u\n",tlmbw->r1);
    fprintf(f, "tiempo quemado observado t/s               : %d\n",tlmbw->td);

    fprintf(f_dat, "%ld %ld ", t, start_t);
    fprintf(f_dat, "%d %u %u %u %u %u %d %d %d %d %d %d %d %d %d %d\n",
	 tlmbw->v1oc,
	 tlmbw->v1,
	 tlmbw->i1,
	 tlmbw->i1pk,
	 tlmbw->r1,
         tlmbw->v2oc,
	 tlmbw->v2,
	 tlmbw->r2,
	 tlmbw->t0,
	 tlmbw->td,
      	 tlmbw->state_begin,
         tlmbw->state_end,
         tlmbw->state_now,
         tlmbw->enable,
         tlmbw->counter,
         tlmbw->tmp);

   
}


void visualiza_ine(uint8_t sat_id, INE * ine) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Extended power (ine) packet received on local time %s ***\n", fecha);

    fprintf(f,"sat_id : %d (%s)\n\n", sat_id, source_desc(sat_id));

    fprintf(f_dat, "%ld %ld ", t, start_t);

    for(int n=0; n!= MAXINE;n++) {

	fprintf(f,     "%2d %4s | VI %5d [mV] %5d [mA] | AOP %5d [mW] | VIPpeak %5d [mV] %5d [mA] %5d [mW]\n", n, name[n], ine[n].v,  ine[n].i,  ine[n].p, ine[n].vp,  ine[n].ip,  ine[n].pp);
	fprintf(f_dat, "%2d %4s %5d %5d %5d %5d %5d %5d ",                          n, name[n], ine[n].v,  ine[n].i,  ine[n].p, ine[n].vp,  ine[n].ip,  ine[n].pp);
 
   }

   fprintf(f_dat, "\n");

}



void add_padding_codec2(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    // Cada trama: 28 bits útiles → 32 bits (añadir 4 bits a 0)
    const int BITS_PER_FRAME = 28;

    size_t total_bits = in_len * 8;
    size_t n_frames = total_bits / BITS_PER_FRAME;

    uint64_t bitbuf = 0;
    int bits_in_buf = 0;
    size_t in_pos = 0;
    size_t out_pos = 0;

    for (size_t f = 0; f < n_frames; f++) {
        // Asegurarnos de tener 28 bits cargados en bitbuf
        while (bits_in_buf < BITS_PER_FRAME && in_pos < in_len) {
            bitbuf = (bitbuf << 8) | in[in_pos++];
            bits_in_buf += 8;
        }

        // Extraer 28 bits (MSB-first)
        uint32_t frame_bits = (bitbuf >> (bits_in_buf - BITS_PER_FRAME)) & 0x0FFFFFFF;
        bits_in_buf -= BITS_PER_FRAME;

        // Añadir 4 bits de padding (LSB = 0)
        frame_bits <<= 4;  // los 4 bits bajos quedan a 0

        // Escribir los 4 bytes resultantes en orden MSB-first
        out[out_pos + 0] = (frame_bits >> 24) & 0xFF;
        out[out_pos + 1] = (frame_bits >> 16) & 0xFF;
        out[out_pos + 2] = (frame_bits >> 8) & 0xFF;
        out[out_pos + 3] = frame_bits & 0xFF;
        out_pos += 4;
    }

    *out_len = out_pos;
}

void visualiza_codec2(uint8_t sat_id, codec2_frame_st * codec2) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Codec2 frame received on local time %s ***\n", fecha);

    fprintf(f,"sat_id    : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"frame num : %d\n", codec2->frame_number);
    fprintf(f,"data      : ");

    uint8_t codec2_con_padding[64];
    size_t out_len;

    uint8_t xor_codec2[35] = {
				0xed, 0x15, 0xd5, 0x3b, 0x34, 0x70, 0xe0, 0xfd, 0xed, 0x83, 0x90, 0xdb,
				0xaa, 0x2e, 0x25, 0xd6, 0x5e, 0x81, 0x41, 0x86, 0xbd, 0x67, 0x79, 0x5d,
				0x70, 0xa1, 0x13, 0xce, 0x50, 0x0c, 0x19, 0xca, 0xfb, 0x44, 0x0d };

    for (uint8_t i = 0; i < 35; i++)
	codec2->codec2_bits[i] = codec2->codec2_bits[i] ^ xor_codec2[i]; 

    add_padding_codec2(codec2->codec2_bits, sizeof(codec2->codec2_bits), codec2_con_padding, &out_len);
	
    for (uint8_t i = 0; i < 40; i++) {
    
	fprintf(f,"%02X ", codec2_con_padding[i]);
        fwrite(&codec2_con_padding[i], 1, 1, f_dat);

    }

    fprintf(f,"\n");

}


void visualiza_ssdv(uint8_t sat_id, ssdv_file_frame * ssdv) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** SSDV frame received on local time %s ***\n", fecha);

    uint16_t packetID  = (ssdv->packetID >> 8) | (ssdv->packetID << 8);
    uint16_t mcuIndex  = (ssdv->mcuIndex >> 8) | (ssdv->mcuIndex << 8);

    fprintf(f,"sat_id    : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"image id  : %d\n", ssdv->imageID);
    fprintf(f,"packet id : %d\n", packetID);
    fprintf(f,"width id  : %03d\n", 16*ssdv->width);
    fprintf(f,"height id : %03d\n", 16*ssdv->height);
    fprintf(f,"flags     : %03d\n", ssdv->flags);
    fprintf(f,"mcuOffset : %03d\n", ssdv->mcuOffset);
    fprintf(f,"mcuIndex  : %d\n", mcuIndex);
    fprintf(f,"checksum  : ");

    for (int i = 0; i < 4; i++) {
	uint32_t valor = (ssdv->checksum >> (i*8)) & 0xFF;
	fprintf(f,"%02X", valor);
    }

    fprintf(f,"\n");
    fprintf(f,"FEC       : ");

    for (int i = 0; i < 32; i++) 
    	fprintf(f,"%02X", ssdv->FEC[i]);

    // binario

    // ponemos los primeros 5 bytes a mano ya que el tm no los tiene
  
    ssdv->training     = 0x55;
    ssdv->ssdv_type    = 0x66;
    ssdv->sync_part1   = 0x35BF;
    ssdv->size         = 251;

    for (int i=0; i < sizeof(ssdv_file_frame); i++) fwrite((uint8_t*)ssdv+i, 1,1, f_dat);

    fprintf(f,"\n");

}


void visualiza_PN9(uint8_t sat_id, uint8_t * pn9) {

    uint16_t match = 0, wrong = 0;
    extern uint8_t PN9_RAW_DATA[PN9_RAW_DATA_SIZE];

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** PN9 frame received on local time %s ***\n", fecha);

    fprintf(f,"sat_id    : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"data      : ");

    for (uint16_t i = 0; i < PN9_RAW_DATA_SIZE; i++) {

        fprintf(f,"%02X ", pn9[i]);
	fprintf(f_dat, "%02X ", pn9[i]);

	if (pn9[i] == PN9_RAW_DATA[i]) match++; else wrong++;

    }

    fprintf(f,"\n\n");

    if (match+wrong > 0) fprintf(f,"%s Data matching rate : %6.2f%% (Total %d, failed %d)\n", fecha, (float)(match*100.0/(match+wrong)), match+wrong, wrong);
	else fprintf(f,"%s Data matching rate : N/A\n", fecha);

}


void visualiza_bbs(uint8_t sat_id, bbs_packet_st * bbs) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** BBS packet received on local time %s ***\n", fecha);

    fprintf(f,"sat_id        : %d (%s)\n", sat_id, source_desc(sat_id));

    fprintf(f,"\n");
    fprintf(f,"BBS contents  : Callsign Message Codec2 frames\n");

    for (uint8_t i = 0; i < 5; i++) {

	fprintf(f, "                ");
        for (uint8_t j = 0; j < 6; j++) fprintf(f, "%c", bbs->callsigns[i*6+j]);

        fprintf(f, "   ");
        for (uint8_t j = 0; j < 7; j++) fprintf(f, "%c", bbs->messages[i*7+j]);

        fprintf(f, " ");

        fprintf(f, "%d\n", bbs->num_codec2_frames[i]);
    }

}


void visualiza_efemeridespacket(uint8_t sat_id,_frm * ef) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    time_t tunix = ef->utc;
    struct tm *timeinfo = gmtime(&tunix);

    int year = timeinfo->tm_year + 1900;
    int month = timeinfo->tm_mon + 1;
    int day = timeinfo->tm_mday;
    int hour = timeinfo->tm_hour;
    int minute = timeinfo->tm_min;
    int second = timeinfo->tm_sec;

    char fechahora[64];

    time_t tunix_tle = ef->sat.tle.epoch;
    struct tm *timeinfo_tle = gmtime(&tunix_tle);

    int year_tle = timeinfo_tle->tm_year + 1900;
    int month_tle = timeinfo_tle->tm_mon + 1;
    int day_tle = timeinfo_tle->tm_mday;
    int hour_tle = timeinfo_tle->tm_hour;
    int minute_tle = timeinfo_tle->tm_min;
    int second_tle = timeinfo_tle->tm_sec;

    char fechahora_tle[64];

    sprintf(fechahora, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    sprintf(fechahora_tle, "%04d-%02d-%02d %02d:%02d:%02d", year_tle, month_tle, day_tle, hour_tle, minute_tle, second_tle);

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Ephemeris packet received on local time %s ***\n", fecha);

    fprintf(f,"sat_id      : %d (%s)\n", sat_id, source_desc(sat_id));
    fprintf(f,"UTC time    : %d seconds (%s) (on board) YYYY-MM-DD HH24:MI:SS\n", ef->utc, fechahora);
    fprintf(f,"lat         : %d degrees\n", ef->lat);
    fprintf(f,"lon         : %d degrees\n", ef->lon);
    fprintf(f,"alt         : %d km\n", ef->alt);
    if (ef->utc == 0 || ef->sat.tle.epoch == 0) fprintf(f,"zone        : unknown\n"); else fprintf(f,"zone        : %s\n", overflying(ef->lat, ef->lon));
    fprintf(f,"Adr         : %d\n", ef->sat.adr);
    fprintf(f,"Ful         : %d\n", ef->sat.ful);
    fprintf(f,"Fdl         : %d\n", ef->sat.fdl);
    fprintf(f,"Epoch       : %d seconds (%s) (TLE time) YYYY-MM-DD HH24:MI:SS\n", ef->sat.tle.epoch, fechahora_tle);
    fprintf(f,"Xndt2o      : %f\n", ef->sat.tle.xndt2o);
    fprintf(f,"Xndd6o      : %f\n", ef->sat.tle.xndd6o);
    fprintf(f,"Bstar       : %f\n", ef->sat.tle.bstar);
    fprintf(f,"Xincl       : %f\n", ef->sat.tle.xincl);
    fprintf(f,"Xnodeo      : %f\n", ef->sat.tle.xnodeo);
    fprintf(f,"Eo          : %f\n", ef->sat.tle.eo);
    fprintf(f,"Omegao      : %f\n", ef->sat.tle.omegao);
    fprintf(f,"Xmo         : %f\n", ef->sat.tle.xmo);
    fprintf(f,"Xno         : %f\n", ef->sat.tle.xno);

    fprintf(f_dat, "%ld %ld ", t, start_t);
    fprintf(f_dat, "%d %d %d %d %d %f %f %f %f %f %f %f %f %f %d %d %d 0\n",
	ef->utc,
  	ef->sat.adr,
	ef->sat.ful,
	ef->sat.fdl,
	ef->sat.tle.epoch,
	ef->sat.tle.xndt2o,
	ef->sat.tle.xndd6o,
	ef->sat.tle.bstar,
	ef->sat.tle.xincl,
	ef->sat.tle.xnodeo,
	ef->sat.tle.eo,
	ef->sat.tle.omegao,
	ef->sat.tle.xmo,
	ef->sat.tle.xno,
	ef->lat,
	ef->lon,
	ef->alt);


}


void visualiza_time_series_packet(uint8_t sat_id, time_series_packet * packet) {

    time_t t = time(NULL)+diff_time;
    struct tm tm;
    tm = *localtime(&t);

    char fecha[64];

    char * desc_time_serie [] = { "signal_peak", "moda_noise", "vbat1 - bat voltage read in EPS.ADC", "tcpu", "tpb", "tpa-tpd mean_temp" };

    sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(f,"*** Time series packet received on local time %s ***\n", fecha);

    uint16_t days_tx = 0, hours_tx = 0, minutes_tx = 0, seconds_tx = 0;

    days_tx      = (packet->clock / 86400); // segundos que dan para dias enteros
    hours_tx     = (packet->clock % 86400)/3600; // todo lo que no de para un dia dividido entre 3600 seg que son una hora
    minutes_tx   = (packet->clock % 3600)/60; // todo lo que no de para una hora dividido entre 60
    seconds_tx   = (packet->clock % 60);

    fprintf(f,"sat_id      : %d (%s)\n", sat_id, source_desc(sat_id));

    fprintf(f,"sclock	    : %d seconds (%d days and %02d:%02d:%02d hh:mm:ss)\n",packet->clock, days_tx, hours_tx, minutes_tx, seconds_tx);
    if (packet->variable < 6) fprintf(f,"Variable    : %d (%s)\n", packet->variable, desc_time_serie[packet->variable]);
	else fprintf(f,"Variable    : %d (%s)\n", packet->variable, "Unknown");

    fprintf(f_dat, "%ld %ld ", t, start_t);

    uint32_t vbat1;

    for (uint8_t i = 0; i < TIME_SERIES_DATA_SIZE; i++){

    	switch(packet->variable) {

	   case 0:

		fprintf(f,"Data [%03d]  :  %.2X (%03d dB) Sampled (T-%03d) minutes\n", i, packet->data[i], packet->data[i], (29-i)*3);
		fprintf(f_dat, "%d ", packet->data[i]);

	   break;

	   case 1:

		fprintf(f,"Data [%03d]  :  %.2X (%03d dB) Sampled (T-%03d) minutes\n", i, packet->data[i], packet->data[i], (29-i)*3);
		fprintf(f_dat, "%d ", packet->data[i]);

	   break;

    	   case 2:

   		vbat1 = (packet->data[i] << 4);
            	vbat1 = vbat1 * 1400;
             	vbat1 = vbat1 / 1000;

    		fprintf(f,"Data [%03d]  : %4d mV bat voltage read in EPS.ADC - Sampled at (T-%03d) minutes\n", i, vbat1, (29-i)*3);
		fprintf(f_dat, "%d ", vbat1);

	   break;

           case 3:

    		if (packet->data[i]  == 255) fprintf(f,"Data [%03d]  :       degC temperature in CPU.ADC - Sampled at (T-%03d) minutes\n", i, (29-i)*3);
		else fprintf(f,"Data [%03d]  : %+5.1f degC temperature in CPU.ADC - Sampled at (T-%03d) minutes\n", i, ((float)packet->data[i]/2)-40.0, (29-i)*3);

		fprintf(f_dat, "%5.1f ", (float)(packet->data[i])/2.0-40.0);

           break;

           case 4:

                if (packet->data[i]  == 255) fprintf(f,"Data [%03d]  :       degC temperature in SPB.I2C - Sampled at (T-%03d) minutes\n", i, (29-i)*3);
                else fprintf(f,"Data [%03d]  : %+5.1f degC temperature in SPA.I2C - Sampled at (T-%03d) minutes\n", i, ((float)packet->data[i]/2)-40.0, (29-i)*3);

                fprintf(f_dat, "%5.1f ", (float)(packet->data[i])/2.0-40.0);

           break;

           case 5:

                if (packet->data[i]  == 255) fprintf(f,"Data [%03d]  :       degC temperature mean 4 panels (SPA-SPD).I2C - Sampled at (T-%03d) minutes\n", i, (29-i)*3);
                else fprintf(f,"Data [%03d]  : %+5.1f degC temperature mean 4 panels (SPA-SPD).I2C - Sampled at (T-%03d) minutes\n", i, ((float)packet->data[i]/2)-40.0, (29-i)*3);

                fprintf(f_dat, "%5.1f ", (float)(packet->data[i])/2.0-40.0);

           break;

    	   default:

        	fprintf(f,"Data [%03d]	:  %.2X (%03d) Sampled (T-%03d) minutes\n", i, packet->data[i], packet->data[i], (29-i)*3);
                fprintf(f_dat, "%d ", packet->data[i]);

	   break;

	}

    }

    fprintf(f_dat, "\n");

}


char * source_desc(uint8_t sat_id) {

	static	char * unknown = "Unknown";
	static	char * sat_desc [] = {
				"Unknown", 		// 0
				"Unknown",  	// 1
				"HADES-ICM", 	// 2
				"SpinnyONE HADES-SA", 		// 3
				"Unknown", 		// 4
				"Unknown", 		// 5
				"Unknown", 		// 6
				"Unknown", 		// 7
				"Unknown", 		// 8
				"GENESIS-M",	// 9
				"Unknown", 		// 10
				"MARIA-G", 		// 11
				"UNNE-1B",  		// 12
				"HADES-R", 		// 13
		};

	if (sat_id > 13) return unknown;

	return (sat_desc[sat_id]);

}


void procesar(char * file_name) {

   uint8_t RX_buffer[RX_BUFFER_SIZE];

   uint16_t telemetry_size = 0;

   uint8_t type   = 0;
   uint8_t source = 0;

   power_packet powerp;
   temp_packet tempp;
   status_packet statusp;
   power_ranges_packet powerrangesp;
   temp_ranges_packet temprangesp;
   INE ine[MAXINE];
   TLMBW tlmbw;
   _frm efemeridesp;
   codec2_frame_st codec2;
   ssdv_file_frame ssdv;
   uint8_t pn9[PN9_RAW_DATA_SIZE];
   bbs_packet_st bbs;

   time_series_packet timeseriesp;

   char * desc [] = { "-", "pwr", "tmp", "status", "pwr_ranges", "tmp_ranges", "unknown", "unknown", "antenna", "extended power - ina219", "SSDV", "CODEC2 voice", "ephemeris",
	"PN9 random data", "time series", "BBS" };

   time_t t = time(NULL) + diff_time;
   struct tm tm = *localtime(&t);
   char fecha[64];

   memset(fecha, 0, sizeof(fecha));
   sprintf(fecha, "%d%02d%02d-%02d:%02d:%02d :", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

   FILE * f_entrada = fopen(file_name, "r");

   if (f_entrada == NULL) {

        printf("%s Could not open file %s\n", fecha, file_name);
        printf("\n");
        return;

   }

   printf("%s Processing file %s\n", fecha, file_name);
   printf("%s Bytes read : ", fecha);

   uint8_t num_bytes = 0;

   int status = 0;

   char temp[TEMP_BUFFER_SIZE];
   uint8_t current_byte = 0;

   memset(temp, 0, sizeof(temp));

   do {

        status = fscanf(f_entrada, "%s", temp);

        if (status != 1) break;

        if (strlen(temp) != 2) {

       		printf("\n\n%s is not a valid byte value. Two digits are needed\n", temp);
                fclose(f_entrada);
                return;

        }

	printf("%s ", temp);
        fflush(stdout);

        char h_digit = temp[0];
        char l_digit = temp[1];

        int16_t h = hex2int(h_digit);
        int16_t l = hex2int(l_digit);

        current_byte  = (h << 4) | l;

        // first byte indicates satellite id and packet type

        if (num_bytes == 0) {

             type   = h;
             source = l;

        }

        if (num_bytes == 0 && (source != 3)) {

             printf("\n\nSatellite is unknown (%d)\n\n", source);
             fclose(f_entrada);
             return;

        }

        if (num_bytes == 0 && (type == 0 || type > 15)) {

             printf("\n\nPacket type is unknown (%d)\n\n", type);
             fclose(f_entrada);
             return;

        }

        RX_buffer[num_bytes] = current_byte;

        if (num_bytes == RX_BUFFER_SIZE-1) {

             printf("\n\nPacket too long\n\n");
             fclose(f_entrada);
             return;


        }

        num_bytes++;


   } while (1);

   fclose(f_entrada);

   printf("\n");

   telemetry_size = telemetry_packet_size(source, type); // quitamos el propio campo type

   printf("%s Satellite id %X (%s) packet type %d (%s). Expected bytes are %d for this packet type\n", fecha, source, source_desc(source), type, desc[type], telemetry_size);
   fflush(stdout);

   if (num_bytes != telemetry_size) {

   	printf("%s Received bytes are %d - Aborting\n\n", fecha, num_bytes);
   	fflush(stdout);
	return;

   } else {

        printf("%s Received bytes match\n", fecha);
        fflush(stdout);

   }

   char name_tlm_con_fecha[128];
   char name_tlm_sin_fecha[128];
   char name_dat[128];

   char fecha_fichero[64];

   sprintf(fecha_fichero, "%d%02d%02d-%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

   switch(type) {

	case 10: // ssdv

             	memset(&ssdv, 0, sizeof(ssdv));
             	memcpy((void*)&ssdv+5, (void*)RX_buffer, telemetry_size);

		uint16_t packetID  = (ssdv.packetID >> 8) | (ssdv.packetID << 8);
    		uint8_t imageID    = ssdv.imageID;

                sprintf(name_tlm_con_fecha, "%s_sat_%02d_type_%02d_ssdv_img_%03d_packet_%04d.tlm", fecha_fichero, source, type, imageID, packetID);
                sprintf(name_tlm_sin_fecha,    "sat_%02d_type_%02d_ssdv_img_%03d_packet_%04d.tlm", source, type, imageID, packetID);
                sprintf(name_dat,           "%s_sat_%02d_type_%02d_ssdv_img_%03d_packet_%04d.bin", fecha_fichero, source, type, imageID, packetID);


	break;

        case 11: // codec2

                // indicamos el orden en que se ha recibido este frame y el numero de frame que viene en el paquete

                memset(&codec2, 0, sizeof(codec2));
                memcpy((void*)&codec2+PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);

                sprintf(name_tlm_con_fecha, "%s_sat_%02d_type_%02d_codec2_frame_%03d.tlm", fecha_fichero, source, type, codec2.frame_number);
                sprintf(name_tlm_sin_fecha, "sat_%02d_type_%02d_codec2_frame_%03d.tlm", source, type, codec2.frame_number);
                sprintf(name_dat,           "%s_sat_%02d_type_%02d_codec2_frame_%03d.bin", fecha_fichero, source, type, codec2.frame_number);

                break;

	case 14:

                memset(&timeseriesp, 0, sizeof(timeseriesp));
                memcpy((void*)&timeseriesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);

                sprintf(name_tlm_con_fecha, "%s_sat_%02d_type_%02d_%02d.tlm", fecha_fichero, source, type, timeseriesp.variable);
                sprintf(name_tlm_sin_fecha,    "sat_%02d_type_%02d_%02d.tlm", source, type, timeseriesp.variable);
                sprintf(name_dat,              "sat_%02d_type_%02d_%02d.dat", source, type, timeseriesp.variable);


 	break;

	default:

		sprintf(name_tlm_con_fecha, "%s_sat_%02d_type_%02d.tlm",fecha_fichero, source, type);
		sprintf(name_tlm_sin_fecha,    "sat_%02d_type_%02d.tlm", source, type);
		sprintf(name_dat,              "sat_%02d_type_%02d.dat", source, type);
			
	break;

   }

   f = fopen(name_tlm_con_fecha,"w+");	
				
   if (f == NULL) {

	printf("\n\nFailed creating file %s\n", name_tlm_con_fecha);
	return;

   }

   f_dat=fopen(name_dat,"a+");

   if (f_dat == NULL) {

	printf("\n\nFailed creating file %s\n", name_dat);
	fclose(f);
        return;

   }

   switch(type) {

	case 1:
        	memset(&powerp, 0, sizeof(powerp));
                memcpy((void*)&powerp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
		visualiza_powerpacket(source, &powerp);
	break;

	case 2:
                memset(&tempp, 0, sizeof(tempp));
                memcpy((void*)&tempp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
		visualiza_temppacket(source, &tempp);
	break;

        case 3:
                memset(&statusp, 0, sizeof(statusp));
                memcpy((void*)&statusp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_statuspacket(source, &statusp);
        break;

        case 4:
                memset(&powerrangesp, 0, sizeof(powerrangesp));
                memcpy((void*)&powerrangesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_powerrangespacket(source, &powerrangesp);
        break;

        case 5:  
                memset(&temprangesp, 0, sizeof(temprangesp));
                memcpy((void*)&temprangesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_temprangespacket(source, &temprangesp);
        break;

	case 8: 
                memset(&tlmbw, 0, sizeof(tlmbw));
                memcpy((void*)&tlmbw, (void*)RX_buffer+1, telemetry_size-3);  // estas estructuras no llevan tipo ni CRC
                visualiza_deploypacket(source, &tlmbw);
        break;

        case 9:  
                memset(&ine, 0, sizeof(ine));
                memcpy((void*)ine, (void*)RX_buffer+1, telemetry_size-3);  //  estas estructuras no llevan tipo ni CRC
                visualiza_ine(source, ine);
        break;

        case 10:
                memset(&ssdv, 0, sizeof(ssdv));
                memcpy((void*)&ssdv+5, (void*)RX_buffer, telemetry_size);
                visualiza_ssdv(source, &ssdv);
        break;

	case 11:
		memset(&codec2, 0, sizeof(codec2));
		memcpy((void*)&codec2+PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
		visualiza_codec2(source, &codec2);
	break;

	case 12:
                memset(&efemeridesp, 0, sizeof(efemeridesp));
                memcpy((void*)&efemeridesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);  
                visualiza_efemeridespacket(source, &efemeridesp);
	break;

	case 13:
                memset(pn9, 0, sizeof(pn9));
                memcpy((void*)pn9, (void*)RX_buffer+1, telemetry_size-1);
                visualiza_PN9(source, pn9);

	break;

        case 14:
                memset(&timeseriesp, 0, sizeof(timeseriesp));
                memcpy((void*)&timeseriesp + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                visualiza_time_series_packet(source, &timeseriesp);
        break;

	case 15:
                memset(&bbs, 0, sizeof(bbs));
                memcpy((void*)&bbs + PACKET_HEADER_SIZE, (void*)RX_buffer, telemetry_size);
                visualiza_bbs(source, &bbs);
	break;

	default:

		
	break;
	

   } // switch

   fclose(f);
   fclose(f_dat);

   // hacer una copia del fichero pero sin fecha

   FILE * f_orig, * f_dest;

   printf("\n");

   f_orig = fopen(name_tlm_con_fecha,"r");	
   f_dest = fopen(name_tlm_sin_fecha,"w+");
		
   uint8_t buffer;
   uint8_t nleidos;
	
   do {

	nleidos = fread(&buffer, 1, 1,f_orig);

   	if (nleidos == 1) {

		fwrite(&buffer, 1, 1, f_dest);
		printf("%c", buffer);

	}

   } while (nleidos == 1);

   fclose(f_orig);
   fclose(f_dest);

   printf("\n\n");

   fflush(stdout);

}


void /* Calculate if a satellite is above any zone */
trx(double lat, double lon, int latc, int lonc, int rc, int *inout)
{
    double rc_km, dlat, dlon, a_t, c_t, d_t;
    rc_km = rc * (pi / 180) * 6371; // radio del círculo en km
    dlat = (lat - latc)* (pi / 180) / 2.0;
    dlon = (lon - lonc)* (pi / 180) / 2.0;
    a_t = sin(dlat) * sin(dlat) + cos(latc * (pi / 180) ) * cos(lat * (pi / 180) ) * sin(dlon) * sin(dlon);
    c_t = 2.0 * atan2(sqrt(a_t), sqrt(1.0 - a_t));
    d_t = 6371 * c_t;

    if (d_t <= rc_km){
        *inout = 1;
    } else {
        *inout = 0;
    }
} /* trx */

char * overflying(double lat, double lon)
{
	//LAT,LONG,RADIO
	int zones[nzones][3]={			{ 54,  -4,  7},  // Reino Unido 
						{ 42,  -4, 6},	// Europa (S) 
						{ 48,  16, 12},	// Europa (M) 
						{ 64,  18,  7},	// Europa (N) 						
						{ 36, 139, 11},	// Japon (N)  						
						{ 10, 115, 23},	// Asia (S)						
						{ 27,  98, 27},	// Asia (E)				
						{ 32,  61, 23},	// Asia (W)					
						{ 24,  44, 12},	// Arabia Saudi 
						{ 60, 151, 19},	// Rusia (E) 
						{ 62, 103, 19},	// Rusia (C) 
						{ 57,  53, 17},	// Rusia (W) 	  
						{-21,  80, 35},	// Oceano Indico   
						{-50,  40, 20},	// Oceano Indico 
						{-22, 134, 26},	// Oceania 
						{  0,  17, 38},	// Africa 
						{-40, -14, 27},	// Oceano Atlantico Sur 
						{-48, -39, 18},	// Oceano Atlantico Sur 
						{ 33, -37, 23},	// Oceano Atlantico Norte 
						{ 71, -37, 12},	// Groenlandia 		
						{ -8, -59, 24},	// America del Sur 
						{-40, -65, 19},	// America del Sur 
						{ 16, -88, 17},	// Centro America 
						{ 47, -97, 30},	// America del Norte 
						{ 63,-151,  8},	// America del Norte	
						{ 72, -98, 10},	// America del Norte				
						{-90,   0, 22},	// Antartida 
						{ 17,-173, 47},	// Oceano Pacifico
						{-25,-128, 47},	// Oceano Pacifico
						{-49, 142, 28},	// Oceano Pacifico
						{-38,-165, 34},	// Oceano Pacifico
						{ 90, 140, 10},	// Oceano Artico
						{ 90,   60, 8},	// Oceano Artico
						{ 90,  168, 8}};// Oceano Artico
				  
	char countries[nzones][20]={"United Kingdom","Europe","Europe","Europe","Japan","Asia","Asia","Asia",
	"Saudi Arabia","Russia","Russia","Russia","the Indian Ocean","the Indian Ocean","Oceania","Africa",
	"the Atlantic Ocean","the Atlantic Ocean","the Atlantic Ocean","Greenland","South America","South America",
	"Central America","North America","North America","North America","Antartica","the Pacific Ocean",
	"the Pacific Ocean","the Pacific Ocean","the Pacific Ocean","the Artic Ocean","the Artic Ocean","the Artic Ocean"};	

	int i; int p = 0;
    for (i = 0; i < nzones; i++) {
        int inout = -1;
        trx(lat, lon, zones[i][0], zones[i][1], zones[i][2], &inout);
        if (inout == 0)continue;
        p = 1; break;
    }
	
    static char result[128];
    memset(result, 0, sizeof(result));

	if(p==1)snprintf(result, sizeof(result), "Flying over %s", countries[i]);
	if(p==0)snprintf(result, sizeof(result), "The overflown area is not found in the database");
    return result;
}


int16_t hex2int(char c) {

    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    return -1;

}

