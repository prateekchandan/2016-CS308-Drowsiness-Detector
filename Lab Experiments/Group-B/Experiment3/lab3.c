#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"

#define PWM_FREQUENCY 55

volatile int num,state;
volatile int switch1;

/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set crystal frequency and enable GPIO Peripherals  

* Example Call: setup();

*/
void setup(){
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);

	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

int detectKeyPress() {
	int sw_val = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
	switch (switch1) {
	case 1:
		if (!sw_val) {
			switch1 = 2;
		}
		return 0;
	case 2:
		if (!sw_val) {
			switch1 = 3;
			return 1;
		} else {
			return 0;
		}
	case 3:
		if (!sw_val) {
			return 0;
		} else {
			switch1 = 1;
			return 0;
		}
	}
	return 0;
}
volatile uint32_t ui32Load;
volatile uint32_t ui32PWMClock;
volatile uint8_t ui8AdjustR;
volatile uint8_t ui8AdjustG;
volatile uint8_t ui8AdjustB;
volatile int t;

int main(void) {

	t = 0;
	ui8AdjustR = 120;
	ui8AdjustG = 0;
	ui8AdjustB = 0;

	int m = 1000000;
	switch1 = 1;

	setup();

	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;

	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT | PWM_OUT_6_BIT | PWM_OUT_7_BIT,true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);

	// Auto Mode
	while (1) {
		int k = t % 360;
		if (k < 120) {
			ui8AdjustR = 120 - k;
			ui8AdjustG = k;
			ui8AdjustB = 0;
		} else if (k < 240) {
			ui8AdjustR = 0;
			ui8AdjustG = 240 - k;
			ui8AdjustB = k - 120;
		} else {
			ui8AdjustR = k - 240;
			ui8AdjustG = 0;
			ui8AdjustB = 360 - k;
		}
		if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) {
			if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00) {
				switch1 = 2;
				break;
			}
			m = m - 100000;
			if (m < 100000)
				m = 100000;
		}
		if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00) {
			if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) {
				switch1 = 2;
				break;
			}
			m = m + 100000;
			if (m > 2000000)
				m = 2000000;
		}

		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5,1 + ui8AdjustR * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6,1 + ui8AdjustB * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7,1 + ui8AdjustG * ui32Load / 1000);
		t++;
		SysCtlDelay(m);
	}

	// Manual Mode
	while (1) {
		num=0;
		volatile int switch1count = 0;
		state = 0;
		SysCtlDelay(SysCtlClockGet() / 50);

		while (1) {
			switch1count = switch1count + detectKeyPress();
			if (switch1 == 1 || switch1 == 2)
				num = 0;
			if (switch1 == 3)
				num = num + 1;
			if (num > 10) {
				state = 1;
				while (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00
						|| GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) {
				}
				break;
			}
			SysCtlDelay(SysCtlClockGet() / 50);
			if (!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00
					&& !GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
				break;
		}

		if(state) {
			int sp1=0;
			int sp2=0;
			while (1) {
				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00
						&& GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) {
					switch1 = 2;
					break;
				}
				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) != 0x00 && sp1==1) {
					ui8AdjustG=ui8AdjustG-10;
					if (ui8AdjustG <= 10) {
						ui8AdjustG = 10;
					}
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7,
							ui8AdjustG * ui32Load / 1000);
				}

				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
					sp1=1;

				if (! GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
					sp1=0;

				if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00) && sp2==1) {
					ui8AdjustG=ui8AdjustG + 10;
					if (ui8AdjustG > 120) {
						ui8AdjustG = 120;
					}
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7,
							ui8AdjustG * ui32Load / 1000);
				}

				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
					sp2=1;

				if (! GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
					sp2=0;


				SysCtlDelay(100000);

			}
		}
		else if(switch1count==1) {
			int sp1=0;
			int sp2=0;
			while (1) {
				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00
						&& GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) {
					switch1 = 2;
					break;
				}
				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) != 0x00 && sp1==1) {
					ui8AdjustR=ui8AdjustR-10;
					if (ui8AdjustR <= 10) {
						ui8AdjustR = 10;
					}
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5,
							ui8AdjustR * ui32Load / 1000);
				}

				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
					sp1=1;

				if (! GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
					sp1=0;

				if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00) && sp2==1) {
					ui8AdjustR=ui8AdjustR + 10;
					if (ui8AdjustR > 120) {
						ui8AdjustR = 120;
					}
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5,
							ui8AdjustR * ui32Load / 1000);
				}

				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
					sp2=1;

				if (! GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
					sp2=0;


				SysCtlDelay(100000);

			}
		}
		else {
			int sp1=0;
			int sp2=0;
			while (1) {
				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00
						&& GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) {
					switch1 = 2;
					break;
				}
				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) != 0x00 && sp1==1) {
					ui8AdjustB=ui8AdjustB-10;
					if (ui8AdjustB <= 10) {
						ui8AdjustB = 10;
					}
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6,
							ui8AdjustB * ui32Load / 1000);
				}

				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
					sp1=1;

				if (! GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
					sp1=0;

				if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00) && sp2==1) {
					ui8AdjustB=ui8AdjustB + 10;
					if (ui8AdjustB > 120) {
						ui8AdjustB = 120;
					}
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6,
							ui8AdjustB * ui32Load / 1000);
				}

				if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
					sp2=1;

				if (! GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
					sp2=0;


				SysCtlDelay(100000);
			}
		}
	}
}