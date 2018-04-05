/*************************************************************************
 * Lab 6 - Test 2 - Hunsung Lee & Jonghoon Choi
 * This test is the example on Lecture 14, Slide 11
************************************************************************/
 
#define NDEBUG
#include "assert.h"
#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "shared_structs.h"



/*--------------------------*/
/* Parameters for test case */
/*--------------------------*/
 
/* Stack space for processes */
#define NRT_STACK 80
#define RT_STACK  80
lock_t l;
lock_t l2;
 
 void setup(){
	// PINS SETUP
	//PTB9 is output voltage
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	PORTB->PCR[9] = PORT_PCR_MUX(001);
	PTB->PDDR |= (1 << 9);
		 
	//ADC MODULE SETUP
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	ADC0_CFG1 |= ADC_CFG1_MODE(3);
	ADC0_SC1A |= ADC_SC1_ADCH(31); 
}



/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/
/* Process start time */
realtime_t t_pRT1start = {0, 0};
realtime_t t_pRT2start = {0, 50};
realtime_t t_pRT3start = {0, 100};
realtime_t t_pRT4start = {0, 300};

/* Process deadline */
realtime_t t_pRT1dead = {1, 0};
realtime_t t_pRT2dead = {0, 800};
realtime_t t_pRT3dead = {0, 600};
realtime_t t_pRT4dead = {0, 400};

/* Process period */
realtime_t t_pRT1period = {1, 0};
 
/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}



/*----------------------------------------------------
 * 	Periodic real-time process 1
 *----------------------------------------------------*/
 
void p1(void) {
	l_lock(&l);
	
	PIT->CHANNEL[0].TCTRL = 0x1;
	debug_printf("P1 - Critical Section \r\n");
	short volPTB2 = ADC_read16b(12);
 	short capPTB2 = calculateCapacitance(100, volPTB2, 3); //3 or 5
	debug_printf("Voltage PTB2: %5d \r\n",volPTB2);
	debug_printf("Capacitance PTB2: %5d \r\n",capPTB2);
	PIT->CHANNEL[0].TCTRL = 0x3;
	
	for(int x = 0; x < 10; x++){
		PIT->CHANNEL[0].TCTRL = 0x1;
		debug_printf("P1 - CS Cycle %d \r\n", x);
		LEDBlue_Toggle();
		mediumDelay();
		PIT->CHANNEL[0].TCTRL = 0x3;
	}
	
	l_unlock(&l);
}

/*----------------------------------------------------
 * 	Periodic real-time process 2
 *----------------------------------------------------*/
void p2(void) {
	l_lock(&l2);
	
	PIT->CHANNEL[0].TCTRL = 0x1;
	debug_printf("P2 - Critical Section \r\n");
	short volPTB3 = ADC_read16b(13); 
	short capPTB3 = calculateCapacitance(100, volPTB3, 3); //3 or 5
	debug_printf("Voltage PTB3: %5d \r\n",volPTB3);
	debug_printf("Capacitance PTB3: %5d \r\n",capPTB3);
	PIT->CHANNEL[0].TCTRL = 0x3;

	for(int x = 0; x < 10; x++){
		PIT->CHANNEL[0].TCTRL = 0x1;
		debug_printf("P2 - CS Cycle %d \r\n", x);
		LEDRed_Toggle();
		mediumDelay();
		PIT->CHANNEL[0].TCTRL = 0x3;
	}
	
	l_unlock(&l2);
}

/*----------------------------------------------------
 * 	Periodic real-time process 3
 *----------------------------------------------------*/
void p3(void) {
	PIT->CHANNEL[0].TCTRL = 0x1;
	debug_printf("P3 - NonCritical Section \r\n");
	PIT->CHANNEL[0].TCTRL = 0x3;
	for(int x = 0; x < 2; x++){
		PIT->CHANNEL[0].TCTRL = 0x1;
		debug_printf("P3 - NCS Cycle %d \r\n", x);
		LEDGreen_Toggle();
		mediumDelay();
		PIT->CHANNEL[0].TCTRL = 0x3;
	}
	
	l_lock(&l);
	PIT->CHANNEL[0].TCTRL = 0x1;
	debug_printf("P3 - Critical Section \r\n");
	short volPTB2 = ADC_read16b(12);
  	short capPTB2 = calculateCapacitance(100, volPTB2, 3); //3 or 5
	debug_printf("Voltage PTB2: %5d \r\n",volPTB2);
	debug_printf("Capacitance PTB2: %5d \r\n",capPTB2);
	PIT->CHANNEL[0].TCTRL = 0x3;
	
	for(int x = 0; x < 10; x++){
		PIT->CHANNEL[0].TCTRL = 0x1;
		debug_printf("P3 - CS Cycle %d \r\n", x);
		LEDGreen_Toggle();
		mediumDelay();
		PIT->CHANNEL[0].TCTRL = 0x3;
	}
	
	l_unlock(&l);
}

void p4(void) {
	
	PIT->CHANNEL[0].TCTRL = 0x1;
	debug_printf("P4 - NonCritical Section \r\n");
	PIT->CHANNEL[0].TCTRL = 0x3;

	for(int x = 0; x < 2; x++){
		PIT->CHANNEL[0].TCTRL = 0x1;
		debug_printf("P4 - NCS Cycle %d \r\n", x);
		LEDBlue_Toggle();
		LEDRed_Toggle();
		mediumDelay();
		PIT->CHANNEL[0].TCTRL = 0x3;
	}
	
	l_lock(&l2);
	PIT->CHANNEL[0].TCTRL = 0x1;
	debug_printf("P4 - Critical Section \r\n");
	short volPTB3 = ADC_read16b(13); 
	short capPTB3 = calculateCapacitance(100, volPTB3, 3); //3 or 5
	debug_printf("Voltage PTB3: %5d \r\n",volPTB3);
	debug_printf("Capacitance PTB3: %5d \r\n",capPTB3);
	PIT->CHANNEL[0].TCTRL = 0x3;

	
	for(int x = 0; x < 10; x++){
		PIT->CHANNEL[0].TCTRL = 0x1;
		debug_printf("P4 - CS Cycle %d \r\n", x);
		LEDBlue_Toggle();
		LEDRed_Toggle();		
		mediumDelay();
		PIT->CHANNEL[0].TCTRL = 0x3;
	}
	l_unlock(&l2);
}



/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	hardware_init();
	LED_Initialize();
	setup();
	l_init(&l);
	l_init(&l2);
	l.highest_process = 3;
	l2.highest_process = 4;
  	/* Create processes */ 
		if (process_rt_periodic_create(p1, RT_STACK, &t_pRT1start, &t_pRT1dead, &t_pRT1period, 1, 1) < 0) { return -1; };
		if (process_rt_periodic_create(p2, RT_STACK, &t_pRT2start, &t_pRT2dead, &t_pRT1period, 2, 2) < 0) { return -1; };
   		if (process_rt_periodic_create(p3, RT_STACK, &t_pRT3start, &t_pRT3dead, &t_pRT1period, 3, 3) < 0) { return -1; };
		if (process_rt_periodic_create(p4, RT_STACK, &t_pRT4start, &t_pRT4dead, &t_pRT1period, 4, 4) < 0) { return -1; };
  	/* Launch concurrent execution */
	process_start();
  	LED_Off();
	/* Hang out in infinite loop (so we can inspect variables if we want) */ 
	while (1);
	return 0;
}
