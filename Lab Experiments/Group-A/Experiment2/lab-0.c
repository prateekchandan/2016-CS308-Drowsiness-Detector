#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set crystal frequency and enable GPIO Peripherals  

* Example Call: setup();

*/
void setup(void)
{
	/* Configure the system clock to run at 40MHz */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

/*

* Function Name: led_pin_config()

* Input: none

* Output: none

* Description: Set PORTF Pin 1, Pin 2, Pin 3 as output. On this pin Red, Blue and Green LEDs are connected.

* Example Call: led_pin_config();

*/

void led_pin_config(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}


/*

* Function Name: switchPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 0 and Pin 4 as input. Note that Pin 0 is locked.

* Example Call: switchPinConfig();

*/
void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0|GPIO_PIN_4;

	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

uint32_t ui32Period;
void timerConfig(){

	/* enable the clock to that peripheral */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC); //TIMER0_BASE is the start of the timer registers for Timer0

	/* SysCtlClockGet() returns the frequency of system timer in Hertz */
	ui32Period = (SysCtlClockGet()/100)/2;

	/* calculated period is then loaded into the Timer’s Interval Load register */
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	/* IntEnable enables the specific vector associated with Timer0A */
	IntEnable(INT_TIMER0A);

	/* TimerIntEnable, enables a specific event within the timer to generate an interrupt */
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	/* IntMasterEnable() is the master interrupt enable API for all interrupts */
	IntMasterEnable();

	/* This will start the timer and interrupts will begin triggering on the timeouts. */
	TimerEnable(TIMER0_BASE, TIMER_A);
}

int main(void)
{

	setup();
	led_pin_config();
	switchPinConfig();
	timerConfig();

	while(1)
	{
	}
}

 /*
  * State variable is used to maintain state
  * state 0 : Idle
  * state 1 : Press
  * state 2 : Release
  */
 uint8_t sw1State = 0;
 uint8_t sw2State = 0;

/* Function to detect the keypress and perform necessary action */
unsigned char detectKeyPress(bool switch1){
	unsigned char ret = 0;
	bool pressed;
	uint8_t state, finalState;
	if(switch1){
		pressed = (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00);
		state = sw1State;
	} else {
		pressed = (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00);
		state = sw2State;
	}

	switch(state){
	case 0:
		if(pressed) finalState = 1;
		break;
	case 1:
		if(pressed) {ret = 1; finalState=2;}
		else {finalState = 0;}
		break;
	case 2:
		if(pressed) {ret = 1; finalState = 2;}
		else {finalState = 0;}
	}

	if(switch1){
		sw1State = finalState;
	} else {
		sw2State = finalState;
	}
	return ret;
}



/* LED Color */
uint8_t ui8LED = 0;
/* Press States */
bool sw1Pressed = 0,sw2Pressed = 0;
int sw2Status = 0;
/* Interrupt Service Routine */
void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	unsigned char sw1Press = detectKeyPress(true);
	if(sw1Press == 1){
		// Pressed
		if(sw1Pressed == 0){
			if(ui8LED == 2)
				ui8LED = 8;
			else if(ui8LED ==8)
				ui8LED = 4;
			else
				ui8LED = 2;

			sw1Pressed = 1;
		}
		// Turn on the LED
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8LED);
	}else{
		sw1Pressed = 0;
		// Turn on the LED
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
	}

	unsigned char sw2Press = detectKeyPress(false);
	if (sw2Press == 1){
		if(sw2Pressed == 0){
			sw2Status += 1;
			sw2Pressed = 1;
		}
	} else {
		sw2Pressed = 0;
	}
}

