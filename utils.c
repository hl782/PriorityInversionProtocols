#include <fsl_device_registers.h>
#include "utils.h"

/*----------------------------------------------------------------------------
  Function that initializes LEDs
 *----------------------------------------------------------------------------*/
void LED_Initialize(void) {

  SIM->SCGC5    |= (1 <<  10) | (1 <<  13);  /* Enable Clock to Port B & E */ 
  PORTB->PCR[22] = (1 <<  8) ;               /* Pin PTB22 is GPIO */
  PORTB->PCR[21] = (1 <<  8);                /* Pin PTB21 is GPIO */
  PORTE->PCR[26] = (1 <<  8);                /* Pin PTE26  is GPIO */
  
  PTB->PDOR = (1 << 21 | 1 << 22 );          /* switch Red/Green LED off  */
  PTB->PDDR = (1 << 21 | 1 << 22 );          /* enable PTB18/19 as Output */

  PTE->PDOR = 1 << 26;            /* switch Blue LED off  */
  PTE->PDDR = 1 << 26;            /* enable PTE26 as Output */
}

/*----------------------------------------------------------------------------
  Function that toggles the red LED
 *----------------------------------------------------------------------------*/

void LEDRed_Toggle (void) {
	PTB->PTOR = 1 << 22; 	   /* Red LED Toggle */
}

/*----------------------------------------------------------------------------
  Function that toggles the blue LED
 *----------------------------------------------------------------------------*/
void LEDBlue_Toggle (void) {
	PTB->PTOR = 1 << 21; 	   /* Blue LED Toggle */
}

/*----------------------------------------------------------------------------
  Function that toggles the green LED
 *----------------------------------------------------------------------------*/
void LEDGreen_Toggle (void) {
	PTE->PTOR = 1 << 26; 	   /* Green LED Toggle */
}

/*----------------------------------------------------------------------------
  Function that turns on Red LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDRed_On (void) {
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PCOR   = 1 << 22;   /* Red LED On*/
  PTB->PSOR   = 1 << 21;   /* Blue LED Off*/
  PTE->PSOR   = 1 << 26;   /* Green LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that turns on Green LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDGreen_On (void) {
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PSOR   = 1 << 21;   /* Blue LED Off*/
  PTE->PCOR   = 1 << 26;   /* Green LED On*/
  PTB->PSOR   = 1 << 22;   /* Red LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that turns on Blue LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDBlue_On (void) {
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTE->PSOR   = 1 << 26;   /* Green LED Off*/
  PTB->PSOR   = 1 << 22;   /* Red LED Off*/
  PTB->PCOR   = 1 << 21;   /* Blue LED On*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that turns all LEDs off
 *----------------------------------------------------------------------------*/
void LED_Off (void) {	
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PSOR   = 1 << 22;   /* Green LED Off*/
  PTB->PSOR   = 1 << 21;   /* Red LED Off*/
  PTE->PSOR   = 1 << 26;   /* Blue LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that returns the analog reading
 *----------------------------------------------------------------------------*/
unsigned short ADC_read16b(int channel){
	ADC0_SC1A = channel & ADC_SC1_ADCH_MASK; 			//Write to SC1A to start conversion
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); 		//Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK)); 	//Wait until conversion complete
	return ADC0_RA;
}

/*----------------------------------------------------------------------------
  Function that calculates Voltage measured by ptb2
 *----------------------------------------------------------------------------*/
short calculateVolt_ptb2(short cap_s, short cap_t, short volt_ptb9){
	return (volt_ptb9*cap_t)/(cap_s + cap_t);
}

/*----------------------------------------------------------------------------
  Function that calculates capcitance of target capacitor
 *----------------------------------------------------------------------------*/
short calculateCapacitance(short cap_s, short volt_ptb2, short volt_ptb9){
	return (cap_s)*(volt_ptb2/(volt_ptb9 - volt_ptb2));
}

void delay(void){
	int j;
	for(j=0; j<100000; j++);
}
