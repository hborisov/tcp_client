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

       
        setSocketTCPMode(SOCKET_1);
        setSocketSourcePort(SOCKET_1, 4443);
        openSocket(SOCKET_1);

        uint8_t status = readSocketStatus(SOCKET_1);
        delayOneSecond();
        if (status != SOCK_INIT) {
            while(BusyUSART());
            putsUSART((char*)"\r\nCannot init socket.");
            closeSocket(SOCKET_1);
            return -1;
        } else {
            while(BusyUSART());
            putsUSART((char*)"\r\nSocket initialized.");
        }

        uint8_t address[4] = {0xC0, 0xA8, 0x01, 0x67};   //192.168.1.103
        setSocketDestinationIPAddress(SOCKET_1, address);
        setSocketDestinationPort(SOCKET_1, 4444);
        connect(SOCKET_1);

        while(1) {
            delayOneSecond();
            status = readSocketStatus(SOCKET_1);
            if (status != SOCK_ESTABLISHED) {
                while(BusyUSART());
                putsUSART((char*)"\r\nSocket not established: ");
                unsigned char numBytesString[10];
                itoa(numBytesString, status, 10);
                while(BusyUSART());
                putsUSART(numBytesString);
                closeSocket(SOCKET_1);
                return -1;
            } else {
                while(BusyUSART());
                putsUSART((char*)"\r\nSocket established: ");

                break;
            }
        }

        
    
        //sending process

        uint16_t freeSize = readTxFreeSize(SOCKET_1);
        while(BusyUSART());
        putsUSART((char*)"\r\nTx buffer free size: ");
        unsigned char freeSizeString[10];
        itoa(freeSizeString, freeSize, 10);
        while(BusyUSART());
        putsUSART(freeSizeString);

        unsigned char data[] = "GET /enlighten/calais.asmx HTTP/1.1\n";
        while(BusyUSART());
        putsUSART((char*)"\r\nNumber of bytes to send: ");
        unsigned char numBytesString[10];
        itoa(numBytesString, sizeof(data), 10);
        while(BusyUSART());
        putsUSART(numBytesString);


        uint16_t writePointer = readWritePointer(SOCKET_1);
        uint8_t writePointerH, writePointerL;
        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer high byte: ");
            //unsigned char numBytesString[10];
            itoa(numBytesString, writePointerH, 10);
            while(BusyUSART());
            putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer low byte: ");
            itoa(numBytesString, writePointerL, 10);
            while(BusyUSART());
            putsUSART(numBytesString);
                
        writeToSocketTxBuffer(SOCKET_1_TX_BUFFER, writePointer, data, sizeof(data));
        writePointer += sizeof(data);

        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer high byte: ");
            //unsigned char numBytesString[10];
            itoa(numBytesString, writePointerH, 10);
            while(BusyUSART());
            putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer low byte: ");
            itoa(numBytesString, writePointerL, 10);
            while(BusyUSART());
            putsUSART(numBytesString);

        increaseWritePointer(SOCKET_1, writePointer);

        send(SOCKET_1);

        writePointer = readWritePointer(SOCKET_1);
        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer high byte(after): ");
            //unsigned char numBytesString[10];
            itoa(numBytesString, writePointerH, 10);
            while(BusyUSART());
            putsUSART(numBytesString);
        while(BusyUSART());
        putsUSART((char*)"\r\nWrite Buffer low byte(after): ");
            itoa(numBytesString, writePointerL, 10);
            while(BusyUSART());
            putsUSART(numBytesString);


//receive process
        uint16_t numBytesReceived = readNumberOfBytesReceived(SOCKET_1);
        while(BusyUSART());
        putsUSART((char*)"\r\nNumber of bytes received: ");
        unsigned char numBytesString[10];
        itoa(numBytesString, numBytesReceived, 10);
        while(BusyUSART());
        putsUSART(numBytesString);

        uint8_t readBuffer[128];
        uint16_t readPointer = readReadPointer(SOCKET_1);
        readFromSocketRxBufferLen(SOCKET_1_RX_BUFFER, readPointer, readBuffer, numBytesReceived);

        for(int i=0; i<numBytesReceived; i++) {
            while(BusyUSART());
            WriteUSART(readBuffer[i]);
        }

        readPointer += numBytesReceived;
        increaseReadPointer(SOCKET_1, readPointer);
        receive(SOCKET_1);       
        

        delayOneSecond();
        status = readSocketStatus(SOCKET_1);
        delayOneSecond();
        if (status == SOCK_CLOSE_WAIT) {
            clearInterrupts(SOCKET_1);
            close(SOCKET_1);
            while(BusyUSART());
            putsUSART((char*)"\r\nsocket closed1.");
           // return 0;
        }

        disconnect(SOCKET_1);
        do {
            delayOneSecond();
            status = readSocketStatus(SOCKET_1);
        //    while(BusyUSART());
        //    WriteUSART(status);
            unsigned char numBytesString[10];
            itoa(numBytesString, status, 10);
            while(BusyUSART());
            putsUSART(numBytesString);

            if (status == SOCK_CLOSE_WAIT) {
            clearInterrupts(SOCKET_1);
            close(SOCKET_1);
            while(BusyUSART());
            putsUSART((char*)"\r\nsocket closed1.");
            break;
           // return 0;
        }
        } while (status != SOCK_CLOSED);
        //if (status == SOCK_CLOSED) {
            close(SOCKET_1);
            while(BusyUSART());
            putsUSART((char*)"\r\nsocket closed.");
        //}

        while(BusyUSART());
        putsUSART((char*)"\r\nfinito.");

    CloseSPI();

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