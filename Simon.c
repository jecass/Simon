/*
   ELEC327 Midterm Project - Simon Game
   Shuqing Chen

*/
#include <msp430.h> 
#include <stdint.h>
#include "rand4.h"

#define M 5 						 // Sequence length

int seed = 1;
int animationFlag = 1;
int countLED = 0;
int subcountLED = 0;
int intensity = 0;
int timeoutCount = 0;
int gameTime = 0;
int pregameTime = 10000;

int k = 0;
int i;
int buzzerCount = 0;
uint8_t b_led_frame = 0x00;	 		// LED sub-frames
uint8_t g_led_frame = 0x00;
uint8_t r_led_frame = 0x00;
uint8_t doubleLED = 0x00;

int resetFlag = 0;
int winFlag = 0;
int playMode = 0;
int *sequence;
int seqLength = M;
int indexSequence[M];
int NseqLength = 0;
int Index = 0;
int Turn = 1;
int first_press = 0;
int displayFlag = 0;
int resetCount = 0;
int indexCheck = 0;
int rightPress = 0;
int doublePress = 0;
int Nindex = 0;

int portEnable1 = 0;
int portEnable2 = 0;
int portEnable3 = 0;
int portEnable4 = 0;

// Notes for a major C chord
int periods[] = {1000000/261.63,
	1000000/293.66,
	1000000/329.63,
	1000000/349.23,
	1000000/392,
	1000000/440,
	1000000/493.88,
	1000000/523.25};

void SPI_config();
void SPI_transmit(uint8_t whichLED);
void SPI_transmit2();
void SPI_transmit3();
void SPI_transmit4();

void main(void) {
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT

	if (CALBC1_1MHZ == 0xFF || CALDCO_1MHZ == 0xff)
		while(1); // Erased calibration data? Trap!

	BCSCTL1 = CALBC1_1MHZ; // Set the DCO to 1 MHz
	DCOCTL = CALDCO_1MHZ;  // And load calibration data

	/* Output port pins configuration */
	P1SEL |=  BIT2 + BIT4;		// P1.4 as USCI_A0 SPI clock, P1.2 as USCI_A0 SPI MOSI pin
	P1SEL2 |=  BIT2 + BIT4;
	P2DIR |= BIT1 + BIT5; 		// P2.1, P2.5 output pins for buzzer
	P2OUT &= ~BIT5;				// P2.5 - ground pin for buzzer
	P2SEL |= BIT1; 		  		// P2.1 TA1.1 - buzzer frequency

	/* SPI configuration */
	SPI_config();

	/* TimerA1 generates PWM for buzzer */
	TA1CCTL1 = OUTMOD_7;  						// Output is high until the counter reaches the value of CCR1
	TA1CTL = TASSEL_2 + MC_1; 					// SMCLK, upmode
	TA1CCR0 = periods[0];						// Initialized to lowest frequency
	TA1CCR1 = 0;								// Duty cycle 5%

	/* Watchdog timer setting */
	WDTCTL = WDT_MDLY_8;		// Enter WDT ISR every 8ms
	IE1 |= WDTIE;				// WDT interrupts enabled

	/* Port interrupt setting */
	P2DIR &= ~(BIT0 + BIT2 + BIT3 + BIT4);		// P2.0 button 1 input, P2.2 button 2 input, P12.3 button 3 input, P2.4 button 4 input
	P2IE |= BIT0 + BIT2 + BIT3 + BIT4;          // P2.0, P2.2, P2.4, P2.4 interrupt enabled
	P2IES |= BIT0 + BIT2 + BIT3 + BIT4;			// Initially detect falling edges
	P2IFG &= ~(BIT0 + BIT2 + BIT3 + BIT4);      // P2.0, P2.2, P2.3, P2.4 IFG cleared
	P2REN |= BIT0 + BIT2 + BIT3 + BIT4;			// Pull up until button press

	__bis_SR_register(CPUOFF + GIE);
}


