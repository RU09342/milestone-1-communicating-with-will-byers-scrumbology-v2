#include <msp430.h>
/*
 *  MILESTONE RGB 5529.
 *
 *  Created on: Oct 15, 2017
 *  Last Edited: Oct 15, 2017
 *  Author: Tyler Brady / Mike Giuliano
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //LED BIT SETUP
    P1DIR |= (BIT2 + BIT3 + BIT4); // Set P1.2, P1.3, and P1.4 to output direction
    P1SEL |= (BIT2 + BIT3 + BIT4); // Sets P1.2, P1.3, and P1.4 to be the output of the CCR1, CCR2, and CCR3

    //PWM TIMER SETUP
    TA0CCTL1 = OUTMOD_7;//SET/RESET MODE RED
    TA0CCTL2 = OUTMOD_7;//SET/RESET MODE GREEN
    TA0CCTL3 = OUTMOD_7;//SET/RESET MODE BLUE
    TA0CCR0 = 255; // Full Cycle
    TA0CCR1 = 0; //Red set at 0%
    TA0CCR2 = 0; //Green set at 0%
    TA0CCR3 = 0; //Blue set at 0%
    TA0CTL = TBSSEL_2 + MC_1 + ID_2; //SMCLK/4, up mode

    //UART SETUP
    P3SEL |= BIT3; //TX peripheral mode
    P3SEL |= BIT4; //RX peripheral mode
    UCA0CTL1 |= (UCSWRST); //State machine reset + small clock initialization
    UCA0CTL1 |= UCSSEL_2;
    UCA0BR0 = 6; //9600 baud *Determined by TI calculator(http://processors.wiki.ti.com/index.php/USCI_UART_Baud_Rate_Gen_Mode_Selection)
    UCA0BR1 = 0; //9600 baud *Determined by TI calculator(http://processors.wiki.ti.com/index.php/USCI_UART_Baud_Rate_Gen_Mode_Selection)
    UCA0MCTL |= UCBRS_0; // Modulation
    UCA0MCTL |= UCBRF_13;
    UCA0MCTL |= UCOS16;
    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
    UCA0IE |= UCRXIE; // Enable USCI_A0 RX interrupt
    //


    _BIS_SR(LPM0_bits + GIE); // Enter LPM0 w/ interrupt
    while(1)
    {
    }
}
volatile int bitcounter = 0;
#pragma vector = USCI_A0_VECTOR
__interrupt void USCI_A0 (void) //Interrupt fires any time an input or output occurs with UART
{
    switch(__even_in_range(UCA0IV, USCI_UCTXIFG)) //Equivalent to switch(UCA0IV) but generates more efficient code
    {
        case USCI_NONE: break; //Handles the case where RX = NULL

        case USCI_UCRXIFG: //Handles the case of an input being received.
            switch(bitcounter)
            {
                case 0:
                    while(!(UCA0IFG & UCTXIFG)); //As long as you're not already transmitting, continue, else trap until not transmitting.
                    UCA0TXBUF = UCA0RXBUF - 3; //Transmits the length of the remaining bytes
                    __no_operation(); //Pauses the clock for a moment
                    break;
                case 1:
                    TA0CCR1 = (UCA0RXBUF); //SETS RED SECTION
                    break;
                case 2:
                    TA0CCR3 = (UCA0RXBUF); //SETS GREEN SECTION
                    break;
                case 3:
                    TA0CCR3 = (UCA0RXBUF); //SETS BLUE SECTION
                    break;
                default:
                    while(!(UCA0IFG & UCTXIFG)); //Repeat command from case 0.
                    UCA0TXBUF = UCA0RXBUF; //Just transmit the incoming byte on to the next board
            }
            if(UCA0RXBUF != 0x0D) {bitcounter += 1;} //As long as the byte received is not the final, count to the next state.
            else if(UCA0RXBUF == 0x0D){bitcounter = 0;} //Once the final byte is received reset the counter bit.
            break;
        case USCI_UCTXIFG: break; //In the case of the transmitting do nothing.

        default: break; // Covers the case to do nothing if somehow comes out of bounds.
    }
}
