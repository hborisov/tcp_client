/* 
 * File:   tcp_client.c
 * Author: Hristo Borisov
 *
 * Created on August 31, 2014, 9:56 AM
 */
#define USE_OR_MASK
#define _XTAL_FREQ 8000000

#include <xc.h>
#include <stdlib.h>
#include <string.h>
#include <plib/spi.h>
#include "plib/usart.h"
#include "../../D/dev/pic/lib/myw5500/myw5500.h"

#pragma config FOSC = INTIO67
#pragma config WDTEN = OFF, LVP = OFF

//unsigned char postHeader[] = "POST http://192.168.1.103/http_server/REST HTTP/1.1\nContent-Type: application/x-www-form-urlencoded\nContent-Length: 14\n\n";
unsigned char xivelyPUT[] = "PUT http://173.203.98.29/v2/feeds/992213505 HTTP/1.1\nHost: 173.203.98.29\nAuthorization: Basic aHJpc3RvYm9yaXNvdjpAYmNkITIzNA==\nContent-Length: ";
unsigned char xivelyPayload[] = "{\"datastreams\":[{\"id\":\"tsensor1\",\"current_value\":\"";
unsigned char xivelyPayloadTrail[] = "\"}]}";
//unsigned char postRequest[] = "POST http://192.168.1.103/http_server/REST HTTP/1.1\nContent-Type: application/x-www-form-urlencoded\nContent-Length: 13\n\nparam1=value2\n\n";
unsigned char rxBuffer[32];

void setupUSART(void);
void setupSPI(void);
void delayOneSecond(void);
int http_post(unsigned char* data);
int http_server(void);


int main(void) {

    OSCCONbits.IDLEN = 0;
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 0;
    OSCCONbits.OSTS  = 0;
    OSCCONbits.IOFS  = 1;
    OSCCONbits.SCS1  = 1;
    OSCCONbits.SCS0  = 1;

    //timer 0 setup
    T0CONbits.PSA = 0;
    T0CONbits.T0PS2 = 1;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS0 = 1;
    T0CONbits.T0CS = 0;
    T0CONbits.T08BIT = 0;

    TMR0H = 0;
    TMR0L = 0;
    T0CONbits.TMR0ON = 1;

    TRISD = 0b00000000;
    LATD = 0b10000000;
    TRISAbits.RA5 = 0;  //SPI slave select
    PORTAbits.RA5 = 1;
    

    //setup interrupts on portb pin0
    INTCON2bits.INTEDG0 = 0;
    INTCONbits.INT0IF = 0;
    INTCON2bits.TMR0IP = 1;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    INTCONbits.INT0IE = 1;

    PIE1 = 0;
    PIE1bits.RCIE = 1;
    PIE2 = 0;
    RCONbits.IPEN = 1;
    INTCONbits.GIEL = 1;
    INTCONbits.PEIE = 1;  //for usart receive
    INTCONbits.GIEH = 1;


    //setup portb.0 as input
    INTCON2bits.RBPU = 0;
    WPUBbits.WPUB0 = 1;
    ANSELH = 0x00;
    TRISBbits.TRISB0 = 1;

    setupUSART();
    setupSPI();

    //CloseSPI();

    http_server();

    while(1);
    return 0;
}