// Port2 ISR - button press detection
#pragma vector=PORT2_VECTOR
__interrupt void port_2(void)
{
	/* In start mode, select play mode */
	if (animationFlag == 1 && playMode == 0){
		// Detect button 1 press at rising edge
		if ((P2IFG & BIT0) == BIT0){
			P2IE &= ~BIT0;						// P2.0 interrupt disabled to avoid bouncing effect
			if (resetFlag != 0){
				resetFlag --;
			}
			else{
				if ((P2IES & BIT0) == 0x00){
					// Generate the sequence
					sequence = (int *)calloc(seqLength, sizeof(int));
					for (i = 0; i <= seqLength - 1; i++){
						sequence[i] = rand4(seed) + 1;		// Sequence of integer 1, 2, 3, or 4
						seed ++;
					}
					animationFlag = 0;				// Turn off start mode animation
					playMode = 1;					// Select play mode 1
					SPI_transmit(0x00);
					TA1CCR1 = 0;
					countLED = -50;
					displayFlag = 1;
				}
				P2IES ^= BIT0;						// Toggle the detection edge
				P2IFG &= ~BIT0;						// Clear port flag
			}
		}
		// Detect button 2 press at rising edge
		if ((P2IFG & BIT2) == BIT2){
			P2IE &= ~BIT2;						// P2.2 interrupt disabled to avoid bouncing effect
			if (resetFlag != 0){
				resetFlag --;
			}
			else{
				if ((P2IES & BIT2) == 0x00){
					// Re-generate the sequence
					sequence = (int *)calloc(seqLength, sizeof(int));
					for (i = 0; i <= seqLength - 1; i++){
						sequence[i] = rand4(seed) + 1;		// Sequence of integer 1, 2, 3, or 4
						seed ++;
					}
					animationFlag = 0;				// Turn off start mode animation
					playMode = 2;					// Select play mode 2
					SPI_transmit(0x00);
					TA1CCR1 = 0;
					countLED = -50;
					displayFlag = 1;
				}
				P2IES ^= BIT2;						// Toggle the detection edge
				P2IFG &= ~BIT2;						// Clear port flag
			}
		}
		// Detect button 3 press at rising edge
		if ((P2IFG & BIT3) == BIT3){
			P2IE &= ~BIT3;						// P2.3 interrupt disabled to avoid bouncing effect
			if ((P2IES & BIT3) == 0x00){
				// Generate an index sequence whose elements indicate whether an element in the sequence is single- or double-press
				for (i = 0; i <= seqLength - 1; i++){
					indexSequence[i] = (rand4(seed) % 2) + 1;		// Sequence of integer 1 or 2
					seed ++;
					NseqLength += indexSequence[i];
				}
				// Generate the sequence that contains single- and double-presses
				sequence = (int *)calloc(NseqLength, sizeof(int));
				k = 0;
				for (i = 0; i <= seqLength - 1; i++){
					if (indexSequence[i] == 1){					// Generate single-press
						sequence[k] = rand4(seed) + 1;
						seed ++;
						k ++;
					}
					else if (indexSequence[i] == 2){			// Generate double-press
						sequence[k] = rand4(seed) + 1;
						seed ++;
						sequence[k+1] = rand4(seed) + 1;
						seed ++;
						while (sequence[k+1] == sequence[k]){	// The two elements in one double-press should be different
							sequence[k+1] = rand4(seed) + 1;
							seed ++;
						}
						k += 2;
					}
				}
				k = 0;							// Reset the index for later usage
				animationFlag = 0;				// Turn off start mode animation
				playMode = 3;					// Select play mode 3
				SPI_transmit(0x00);
				TA1CCR1 = 0;
				countLED = -50;
				displayFlag = 1;
			}
			P2IES ^= BIT3;						// Toggle the detection edge
			P2IFG &= ~BIT3;						// Clear port flag
		}
		// Detect button 4 press at rising edge
		else if ((P2IFG & BIT4) == BIT4){
			P2IE &= ~BIT4;						// P2.4 interrupt disabled to avoid bouncing effect
			if ((P2IES & BIT4) == 0x00){
				// Generate an index sequence whose elements indicate whether an element in the sequence is single- or double-press
				for (i = 0; i <= seqLength - 1; i++){
					indexSequence[i] = (rand4(seed) % 2) + 1;		// Sequence of integer 1 or 2
					seed ++;
					NseqLength += indexSequence[i];
				}
				// Generate the sequence that contains single- and double-presses
				sequence = (int *)calloc(NseqLength, sizeof(int));
				k = 0;
				for (i = 0; i <= seqLength - 1; i++){
					if (indexSequence[i] == 1){					// Generate single-press
						sequence[k] = rand4(seed) + 1;
						seed ++;
						k ++;
					}
					else if (indexSequence[i] == 2){			// Generate double-press
						sequence[k] = rand4(seed) + 1;
						seed ++;
						sequence[k+1] = rand4(seed) + 1;
						seed ++;
						while (sequence[k+1] == sequence[k]){	// The two elements in one double-press should be different
							sequence[k+1] = rand4(seed) + 1;
							seed ++;
						}
						k += 2;
					}
				}
				k = 0;							// Reset the index for later usage
				animationFlag = 0;				// Turn off start mode animation
				playMode = 4;					// Select play mode 3
				SPI_transmit(0x00);
				TA1CCR1 = 0;
				countLED = -50;
				displayFlag = 1;
			}
			P2IES ^= BIT4;						// Toggle the detection edge
			P2IFG &= ~BIT4;						// Clear port flag
		}
	}

	/* In play mode 1 & 2, sequence with only single-presses */
	else if (displayFlag == 0 && animationFlag == 0 && (playMode == 1 || playMode == 2)){
		// Detect button 1 press
		if ((P2IFG & BIT0) == BIT0){
			P2IE &= ~BIT0;						// P2.0 interrupt disabled to avoid bouncing effect
			SPI_transmit(BIT0);					// Turn on LED 1 at falling edge
			TA1CCR0 = periods[0];				// Turn on buzzer at falling edge
			TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
			// Detect rising edges
			if ((P2IES & BIT0) == 0x00){
				if (sequence[Index] == 1){		// Check with the sequence if the pressed button is right
					Index ++;					// Next index
					timeoutCount = 0;			// Re-start timeout counting
					SPI_transmit(0x00);			// Turn off LED 1 when releasing the button
					TA1CCR1 = 0;				// Turn off the buzzer when releasing the button
					if (Index == 1)
						first_press = 1;		// Record the first press
					if (Index == Turn){
						Index = 0;
						Turn ++;
						displayFlag = 1;		// Display the next turn of the sequence
						countLED = -63;
					}
					if (Turn == seqLength + 1){
						winFlag = 1;			// Game won
						displayFlag = 0;
					}
				}
				else{							// Wrong button -> lose mode
					countLED = 0;
					animationFlag = 2;			// Play lose mode animation
					buzzerCount = 0;
					displayFlag = 0;
					playMode = 0;
				}
			}
			P2IES ^= BIT0;				// Toggle the detection edge
			P2IFG &= ~BIT0;				// Clear port flag
		}
		// Detect button 2 press
		if ((P2IFG & BIT2) == BIT2){
			P2IE &= ~BIT2;						// P2.2 interrupt disabled to avoid bouncing effect
			SPI_transmit(BIT1);					// Turn on LED 2 at falling edge
			TA1CCR0 = periods[1];				// Turn on buzzer at falling edge
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
			// Detect rising edges
			if ((P2IES & BIT2) ==0x00){
				if (sequence[Index] == 2){	    // Check with the sequence if the pressed button is right
					Index ++;					// Next index
					timeoutCount = 0;			// Re-start timeout counting
					SPI_transmit(0x00);			// Turn off LED 2 when releasing the button
					TA1CCR1 = 0;				// Turn off the buzzer when releasing the button
					if (Index == 1)
						first_press = 1;		// Record the first press
					if (Index == Turn){
						Index = 0;
						Turn ++;
						displayFlag = 1;		// Display the next turn of the sequence
						countLED = -63;
					}
					if (Turn == seqLength + 1){
						winFlag = 1;			// Game won
						displayFlag = 0;
					}
				}
				else{							// Wrong button -> lose mode
					countLED = 0;
					animationFlag = 2;			// Play lose mode animation
					buzzerCount = 0;
					displayFlag = 0;
					playMode = 0;
				}
			}
			P2IES ^= BIT2;				// Toggle the detection edge
			P2IFG &= ~BIT2;				// Clear port flag
		}
		// Detect button 3 press
		if ((P2IFG & BIT3) == BIT3){
			P2IE &= ~BIT3;						// P2.3 interrupt disabled to avoid bouncing effect
			SPI_transmit(BIT2);					// Turn on LED 3 at falling edge
			TA1CCR0 = periods[2];				// Turn on buzzer at falling edge
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
			// Detect rising edges
			if ((P2IES & BIT3) == 0x00){
				if (sequence[Index] == 3){		// Check with the sequence if the pressed button is right
					Index ++;					// Next index
					timeoutCount = 0;			// Re-start timeout counting
					SPI_transmit(0x00);			// Turn off LED 3 when releasing the button
					TA1CCR1 = 0;				// Turn off the buzzer when releasing the button
					if (Index == 1)
						first_press = 1;		// Record the first press
					if (Index == Turn){
						Index = 0;
						Turn ++;
						displayFlag = 1;		// Display the next turn of the sequence
						countLED = -63;
					}
					if (Turn == seqLength + 1){
						winFlag = 1;			// Game won
						displayFlag = 0;
					}
				}
				else{							// Wrong button -> lose mode
					countLED = 0;
					animationFlag = 2;			// Play lose mode animation
					buzzerCount = 0;
					displayFlag = 0;
					playMode = 0;
				}
			}
			P2IES ^= BIT3;				// Toggle the detection edge
			P2IFG &= ~BIT3;				// Clear port flag
		}
		// Detect button 4 press
		if ((P2IFG & BIT4) == BIT4){
			P2IE &= ~BIT4;						// P2.4 interrupt disabled to avoid bouncing effect
			SPI_transmit(BIT3);					// Turn on LED 4 at falling edge
			TA1CCR0 = periods[3];				// Turn on buzzer 1 at falling edge
			TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
			// Detect rising edges
			if ((P2IES & BIT4) == 0x00){
				if (sequence[Index] == 4){		// Check with the sequence if the pressed button is right
					Index ++;					// Next index
					timeoutCount = 0;			// Re-start timeout counting
					SPI_transmit(0x00);			// Turn off LED 4 when releasing the button
					TA1CCR1 = 0;				// Turn off the buzzer when releasing the button
					if (Index == 1)
						first_press = 1;		// Record the first press
					if (Index == Turn){
						Index = 0;
						Turn ++;
						displayFlag = 1;		// Display the next turn of the sequence
						countLED = -63;
					}
					if (Turn == seqLength + 1){
						winFlag = 1;			// Game won
						displayFlag = 0;
					}
				}
				else{							// Wrong button -> lose mode
					countLED = 0;
					animationFlag = 2;			// Play lose mode animation
					buzzerCount = 0;
					displayFlag = 0;
					playMode = 0;
				}
			}
			P2IES ^= BIT4;				// Toggle the detection edge
			P2IFG &= ~BIT4;				// Clear port flag
		}
	}

	/* In play mode 3 & 4, sequence with single- and double-presses */
	else if (displayFlag == 0 && animationFlag == 0 && (playMode == 3 || playMode == 4)){
		// Detect button 1 press
		if ((P2IFG & BIT0) == BIT0){
			P2IE &= ~BIT0;							// P2.0 interrupt disabled to avoid bouncing effect
			// Detect falling edges
			if ((P2IES & BIT0) == BIT0){
				doubleLED |= BIT0;
				SPI_transmit(doubleLED);					// Turn on LED 1 at falling edge
				if (indexSequence[Index] == 2){
					doublePress = 1;
					if (indexCheck == 0){			// Detect the first element in a double-press
						if (sequence[Nindex] == 1 || sequence[Nindex+1] == 1){
							indexCheck = 1;
							rightPress = 0;
							timeoutCount = 0;
						}
						else{
							countLED = 0;
							animationFlag = 2;		// Play lose mode animation
							displayFlag = 0;
							playMode = 0;
						}
					}
					else{							// Detect the second element in a double-press
						if (sequence[Nindex] == 1 || sequence[Nindex+1] == 1){
							rightPress = 1;			// Correct double-press
						}
					}
				}
				else{
					doublePress = 0;
				}
			}
			// Detect rising edges
			if ((P2IES & BIT0) == 0x00){
				doubleLED &= ~BIT0;
				SPI_transmit(doubleLED);			// Turn off LED 1 when releasing the button
				if (doublePress == 0){				// Single-press
					if (sequence[Nindex] == 1){		// Check with the sequence if the pressed button is right
						Nindex ++;
						Index ++;					// Next index
						timeoutCount = 0;			// Re-start timeout counting
						if (Index == 1)
							first_press = 1;		// Record the first press
						if (Index == Turn){
							Index = 0;
							Nindex = 0;
							Turn ++;
							displayFlag = 1;		// Display the next turn of the sequence
							countLED = -63;
						}
						if (Turn == seqLength + 1){
							winFlag = 1;			// Game won
							displayFlag = 0;
						}
					}
					else{							// Wrong button -> lose mode
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
				else{								// Double-press
					if (rightPress == 1){
						if (indexCheck == 1){		// First release of the double-press
							indexCheck = 0;
						}
						else{						// Second release of the double-press
							timeoutCount = 0;
							Nindex += 2;
							Index ++;
							if (Index == 1)
								first_press = 1;		// Record the first press
							if (Index == Turn){
								Index = 0;
								Nindex = 0;
								Turn ++;
								displayFlag = 1;		// Display the next turn of the sequence
								countLED = -63;
							}
							if (Turn == seqLength + 1){
								winFlag = 1;			// Game won
								displayFlag = 0;
							}
						}
					}
					else{
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
			}
			P2IES ^= BIT0;				// Toggle the detection edge
			P2IFG &= ~BIT0;				// Clear port flag
		}

		// Detect button 2 press
		if ((P2IFG & BIT2) == BIT2){
			P2IE &= ~BIT2;						// P2.2 interrupt disabled to avoid bouncing effect
			// Detect falling edges
			if ((P2IES & BIT2) == BIT2){
				doubleLED |= BIT1;
				SPI_transmit(doubleLED);					// Turn on LED 2 at falling edge
				if (indexSequence[Index] == 2){
					doublePress = 1;
					if (indexCheck == 0){			// Detect the first element in a double-press
						if (sequence[Nindex] == 2 || sequence[Nindex+1] == 2){
							indexCheck = 1;
							rightPress = 0;
							timeoutCount = 0;
						}
						else{
							countLED = 0;
							animationFlag = 2;		// Play lose mode animation
							displayFlag = 0;
							playMode = 0;
						}
					}
					else{							// Detect the second element in a double-press
						if (sequence[Nindex] == 2 || sequence[Nindex+1] == 2){
							rightPress = 1;			// Correct double-press
						}
					}
				}
				else{
					doublePress = 0;
				}
			}
			// Detect rising edges
			if ((P2IES & BIT2) == 0x00){
				doubleLED &= ~BIT1;
				SPI_transmit(doubleLED);					// Turn off LED 2 when releasing the button
				if (doublePress == 0){				// Single-press
					if (sequence[Nindex] == 2){		// Check with the sequence if the pressed button is right
						Nindex ++;
						Index ++;					// Next index
						timeoutCount = 0;			// Re-start timeout counting
						if (Index == 1)
							first_press = 1;		// Record the first press
						if (Index == Turn){
							Index = 0;
							Nindex = 0;
							Turn ++;
							displayFlag = 1;		// Display the next turn of the sequence
							countLED = -63;
						}
						if (Turn == seqLength + 1){
							winFlag = 1;			// Game won
							displayFlag = 0;
						}
					}
					else{							// Wrong button -> lose mode
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
				else{								// Double-press
					if (rightPress == 1){
						if (indexCheck == 1){		// First release of the double-press
							indexCheck = 0;
						}
						else{						// Second release of the double-press
							timeoutCount = 0;
							Nindex += 2;
							Index ++;
							if (Index == 1)
								first_press = 1;		// Record the first press
							if (Index == Turn){
								Index = 0;
								Nindex = 0;
								Turn ++;
								displayFlag = 1;		// Display the next turn of the sequence
								countLED = -63;
							}
							if (Turn == seqLength + 1){
								winFlag = 1;			// Game won
								displayFlag = 0;
							}
						}
					}
					else{
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
			}
			P2IES ^= BIT2;				// Toggle the detection edge
			P2IFG &= ~BIT2;				// Clear port flag
		}


		// Detect button 3 press
		if ((P2IFG & BIT3) == BIT3){
			P2IE &= ~BIT3;						// P2.3 interrupt disabled to avoid bouncing effect
			// Detect falling edges
			if ((P2IES & BIT3) == BIT3){
				doubleLED |= BIT2;
				SPI_transmit(doubleLED);					// Turn on LED 3 at falling edge
				if (indexSequence[Index] == 2){
					doublePress = 1;
					if (indexCheck == 0){			// Detect the first element in a double-press
						if (sequence[Nindex] == 3 || sequence[Nindex+1] == 3){
							indexCheck = 1;
							rightPress = 0;
							timeoutCount = 0;
						}
						else{
							countLED = 0;
							animationFlag = 2;		// Play lose mode animation
							displayFlag = 0;
							playMode = 0;
						}
					}
					else{							// Detect the second element in a double-press
						if (sequence[Nindex] == 3 || sequence[Nindex+1] == 3){
							rightPress = 1;			// Correct double-press
						}
					}
				}
				else{
					doublePress = 0;
				}
			}
			// Detect rising edges
			if ((P2IES & BIT3) == 0x00){
				doubleLED &= ~BIT2;
				SPI_transmit(doubleLED);					// Turn off LED 3 when releasing the button
				if (doublePress == 0){				// Single-press
					if (sequence[Nindex] == 3){		// Check with the sequence if the pressed button is right
						Nindex ++;
						Index ++;					// Next index
						timeoutCount = 0;			// Re-start timeout counting
						if (Index == 1)
							first_press = 1;		// Record the first press
						if (Index == Turn){
							Index = 0;
							Nindex = 0;
							Turn ++;
							displayFlag = 1;		// Display the next turn of the sequence
							countLED = -63;
						}
						if (Turn == seqLength + 1){
							winFlag = 1;			// Game won
							displayFlag = 0;
						}
					}
					else{							// Wrong button -> lose mode
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
				else{								// Double-press
					if (rightPress == 1){
						if (indexCheck == 1){		// First release of the double-press
							indexCheck = 0;
						}
						else{						// Second release of the double-press
							timeoutCount = 0;
							Nindex += 2;
							Index ++;
							if (Index == 1)
								first_press = 1;		// Record the first press
							if (Index == Turn){
								Index = 0;
								Nindex = 0;
								Turn ++;
								displayFlag = 1;		// Display the next turn of the sequence
								countLED = -63;
							}
							if (Turn == seqLength + 1){
								winFlag = 1;			// Game won
								displayFlag = 0;
							}
						}
					}
					else{
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
			}
			P2IES ^= BIT3;				// Toggle the detection edge
			P2IFG &= ~BIT3;				// Clear port flag
		}

		// Detect button 4 press
		if ((P2IFG & BIT4) == BIT4){
			P2IE &= ~BIT4;						// P2.4 interrupt disabled to avoid bouncing effect
			// Detect falling edges
			if ((P2IES & BIT4) == BIT4){
				doubleLED |= BIT3;
				SPI_transmit(doubleLED);					// Turn on LED 4 at falling edge
				if (indexSequence[Index] == 2){
					doublePress = 1;
					if (indexCheck == 0){			// Detect the first element in a double-press
						if (sequence[Nindex] == 4 || sequence[Nindex+1] == 4){
							indexCheck = 1;
							rightPress = 0;
							timeoutCount = 0;
						}
						else{
							countLED = 0;
							animationFlag = 2;		// Play lose mode animation
							displayFlag = 0;
							playMode = 0;
						}
					}
					else{							// Detect the second element in a double-press
						if (sequence[Nindex] == 4 || sequence[Nindex+1] == 4){
							rightPress = 1;			// Correct double-press
						}
					}
				}
				else{
					doublePress = 0;
				}
			}
			// Detect rising edges
			if ((P2IES & BIT4) == 0x00){
				doubleLED &= ~BIT3;
				SPI_transmit(doubleLED);					// Turn off LED 4 when releasing the button
				if (doublePress == 0){				// Single-press
					if (sequence[Nindex] == 4){		// Check with the sequence if the pressed button is right
						Nindex ++;
						Index ++;					// Next index
						timeoutCount = 0;			// Re-start timeout counting
						if (Index == 1)
							first_press = 1;		// Record the first press
						if (Index == Turn){
							Index = 0;
							Nindex = 0;
							Turn ++;
							displayFlag = 1;		// Display the next turn of the sequence
							countLED = -63;
						}
						if (Turn == seqLength + 1){
							winFlag = 1;			// Game won
							displayFlag = 0;
						}
					}
					else{							// Wrong button -> lose mode
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
				else{								// Double-press
					if (rightPress == 1){
						if (indexCheck == 1){		// First release of the double-press
							indexCheck = 0;
						}
						else{						// Second release of the double-press
							timeoutCount = 0;
							Nindex += 2;
							Index ++;
							if (Index == 1)
								first_press = 1;		// Record the first press
							if (Index == Turn){
								Index = 0;
								Nindex = 0;
								Turn ++;
								displayFlag = 1;		// Display the next turn of the sequence
								countLED = -63;
							}
							if (Turn == seqLength + 1){
								winFlag = 1;			// Game won
								displayFlag = 0;
							}
						}
					}
					else{
						countLED = 0;
						animationFlag = 2;			// Play lose mode animation
						displayFlag = 0;
						playMode = 0;
					}
				}
			}
			P2IES ^= BIT4;				// Toggle the detection edge
			P2IFG &= ~BIT4;				// Clear port flag
		}
	}
}


// WDT ISR - debouncing
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
	/* Re-enable port interrupt after at least 24ms to debounce */
		if ((P2IE & BIT0) == 0x00){
			portEnable1 ++;
			if (portEnable1 == 4){
				P2IE |= BIT0;          	// P2.0 interrupt re-enabled
				P2IFG &= ~BIT0;				// Clear port flag
				portEnable1 = 0;
			}
		}
		if ((P2IE & BIT2) == 0x00){
			portEnable2 ++;
			if (portEnable2 == 4){
				P2IE |= BIT2;          	// P2.2 interrupt re-enabled
				P2IFG &= ~BIT2;				// Clear port flag
				portEnable2 = 0;
			}
		}
		if ((P2IE & BIT3) == 0x00){
			portEnable3 ++;
			if (portEnable3 == 4){
				P2IE |= BIT3;          	// P2.3 interrupt re-enabled
				P2IFG &= ~BIT3;				// Clear port flag
				portEnable3 = 0;
			}
		}
		if ((P2IE & BIT4) == 0x00){
			portEnable4 ++;
			if (portEnable4 == 4){
				P2IE |= BIT4;          	// P2.4 interrupt re-enabled
				P2IFG &= ~BIT4;				// Clear port flag
				portEnable4 = 0;
			}
		}

	/* Display the sequence */
	if (displayFlag == 1 && animationFlag == 0 && playMode != 0){
		// In play mode 1
		if (playMode == 1){
			if (countLED == 0){
				if (sequence[Index] == 1){
					SPI_transmit(BIT0);
					TA1CCR0 = periods[0];
					TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
				}
				else if (sequence[Index] == 2){
					SPI_transmit(BIT1);
					TA1CCR0 = periods[1];
					TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
				}
				else if (sequence[Index] == 3){
					SPI_transmit(BIT2);
					TA1CCR0 = periods[2];
					TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
				}
				else if (sequence[Index] == 4){
					SPI_transmit(BIT3);
					TA1CCR0 = periods[3];
					TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
				}
			}
			countLED ++;
			if (countLED == 100){				// Display each element of the sequence for 0.8 sec
				Index ++;
				SPI_transmit(0x00);
				TA1CCR1 = 0;
			}
			if (Index == Turn){
				countLED = 0;
				Index = 0;
				SPI_transmit(0x00);				// Turn off the LEDs
				displayFlag = 0;				// Finish sequence display
				timeoutCount = 0;
			}
			if (countLED == 200)
				countLED = 0;
		}
		// In play mode 2
		if (playMode == 2){
			if (countLED == 0){
				if (sequence[Index] == 1){
					SPI_transmit(BIT0);
					TA1CCR0 = periods[0];
					TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
				}
				else if (sequence[Index] == 2){
					SPI_transmit(BIT1);
					TA1CCR0 = periods[1];
					TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
				}
				else if (sequence[Index] == 3){
					SPI_transmit(BIT2);
					TA1CCR0 = periods[2];
					TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
				}
				else if (sequence[Index] == 4){
					SPI_transmit(BIT3);
					TA1CCR0 = periods[3];
					TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
				}
			}
			countLED ++;
			if (countLED == 63){				// Display each element of the sequence for 0.5 sec
				Index ++;
				SPI_transmit(0x00);
				TA1CCR1 = 0;
			}
			if (Index == Turn){
				countLED = 0;
				Index = 0;
				SPI_transmit(0x00);				// Turn off the LEDs
				displayFlag = 0;				// Finish sequence display
				timeoutCount = 0;
			}
			if (countLED == 125)
				countLED = 0;
		}
		// In play mode 3
		if (playMode == 3){
			if (countLED == 0){
				if (indexSequence[Index] == 1){
					if (sequence[Nindex] == 1)
						SPI_transmit(BIT0);
					else if (sequence[Nindex] == 2)
						SPI_transmit(BIT1);
					else if (sequence[Nindex] == 3)
						SPI_transmit(BIT2);
					else if (sequence[Nindex] == 4)
						SPI_transmit(BIT3);
				}
				if (indexSequence[Index] == 2){
					if ((sequence[Nindex] == 1 && sequence[Nindex+1] == 2) || (sequence[Nindex] == 2 && sequence[Nindex+1] == 1))
						SPI_transmit(BIT0 + BIT1);		// LED 1 & 2
					else if ((sequence[Nindex] == 1 && sequence[Nindex+1] == 3) || (sequence[Nindex] == 3 && sequence[Nindex+1] == 1))
						SPI_transmit(BIT0 + BIT2);	 	// LED 1 & 3
					else if ((sequence[Nindex] == 1 && sequence[Nindex+1] == 4) || (sequence[Nindex] == 4 && sequence[Nindex+1] == 1))
						SPI_transmit(BIT0 + BIT3);		// LED 1 & 4
					else if ((sequence[Nindex] == 2 && sequence[Nindex+1] == 3) || (sequence[Nindex] == 3 && sequence[Nindex+1] == 2))
						SPI_transmit(BIT1 + BIT2);		// LED 2 & 3
					else if ((sequence[Nindex] == 2 && sequence[Nindex+1] == 4) || (sequence[Nindex] == 4 && sequence[Nindex+1] == 4))
						SPI_transmit(BIT1 + BIT3);		// LED 2 & 4
					else if ((sequence[Nindex] == 3 && sequence[Nindex+1] == 4) || (sequence[Nindex] == 4 && sequence[Nindex+1] == 3))
						SPI_transmit(BIT2 + BIT3);		// LED 3 & 4
				}
			}
			countLED ++;
			if (countLED == 100){				// Display each element of the sequence for 0.8 sec
				Nindex += indexSequence[Index];
				Index ++;
				SPI_transmit(0x00);
			}
			if (Index == Turn){
				countLED = 0;
				Index = 0;
				Nindex = 0;
				SPI_transmit(0x00);				// Turn off the LEDs
				displayFlag = 0;				// Finish sequence display
				timeoutCount = 0;
			}
			if (countLED == 200)
				countLED = 0;
		}
		// In play mode 4
		if (playMode == 4){
			if (countLED == 0){
				if (indexSequence[Index] == 1){
					if (sequence[Nindex] == 1)
						SPI_transmit(BIT0);
					else if (sequence[Nindex] == 2)
						SPI_transmit(BIT1);
					else if (sequence[Nindex] == 3)
						SPI_transmit(BIT2);
					else if (sequence[Nindex] == 4)
						SPI_transmit(BIT3);
				}
				if (indexSequence[Index] == 2){
					if ((sequence[Nindex] == 1 && sequence[Nindex+1] == 2) || (sequence[Nindex] == 2 && sequence[Nindex+1] == 1))
						SPI_transmit(BIT0 + BIT1);		// LED 1 & 2
					else if ((sequence[Nindex] == 1 && sequence[Nindex+1] == 3) || (sequence[Nindex] == 3 && sequence[Nindex+1] == 1))
						SPI_transmit(BIT0 + BIT2);	 	// LED 1 & 3
					else if ((sequence[Nindex] == 1 && sequence[Nindex+1] == 4) || (sequence[Nindex] == 4 && sequence[Nindex+1] == 1))
						SPI_transmit(BIT0 + BIT3);		// LED 1 & 4
					else if ((sequence[Nindex] == 2 && sequence[Nindex+1] == 3) || (sequence[Nindex] == 3 && sequence[Nindex+1] == 2))
						SPI_transmit(BIT1 + BIT2);		// LED 2 & 3
					else if ((sequence[Nindex] == 2 && sequence[Nindex+1] == 4) || (sequence[Nindex] == 4 && sequence[Nindex+1] == 4))
						SPI_transmit(BIT1 + BIT3);		// LED 2 & 4
					else if ((sequence[Nindex] == 3 && sequence[Nindex+1] == 4) || (sequence[Nindex] == 4 && sequence[Nindex+1] == 3))
						SPI_transmit(BIT2 + BIT3);		// LED 3 & 4
				}
			}
			countLED ++;
			if (countLED == 63){				// Display each element of the sequence for 0.8 sec
				Nindex += indexSequence[Index];
				Index ++;
				SPI_transmit(0x00);
			}
			if (Index == Turn){
				countLED = 0;
				Index = 0;
				Nindex = 0;
				SPI_transmit(0x00);				// Turn off the LEDs
				displayFlag = 0;				// Finish sequence display
				timeoutCount = 0;
			}
			if (countLED == 125)
				countLED = 0;
		}
	}

	/* Check for timeout */
	if (displayFlag == 0 && playMode != 0 && animationFlag == 0)
		timeoutCount ++;
	if (playMode == 1 && timeoutCount == 375 && animationFlag == 0){		// Allow 3 seconds delay before go to lose mode
		countLED = 0;
		animationFlag = 2;			// Play lose mode animation
		buzzerCount = 0;
		displayFlag = 0;
		playMode = 0;
	}
	if (playMode == 2 && timeoutCount == 250 && animationFlag == 0){		// Allow 2 second delay before go to lose mode
		countLED = 0;
		animationFlag = 2;			// Play lose mode animation
		buzzerCount = 0;
		displayFlag = 0;
		playMode = 0;
	}
	if (playMode == 3 && timeoutCount == 375 && animationFlag == 0){		// Allow 3 second delay before go to lose mode
		countLED = 0;
		animationFlag = 2;			// Play lose mode animation
		buzzerCount = 0;
		displayFlag = 0;
		playMode = 0;
	}
	if (playMode == 4 && timeoutCount == 250 && animationFlag == 0){		// Allow 3 second delay before go to lose mode
		countLED = 0;
		animationFlag = 2;			// Play lose mode animation
		buzzerCount = 0;
		displayFlag = 0;
		playMode = 0;
	}


	/* Check the game time with the previous best */
	if (playMode != 0 && displayFlag == 0 && animationFlag == 0){
		if (first_press == 1){
			gameTime = 0;				// Start recording the time after the first right press
			first_press = 0;
		}
		gameTime ++;
	}
	if (winFlag == 1){
		winFlag = 0;
		if (gameTime < pregameTime){	// Play win mode with record-break animation
			countLED = 0;
			k = 0;
			buzzerCount = 0;
			animationFlag = 4;
			playMode = 0;
			pregameTime = gameTime;		// Update the previous game time
		}
		else {
			countLED = 0;
			k = 0;
			buzzerCount = 0;
			animationFlag = 3;			// Play win mode animation
			playMode = 0;
		}
	}


	/* Reset */
	if ((P2IN & (BIT0 + BIT2)) == 0x00){
		resetCount ++;						// Detect how many consecutive times both buttons are pressed at the same time
	}
	else{
		resetCount = 0;
	}

	if (resetCount == 250 && (animationFlag == 2 || animationFlag == 3 || animationFlag == 4)){
		playMode = 0;
		resetFlag = 2;
		animationFlag = 1;								// Play start animation
		displayFlag = 0;
		winFlag = 0;
		Index = 0;
		Turn = 1;
		first_press = 0;
		gameTime = 0;
		portEnable1 = 0;
		portEnable2 = 0;
		portEnable3 = 0;
		portEnable4 = 0;
		countLED = 0;
		subcountLED = 0;
		intensity = 0;
		timeoutCount = 0;
		NseqLength = 0;
		buzzerCount = 0;
		indexCheck = 0;
		rightPress = 0;
		doublePress = 0;
		Nindex = 0;
		P2IFG &= ~(BIT0 + BIT2 + BIT3 + BIT4);    		// P2.0, P2.2, P2.3, P2.4 IFG cleared
		P2IE |= BIT0 + BIT2 + BIT3 + BIT4; 				// Enable port2 interrupt
		P2IES |= BIT0 + BIT2 + BIT3 + BIT4; 						    // Reset to detect falling edges first
		free(sequence);									// Free the memory allocated to the array
	}


	/*LED animation and buzzer control */
	// start mode animation
	if (animationFlag == 1){
		playMode = 0;
		if (countLED == 0){
			SPI_transmit(BIT0+BIT2);
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (countLED == 64){
			SPI_transmit(BIT1+BIT3);
			TA1CCR0 = periods[4];
			TA1CCR1 = (int)(periods[4]*0.05);	// Duty cycle 5%
		}
		countLED ++;
		if (countLED == 128)
			countLED = 0;
	}
	// lose mode animation
	if (animationFlag == 2){
		if (countLED == 0){
			SPI_transmit2();
			TA1CCR0 = periods[4];
			TA1CCR1 = (int)(periods[4]*0.05);	// Duty cycle 5%
		}
		if (countLED == 50){
			SPI_transmit2();
			TA1CCR0 = periods[3];
			TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
		}
		if (countLED == 100){
			SPI_transmit2();
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (countLED == 150){
			SPI_transmit2();
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (countLED == 200){
			SPI_transmit2();
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (countLED == 250){
			SPI_transmit2();
			TA1CCR0 = periods[0];
			TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
		}
		if (countLED == 300){
			SPI_transmit2();
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (countLED == 350){
			SPI_transmit2();
			TA1CCR0 = periods[0];
			TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
		}
		countLED ++;
		if (countLED == 400)
			countLED = 0;
	}
	// win mode animation
	if (animationFlag == 3){
		subcountLED ++;
		buzzerCount ++;
		if (subcountLED == 2){
			SPI_transmit3();
			intensity ++;
			subcountLED = 0;
			if (intensity > 31)
				intensity = 0;
		}
		countLED ++;
		if (buzzerCount == 35){
			TA1CCR0 = periods[k];
			TA1CCR1 = (int)(periods[k]*0.05);	// Duty cycle 5%
			k ++;
			buzzerCount = 0;
		}
		if (k >= 8)
			k=0;
		if (countLED == 256)
			countLED = 0;
	}
	// win mode with record-break animation
	if (animationFlag == 4){
		subcountLED ++;
		if (subcountLED == 2){
			SPI_transmit4();
			intensity ++;
			subcountLED = 0;
			if (intensity > 31)
				intensity = 0;
		}
		countLED ++;
		if (countLED == 256)
			countLED = 0;
		if (buzzerCount == 0){	// 3 3
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 80){ // 4
			TA1CCR0 = periods[3];
			TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 120){  // 5 5
			TA1CCR0 = periods[4];
			TA1CCR1 = (int)(periods[4]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 200){ // 4
			TA1CCR0 = periods[3];
			TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 240){ // 3
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 280){ // 2
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 320){ // 1 1
			TA1CCR0 = periods[0];
			TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 400){ // 2
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 440){ // 3 3.
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 540){ // 2_ 2-
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 640){ // 3 3
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 720){ // 4
			TA1CCR0 = periods[3];
			TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 760){  // 5 5
			TA1CCR0 = periods[4];
			TA1CCR1 = (int)(periods[4]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 840){ // 4
			TA1CCR0 = periods[3];
			TA1CCR1 = (int)(periods[3]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 880){ // 3
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 920){ // 2
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 960){ // 1 1
			TA1CCR0 = periods[0];
			TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 1040){ // 2
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 1080){ // 3
			TA1CCR0 = periods[2];
			TA1CCR1 = (int)(periods[2]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 1120){ // 2.
			TA1CCR0 = periods[1];
			TA1CCR1 = (int)(periods[1]*0.05);	// Duty cycle 5%
		}
		if (buzzerCount == 1180){ // 1_ 1-
			TA1CCR0 = periods[0];
			TA1CCR1 = (int)(periods[0]*0.05);	// Duty cycle 5%
		}
		buzzerCount ++;
		if (buzzerCount == 1280){
			TA1CCR1 = 0;	// Duty cycle 5%
			buzzerCount = 0;
		}
	}
}


// Function configuring SPI using module USCI_A0
void SPI_config(){
	UCA0CTL1 = UCSWRST;                           // Disable SPI
	UCA0CTL0 |= UCCKPH + UCMST + UCSYNC + UCMSB;  // 8-bit SPI master, MSb 1st, synchronous mode
	UCA0CTL1 |= UCSSEL_2;                         // SMCLK
	UCA0BR0 = 0x04;                               // Set Frequency divider to 4
	UCA0BR1 = 0;
	UCA0CTL1 &= ~UCSWRST;                         // Initialize USCI state machine, USCI reset released for operation.

}


/* Controls the LEDs in display, play or start mode */
// The least significant 4 bits of whichLED correspond to the controlled LEDs.
// "1" in the bit corresponds to ON, "0" in the bit corresponds to OFF.
// In start mode, LED 1 & 3, LED 2 & 4 blinks alternately
void SPI_transmit(uint8_t whichLED){
	b_led_frame = 0x00;	 		// LED sub-frames
	g_led_frame = 0x00;
	r_led_frame = 0x00;

	// transmit 32-bit start frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));    //  USCI_A0 TX buffer ready?
		UCA0TXBUF = 0x00;
	}
	// transmit 32-bit LED frames for 4 LEDs
	for (i = 1; i <= 4; i++){
		b_led_frame = 0x00;
		g_led_frame = 0x00;
		r_led_frame = 0x00;
		switch (i){
		case 1: {if ((BIT0 & whichLED) != 0x00)  	// LED 1 should be on
			b_led_frame = 0xFF;
		}
		break;
		case 2: {if ((BIT1 & whichLED) != 0x00)		// LED 2 should be on
			g_led_frame = 0xFF;
		}
		break;
		case 3: {if ((BIT2 & whichLED) != 0x00)		// LED 3 should be on
			r_led_frame = 0xFF;
		}
		break;
		case 4: {if ((BIT3 & whichLED) != 0x00){	// LED 4 should be on
			r_led_frame = 0x7F;
			g_led_frame = 0x7F;
			}
		}
		break;
		}
		// transmit 32-bit LED frame
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 224 + 15;		   // global LED frame that controls the intensity of the LED, set to middle range
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = b_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = g_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = r_led_frame;
	}
	// transmit 32-bit end frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 0xFF;
	}
}


/* Controls the LEDs in lose mode */
// Sequential ON/OFF of yellow LED lights followed by two 4-LED red blinks
void SPI_transmit2(){
	b_led_frame = 0x00;	 		// LED sub-frames
	g_led_frame = 0x00;
	r_led_frame = 0x00;

	// transmit 32-bit start frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));    //  USCI_A0 TX buffer ready?
		UCA0TXBUF = 0x00;
	}
	// transmit 32-bit LED frames for 4 LEDs
	for (i = 1; i <= 4; i++){
		b_led_frame = 0x00;
		g_led_frame = 0x00;
		r_led_frame = 0x00;
		if (countLED == 0 && i == 1){  	// LED 1 should be on
			g_led_frame = 0x7F;
			r_led_frame = 0x6F;
		}
		else if (countLED == 50 && i == 2){  	// LED 2 should be on
			g_led_frame = 0x2A;
			r_led_frame = 0xAA;
		}
		else if (countLED == 100 && i == 3){  	// LED 3 should be on
			g_led_frame = 0x10;
			r_led_frame = 0xCA;
		}
		else if (countLED == 150 && i == 4){  	// LED 4 should be on
			g_led_frame = 0x08;
			r_led_frame = 0xE0;
		}
		else if (countLED == 200){  	// All LEDs should be on
			r_led_frame = 0xFF;
		}
		else if (countLED == 300){  	// All LEDs should be on
			r_led_frame = 0xFF;
		}
		// transmit 32-bit LED frame
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 224 + 15;		   // global LED frame that controls the intensity of the LED, set to middle range
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = b_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = g_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = r_led_frame;
	}
	// transmit 32-bit end frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 0xFF;
	}
}


/* Controls the LEDs in win mode */
// Smooth sequential intensity change
void SPI_transmit3(){
	int globalbits;
	b_led_frame = 0xFF;	 		// LED sub-frames
	g_led_frame = 0x00;
	r_led_frame = 0x00;

	// transmit 32-bit start frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));    //  USCI_A0 TX buffer ready?
		UCA0TXBUF = 0x00;
	}
	// transmit 32-bit LED frames for 4 LEDs
	for (i = 1; i <= 4; i++){
		if (countLED <= 63){
			switch (i){
				case 1:	{globalbits = 31 - intensity; break;}
				case 2: {globalbits = 31; break;}
				case 3: {globalbits = intensity; break;}
				case 4: {globalbits = 0; break;}
			}
		}
		else if (countLED >= 64 && countLED <= 127){
			switch (i){
				case 1:	{globalbits = 0; break;}
				case 2: {globalbits = 31 - intensity; break;}
				case 3: {globalbits = 31; break;}
				case 4: {globalbits = intensity; break;}
			}
		}
		else if (countLED >= 128 && countLED <= 191){
			switch (i){
				case 1:	{globalbits = intensity; break;}
				case 2: {globalbits = 0; break;}
				case 3: {globalbits = 31 - intensity; break;}
				case 4: {globalbits = 31; break;}
			}
		}
		else if (countLED >= 192 && countLED <= 255){
			switch (i){
				case 1:	{globalbits = 31; break;}
				case 2: {globalbits = intensity; break;}
				case 3: {globalbits = 0; break;}
				case 4: {globalbits = 31 - intensity; break;}
			}
		}
		// transmit 32-bit LED frame
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 224 + globalbits;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = b_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = g_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = r_led_frame;
	}
	// transmit 32-bit end frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 0xFF;
	}
}


/* Controls the LEDs in win mode with record-break */
// Smooth sequential color change from blue to red and backwards
void SPI_transmit4(){
	b_led_frame = 0x7F;	 		// LED sub-frames
	g_led_frame = 0x00;
	r_led_frame = 0x00;

	// transmit 32-bit start frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));    //  USCI_A0 TX buffer ready?
		UCA0TXBUF = 0x00;
	}
	// transmit 32-bit LED frames for 4 LEDs
	for (i = 1; i <= 4; i++){
		if (countLED <= 63){
			switch (i){
				case 1:	{r_led_frame = 255 - 8*intensity; b_led_frame = 7 + 8*intensity; break;}
				case 2: {r_led_frame = 255; b_led_frame = 7; break;}
				case 3: {r_led_frame = 7 + 8*intensity; b_led_frame = 255 - 8*intensity; break;}
				case 4: {r_led_frame = 7; b_led_frame = 255; break;}
			}
		}
		else if (countLED >= 64 && countLED <= 127){
			switch (i){
				case 1:	{r_led_frame = 7; b_led_frame = 255; break;}
				case 2: {r_led_frame = 255 - 8*intensity; b_led_frame = 7 + 8*intensity; break;}
				case 3: {r_led_frame = 255; b_led_frame = 7; break;}
				case 4: {r_led_frame = 7 + 8*intensity; b_led_frame = 255 - 8*intensity; break;}
			}
		}
		else if (countLED >= 128 && countLED <= 191){
			switch (i){
				case 1:	{r_led_frame = 7 + 8*intensity; b_led_frame = 255 - 8*intensity; break;}
				case 2: {r_led_frame = 7; b_led_frame = 255; break;}
				case 3: {r_led_frame = 255 - 8*intensity; b_led_frame = 7 + 8*intensity; break;}
				case 4: {r_led_frame = 255; b_led_frame = 7; break;}
			}
		}
		else if (countLED >= 192 && countLED <= 255){
			switch (i){
				case 1:	{r_led_frame = 255; b_led_frame = 7; break;}
				case 2: {r_led_frame = 7 + 8*intensity; b_led_frame = 255 - 8*intensity; break;}
				case 3: {r_led_frame = 7; b_led_frame = 255; break;}
				case 4: {r_led_frame = 255 - 8*intensity; b_led_frame = 7 + 8*intensity; break;}
			}
		}
		// transmit 32-bit LED frame
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 224 + 15;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = b_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = g_led_frame;
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = r_led_frame;
	}
	// transmit 32-bit end frame
	for (i = 1; i <= 4; i++){
		while(!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = 0xFF;
	}
}
