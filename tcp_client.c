/* 
 * File:   tcp_client.c
 * Author: hbb
 *
 * Created on August 31, 2014, 9:56 AM
 */
#define USE_OR_MASK
#define _XTAL_FREQ 8000000

#include <xc.h>
#include <stdlib.h>
#include <plib/spi.h>
#include "plib/usart.h"
#include "../test_c.X/../../D/dev/pic/lib/myw5500/myw5500.h"

#pragma config FOSC = INTIO67
#pragma config WDTEN = OFF, LVP = OFF


void setupUSART(void);
void setupSPI(void);
void delayOneSecond(void);

/*
 * 
 */
int main(void) {

    OSCCONbits.IDLEN = 0;
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 0;
    OSCCONbits.OSTS  = 0;
    OSCCONbits.IOFS  = 1;
    OSCCONbits.SCS1  = 1;
    OSCCONbits.SCS0  = 1;

    TRISD = 0b00001111;
    LATDbits.LATD7 = 1;

    TRISAbits.RA5 = 0;  //SPI slave select
    PORTAbits.RA5 = 1;

    setupUSART();
    setupSPI();
    CloseSPI();
}

void setupUSART(void) {
    SPBRG = 0;
    SPBRGH = 0;

    TRISCbits.RC6 = 1; //TX pin set as output
    TRISCbits.RC7 = 1; //RX pin set as input

    // wait until IOFS = 1 (osc. stable)
    while (!OSCCONbits.IOFS);

    unsigned char config=0,spbrg=0,baudconfig=0;
    CloseUSART();

    config = USART_TX_INT_OFF | USART_RX_INT_OFF | USART_ASYNCH_MODE | USART_EIGHT_BIT |
    USART_CONT_RX | USART_BRGH_HIGH;
    spbrg = 51;   // 9600-N-1 @ 8MHz   BRGH=1 BRG16=0
    OpenUSART(config, spbrg);

    TXSTA = 0b00100100;
    RCSTA = 0b10010000;

    baudUSART(baudconfig);
}

void setupSPI() {
    CloseSPI();

    TRISCbits.RC5 = 0;
    TRISCbits.RC4 = 1;
    TRISCbits.RC3 = 0;

    SSPSTAT = 0b01000000;
    SSPCON1 = 0b00100000;

    OpenSPI(SPI_FOSC_4, MODE_00, SMPMID);
}

void delayOneSecond(void) {
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
        __delay_ms(50);
}