void setupUSART(void) {
    SPBRG = 0;
    SPBRGH = 0;

    TRISCbits.RC6 = 1;
    TRISCbits.RC7 = 1;

    // wait until IOFS = 1 (osc. stable)
    while (!OSCCONbits.IOFS);

    unsigned char config=0,spbrg=0,baudconfig=0;
    CloseUSART();

    config = USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH;
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
    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;
        LATDbits.LATD3 = ~LATDbits.LATD3;
        while(BusyUSART());
        WriteUSART(0x01);
    }

    if (INTCONbits.INT0IF) {
        /*
        LATDbits.LATD4 = 1;
        //http_post(postRequest);
        unsigned char dat[128] = "";
        //buffer[7] = '\0';
        strcat(dat, xivelyPayload);
        strcat(dat, "36.8"); //buffer
        strcat(dat, xivelyPayloadTrail);

        unsigned char header_data[256];
        strcpy(header_data, xivelyPUT);
        char b[16];
        strcat(header_data, itoa(b, strlen(dat), 10));
        strcat(header_data, "\n\n");

        strcat(header_data, dat);
        strcat(header_data, "\n\n");

        while(BusyUSART());
        putsUSART(header_data);
        http_post(header_data);
        LATDbits.LATD4 = 0;
         */
    }

     if (PIR1bits.RCIF) {
        INTCONbits.GIEH = 0;
        LATDbits.LATD5 = 1;

          char i;    // Length counter
          unsigned char data;
          unsigned char buffer[16];

          for(i=0;i<8;i++) {
            while(!DataRdyUSART());

            data = getcUSART();
            
            if (USART_Status.OVERRUN_ERROR) {
                LATDbits.LATD0 = 1;
                setupUSART();
                LATDbits.LATD0 = 0;
                break;
            }

            if (USART_Status.FRAME_ERROR) {
                LATDbits.LATD1 = 1;
                setupUSART();
                LATDbits.LATD0 = 0;
                break;
            }

            buffer[i] = data;
          }
        
        unsigned char dat[128] = "";
        buffer[7] = '\0';
        strcat(dat, xivelyPayload);
        char temp[5];
        temp[0] = buffer[4];
        temp[1] = buffer[5];
        temp[2] = '.';
        temp[3] = buffer[6];
        temp[4] = '\0';
        strcat(dat, temp); //buffer
        strcat(dat, xivelyPayloadTrail);
        
        unsigned char header_data[256];
        strcpy(header_data, xivelyPUT);
        char b[16];
        strcat(header_data, itoa(b, strlen(dat), 10));
        strcat(header_data, "\n\n");
        
        strcat(header_data, dat);
        strcat(header_data, "\n\n");
        
        http_post(header_data);
    }

    LATDbits.LATD5 = 0;
    PIR1bits.RCIF = 0;
    INTCONbits.INT0IF = 0;
    INTCONbits.TMR0IF = 0;
    INTCONbits.GIEH = 1;
}

void interrupt low_priority isr_low(void) {
    if (PIR1bits.RCIF) {
        PIR1bits.RCIF = 0;
        LATDbits.LATD5 = 1;
        //char t = ReadUSART();
       // while(BusyUSART());
        unsigned char t = RCREG;
        //getsUSART((char *)rxBuffer,31);
        //putsUSART((char*)"\r\nchar... ");
    }

    LATDbits.LATD5 = 0;
    PIR1bits.RCIF = 0;
}

int http_post(unsigned char* data) {
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
        //uint8_t address[4] = {0xC0, 0xA8, 0x01, 0x67};   //192.168.1.103
        //setSocketDestinationIPAddress(SOCKET_1, address);
        //setSocketDestinationPort(SOCKET_1, 8080);
        uint8_t address[4] = {0xAD, 0xCB, 0x62, 0x1D};   //192.168.1.103
        setSocketDestinationIPAddress(SOCKET_1, address);
        setSocketDestinationPort(SOCKET_1, 80);
        connect(SOCKET_1);

        do {
            status = readSocketStatus(SOCKET_1);
            if (status == SOCK_CLOSED) {
                return -1;
            }

        } while (status != SOCK_ESTABLISHED);
        //while(BusyUSART());
        //putsUSART((char*)"\r\nSocket connected.");

        //sending process
        uint16_t freeSize = readTxFreeSize(SOCKET_1);
        //while(BusyUSART());
        //putsUSART((char*)"\r\nTx buffer free size: ");
        unsigned char freeSizeString[16];
        itoa(freeSizeString, freeSize, 16);
        while(BusyUSART());
        putsUSART(freeSizeString);

        //while(BusyUSART());
        //putsUSART((char*)"\r\nNumber of bytes to send: ");
        unsigned char numBytesString[16];
        itoa(numBytesString, strlen(data), 16);
        while(BusyUSART());
        putsUSART(numBytesString);


        uint16_t writePointer = readWritePointer(SOCKET_1);
        uint8_t writePointerH, writePointerL;
        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        //while(BusyUSART());
        //putsUSART((char*)"\r\nWrite Buffer high byte: ");
            itoa(numBytesString, writePointerH, 16);
            while(BusyUSART());
            putsUSART(numBytesString);
        //while(BusyUSART());
        //putsUSART((char*)"\r\nWrite Buffer low byte: ");
            itoa(numBytesString, writePointerL, 16);
            while(BusyUSART());
            putsUSART(numBytesString);

        writeToSocketTxBuffer(SOCKET_1_TX_BUFFER, writePointer, data);//, strlen(data));
        writePointer += strlen(data);

        writePointerH = (writePointer >> 8) & 0xFF;
        writePointerL = writePointer & 0xFF;
        //while(BusyUSART());
        //putsUSART((char*)"\r\nWrite Buffer high byte: ");
            itoa(numBytesString, writePointerH, 16);
            while(BusyUSART());
            putsUSART(numBytesString);
        //while(BusyUSART());
        //putsUSART((char*)"\r\nWrite Buffer low byte: ");
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

            delayOneSecond();
            delayOneSecond();
            delayOneSecond();
            delayOneSecond();
            delayOneSecond();
//receive process
        uint16_t numBytesReceived = readNumberOfBytesReceived(SOCKET_1);
        //while(BusyUSART());
        //putsUSART((char*)"\r\nNumber of bytes received: ");
        unsigned char numBytesString[16];
        itoa(numBytesString, numBytesReceived, 16);
        while(BusyUSART());
        putsUSART(numBytesString);
        //while(BusyUSART());
        //putsUSART((char*)"\r\n\r\n------------\r\n");

        uint8_t readBuffer[16]; //read only the http header replace with numBytesReceived; for more
        uint16_t readPointer = readReadPointer(SOCKET_1);
        readFromSocketRxBufferLen(SOCKET_1_RX_BUFFER, readPointer, readBuffer, 16);//numBytesReceived);

        for(int i=0; i<16; i++) {
            while(BusyUSART());
            WriteUSART(readBuffer[i]);
        }
        //while(BusyUSART());
        //putsUSART((char*)"\r\n------------\r\n");

        readPointer += numBytesReceived; //here we want to read all the data so that w5500 can receive more
        increaseReadPointer(SOCKET_1, readPointer);
        receive(SOCKET_1);

        disconnect(SOCKET_1);
        do {
            status = readSocketStatus(SOCKET_1);
        } while (status != SOCK_CLOSED);

        //while(BusyUSART());
        //putsUSART((char*)"\r\nfinito.");

        return 0;
}

