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
#include "../../D/dev/pic/lib/myw5500/myw5500.h"

#pragma config FOSC = INTIO67
#pragma config WDTEN = OFF, LVP = OFF


void setupUSART(void);
void setupSPI(void);
void delayOneSecond(void);
int http_post(void);

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
    LATDbits.LATD5 = 0;
    PORTDbits.RD4 = 1;

    TRISAbits.RA5 = 0;  //SPI slave select
    PORTAbits.RA5 = 1;

    //setup interrupts on portb pin0
    INTCON2bits.INTEDG0 = 0;
    INTCONbits.INT0IF = 0;
    INTCONbits.INT0IE = 1;

    PIE1 = 0;
    PIE2 = 0;
    RCONbits.IPEN = 1;
    INTCONbits.GIEL = 1;
    INTCONbits.GIEH = 1;


    //setup portb.0 as input
    INTCON2bits.RBPU = 0;
    WPUBbits.WPUB0 = 1;
    ANSELH = 0x00;
    TRISBbits.TRISB0 = 1;


    setupUSART();
    setupSPI();

    //CloseSPI();

    while(1);
    return 0;
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

    config = USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH;
    spbrg = 51;   // 9600-N-1 @ 8MHz   BRGH=1 BRG16=0
    OpenUSART(config, spbrg);

    //TXSTA = 0b00100100;
    //RCSTA = 0b10010000;

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

void interrupt isr(void) {
    if (INTCONbits.INT0IF) {
        LATDbits.LATD5 = ~LATDbits.LATD5;
        http_post();
    }
    INTCONbits.INT0IF = 0;
}

int http_post(void) {
    while(BusyUSART());
        putsUSART((char*)"\r\nOpenning socket... ");
        setSocketTCPMode(SOCKET_1);
        setSocketSourcePort(SOCKET_1, 4443);
        openSocket(SOCKET_1);

        uint8_t status;
        do {
            status = readSocketStatus(SOCKET_1);

            if (status == SOCK_CLOSED) {
                return -1;
            }
        } while (status != SOCK_INIT);
        while(BusyUSART());
        putsUSART((char*)"\r\nSocket opened. ");

        while(BusyUSART());
        putsUSART((char*)"\r\nSocket connecting... ");
        uint8_t address[4] = {0xC0, 0xA8, 0x01, 0x67};   //192.168.1.103
        setSocketDestinationIPAddress(SOCKET_1, address);
        setSocketDestinationPort(SOCKET_1, 8080);
        connect(SOCKET_1);

        do {
            status = readSocketStatus(SOCKET_1);
            if (status == SOCK_CLOSED) {
                return -1;
            }

        } while (status != SOCK_ESTABLISHED);
        while(BusyUSART());
        putsUSART((char*)"\r\nSocket connected.");

        //sending process
        uint16_t freeSize = readTxFreeSize(SOCKET_1);
        while(BusyUSART());
        putsUSART((char*)"\r\nTx buffer free size: ");
        unsigned char freeSizeString[16];
        itoa(freeSizeString, freeSize, 16);
        while(BusyUSART());
        putsUSART(freeSizeString);

        unsigned char data[] = "POST http://192.168.1.103/http_server/REST HTTP/1.1\nContent-Type: application/x-www-form-urlencoded\nContent-Length: 13\n\nparam1=value2\n\n";
        while(BusyUSART());
        putsUSART((char*)"\r\nNumber of bytes to send: ");
        unsigned char numBytesString[16];
        itoa(numBytesString, sizeof(data), 16);
        while(BusyUSART());
        putsUSART(numBytesString);


        uint16_t writePointer = readWritePointer(SOCKET_1);
        uint8_t writePointerH, writePointerL;
        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer high byte: ");
            itoa(numBytesString, writePointerH, 16);
            while(BusyUSART());
            putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer low byte: ");
            itoa(numBytesString, writePointerL, 16);
            while(BusyUSART());
            putsUSART(numBytesString);

        writeToSocketTxBuffer(SOCKET_1_TX_BUFFER, writePointer, data, sizeof(data));
        writePointer += sizeof(data);

        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer high byte: ");
            itoa(numBytesString, writePointerH, 16);
            while(BusyUSART());
            putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer low byte: ");
            itoa(numBytesString, writePointerL, 16);
            while(BusyUSART());
            putsUSART(numBytesString);

        increaseWritePointer(SOCKET_1, writePointer);

        send(SOCKET_1);

        writePointer = readWritePointer(SOCKET_1);
        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer high byte(after): ");
            itoa(numBytesString, writePointerH, 16);
            while(BusyUSART());
            putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer low byte(after): ");
            itoa(numBytesString, writePointerL, 16);
            while(BusyUSART());
            putsUSART(numBytesString);


//receive process
        uint16_t numBytesReceived = readNumberOfBytesReceived(SOCKET_1);
        while(BusyUSART());
        putsUSART((char*)"\r\nNumber of bytes received: ");
        unsigned char numBytesString[16];
        itoa(numBytesString, numBytesReceived, 16);
        while(BusyUSART());
        putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\n\r\n------------\r\n");

        uint8_t readBuffer[128];
        uint16_t readPointer = readReadPointer(SOCKET_1);
        readFromSocketRxBufferLen(SOCKET_1_RX_BUFFER, readPointer, readBuffer, numBytesReceived);

        for(int i=0; i<numBytesReceived; i++) {
            while(BusyUSART());
            WriteUSART(readBuffer[i]);
        }
        while(BusyUSART());
        putsUSART((char*)"\r\n------------\r\n");

        readPointer += numBytesReceived;
        increaseReadPointer(SOCKET_1, readPointer);
        receive(SOCKET_1);

        disconnect(SOCKET_1);
        do {
            status = readSocketStatus(SOCKET_1);
        } while (status != SOCK_CLOSED);

        while(BusyUSART());
        putsUSART((char*)"\r\nfinito.");

        return 0;
}