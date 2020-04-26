/* 
 * File:   main.c
 * Author: Roy Snijders
 *
 * Created on 25 April 2020
 * NRF24L01+ slave / sensor board with:
 * 1) CO2 sensorMH-Z19B
 * 
 * Transmitter speed 250kbs
 * 
 *   Command ; Ack ; TypeChild ; Payload \n
 * 
 * Command = 0
 * ACK = 0
 * TypeChild:
 *  message1 = C0
 * 
 * Just some text
 * 
 */

#define NodeCMD '0'             //0= default command
#define NodeACK '0'             //0= no acknowledge
#define RF24payloadlen  16

#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"         //config PIC
#include "define.h"         
#include "main.h"
#include "timers.h"         //config timers
#include "uart.h"
#include "spi.h"
#include "nRF24L01.h"       //functions for nRF24L01 transiever 2.4GHz
#include "MHZ19B.h"

// other files can find it in "globals.h"
//-------- globals --------------------------------

//----------------------------------------------------------------------
int main(void)
{   delay_ms(100); /* give device time to power up */
    char   string[8], progresschar='.', co2error=0;
    unsigned int    co2value=0;  
    unsigned char   i,loopcount;
    
    Pinning_Init();
    SPI1_Init();
    UART1_Init(9600,0); 
    UART2_Init(9600,0); 
    Timers_Init();
    NRF24L01_Init(RF24payloadlen,NRF24L01_VAL_AUTO_ACK_OFF);      // payload length P0=16, others are 0
  
    UART1_WriteString("\e[m"); //default colors
    UART1_WriteString("\e[2J"); //clear screen
    UART1_WriteString("\e[H"); //Cursor home
        
    
    //------------- Loop forever ----------
    while (1)
    {   if( PIN_SW1==1) reset();
        //----------------------------------------------------------------
        // Init done, because there is only one device stay in this loop
        //if(loopcount++==4) 
        //{   UART1_WriteChar('\r');
        //    loopcount=0;
        //    if(progresschar=='.') progresschar=' '; else progresschar='.';
        //}
        //UART1_WriteChar(progresschar);
        //---------------- CO2 -----------------------------------------------
        co2error=CO2read(&co2value);
        if(co2error==0)
        {   //CO2 reading is valid
            
            sprintf(string,"%d",co2value);
            SendMessage('C','0',&string[0]);
        }else
        {   UART1_WriteString("Error CO2: ");
            UART1_WriteChar('0'+co2error);
            UART1_WriteString("\r\n");
        }
    
        delay_ms(100);
                    
        
        for (i=0;i<10;i++) delay_ms(100);                                       //delay 1sec
        PIN_LED0 = ~PIN_LED0;
    }
    return 0;
}


//=====================================
void Pinning_Init(void)
{
    ANSA    = 0x0000; // AD1PCFG or ANSA: 0=digital, 1=analog. 
    ANSB    = 0x0000;   
    ANSC    = 0x0000;

    // *************************************************************
    // Pin Mapping
    TRISA   =   0x0000; // Set  port A
    TRISB   =   0x4783; // Set   portB
                                //RB0 = UART2 - TX -> in (see datasheet)
                                //RB1 = UART2 - RX -> in (see datasheet)
                                //RB2 =        IFS0bits.T1IF = 0;      //Clear the Timer1 interrupt status flag
                                //RB3 = 
                                //RB4 = 
                                //RB5 =        IFS0bits.T1IF = 0;      //Clear the Timer1 interrupt status flag
                                //RB6 = 
                                //RB7 = 
                                //..........................
                                //RB8 =  SW1 input
                                //RB9 =  
                                //RB10 = MISO SPI1 =1= input
                                //RB11 = 
                                //RB12 = 
                                //RB13 = 
                                //RB14 =  IRQ nRF24L01+ =1 = input
                                //RB15 =  



    TRISC   =   0x0FFF;         // Set  port C 
                                //RC6 = UART1 - RX== in (see datasheet)
                                //RC7 = UART1 - TX==  in (see datasheet)

    
}


/* Interrupt Service Routine code goes here
*/
//_T1Interrupt() is the T1 interrupt service routine (ISR).
//void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void)
//{   //                           //Toggle output to LED
 //   IFS0bits.T1IF = 0;                      //Reset Timer1 interrupt flag and Return from ISR
//}



void SendMessage(char Nodetype, char ChildID, char *payload)
{   char   stringtx[34];
    unsigned char j=0,i;
    
    stringtx[j++]=NodeCMD;
    stringtx[j++]=';';
    stringtx[j++]=NodeACK;
    stringtx[j++]=';';
    stringtx[j++]=Nodetype;
    stringtx[j++]=ChildID;
    stringtx[j++]=';';
    for (i=0;payload[i]!='\0';i++) stringtx[j++]=payload[i];
    stringtx[j++]='\n';
    stringtx[j++]='\0';

    UART1_WriteString(&stringtx[0]);
        
    NRF24L01_IRQ_ClearAll();                                                //clear all interrupts in the 24L01 
    NRF24L01_PWR_UP();
    NRF24L01_FlushTX();
    NRF24L01_WriteTX_Payload((unsigned char *) &stringtx[0], RF24payloadlen, NRF24L01_VAL_TRANSMITDIRECT);         //transmit
    while(PIN_RF_IRQ != 0);                                              // if high IRQ not active and TX is not done yet, when low continue                
    NRF24L01_IRQ_ClearAll();                                                //clear all interrupts in the 24L01 
}