int http_server(void) {
    while(BusyUSART());
    putsUSART((char*)"\r\nOpenning socket 2 ... ");
    setSocketTCPMode(SOCKET_2);
    setSocketSourcePort(SOCKET_2, 8080);
    openSocket(SOCKET_2);

    uint8_t status;
    do {
        status = readSocketStatus(SOCKET_2);

        if (status == SOCK_CLOSED) {
            return -1;
        }
    } while (status != SOCK_INIT);
    while(BusyUSART());
    putsUSART((char*)"\r\nSocket 2 opened. ");

    listen(SOCKET_2);

    do {
        status = readSocketStatus(SOCKET_2);
        if (status == SOCK_CLOSED) {
            return -1;
        }

    } while (status != SOCK_LISTEN);
    while(BusyUSART());
    putsUSART((char*)"\r\nSocket 2 listening. ");

    //wait for request
    while(1) {
        status = readSocketStatus(SOCKET_2);
        if (status == SOCK_ESTABLISHED) {
            while(BusyUSART());
            putsUSART((char*)"\r\nRequest received. ");

            uint16_t numBytesReceived = readNumberOfBytesReceived(SOCKET_2);
            while(BusyUSART());
            putsUSART((char*)"\r\nNumber of bytes received: ");
            unsigned char numBytesString[16];
            itoa(numBytesString, numBytesReceived, 16);
            while(BusyUSART());
            putsUSART(numBytesString);
            //while(BusyUSART());
            //putsUSART((char*)"\r\n\r\n------------\r\n");

            uint8_t readBuffer[256]; //read only the http header replace with numBytesReceived; for more
            uint16_t readPointer = readReadPointer(SOCKET_2);
            readFromSocketRxBufferLen(SOCKET_2_RX_BUFFER, readPointer, readBuffer, numBytesReceived);//numBytesReceived);

            for(int i=0; i<numBytesReceived; i++) {
                while(BusyUSART());
                WriteUSART(readBuffer[i]);
            }
            //while(BusyUSART());
            //putsUSART((char*)"\r\n------------\r\n");

            readPointer += numBytesReceived; //here we want to read all the data so that w5500 can receive more
            increaseReadPointer(SOCKET_2, readPointer);
            receive(SOCKET_2);

            disconnect(SOCKET_1);
            do {
                status = readSocketStatus(SOCKET_1);
            } while (status != SOCK_CLOSED);

            return 0;
        }
    }

    return 0;
}