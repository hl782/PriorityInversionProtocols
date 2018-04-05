#ifndef __UTILS_H__
#define __UTILS_H__

void LED_Initialize(void);
void LEDRed_Toggle (void);
void LEDBlue_Toggle (void);
void LEDGreen_Toggle (void);
void LEDRed_On (void);
void LEDGreen_On (void);
void LEDBlue_On (void);
void LED_Off (void);

unsigned short ADC_read16b(int channel);
short calculateCapacitance(short cap_s, short volt_ptb2, short volt_ptb9);
short calculateVolt_ptb2(short cap_s, short cap_t, short volt_ptb9);

void delay (void);

#endif

