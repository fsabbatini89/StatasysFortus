/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__dsPIC30F__)
        #include <p30Fxxxx.h>
    #endif
#endif

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp              */
#include "stratasys/pkt.h"

#include <uart.h>

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/
RSTATE host; 
RSTATE cartdrige; 
RSTATE *r1 = &host;
RSTATE *r2 = &cartdrige;

/* HOST TO CARTDRIGE */
uint8_t pbuf[PKT_MAX_PLEN];
uint8_t dbuf[PKT_MAX_DLEN];

/* CARTRIDGE TO HOST */
uint8_t pbuf2[PKT_MAX_PLEN];
uint8_t dbuf2[PKT_MAX_DLEN];

uint8_t pkt_buf[PKT_MAX_PLEN];
uint8_t pkt_ibuf[PKT_MAX_PLEN];
uint8_t pkt_send_buf[PKT_MAX_PLEN + PKT_N_LEADIN];

uint8_t pkt_buf2[PKT_MAX_PLEN];
uint8_t pkt_ibuf2[PKT_MAX_PLEN];
uint8_t pkt_send_buf2[PKT_MAX_PLEN + PKT_N_LEADIN];

uint8_t cmdWriteData[PKT_MAX_DLEN];
uint16_t cmdWriteDataLength;

uint8_t ABS_M30_FULL[130] = {0x00 ,0x80 ,0x02 ,0x7f ,0x12 ,0xca ,0x6b ,0x41 ,0xe3 ,0xcd ,0x50 ,0xe9 ,0x80 ,0x84 ,0x37 ,0x53 ,0x22 ,0x8e ,0xe1 ,0x35 ,0x05 ,0x6f ,0x27 ,0xad ,0x58 ,0xf2 ,0x07 ,0x5f ,0xf3 ,0x9d ,0xbf ,0xdd ,0xf2 ,0x7f ,0x3c ,0x32 ,0xa1 ,0x6b ,0x55 ,0xbe ,0xed ,0x93 ,0x6d ,0x7d ,0x05 ,0x32 ,0xa5 ,0x79 ,0xe4 ,0xd8 ,0xa8 ,0x6d ,0x99 ,0xcb ,0xd1 ,0x09 ,0x97 ,0xe6 ,0xee ,0x4f ,0xac ,0xdc ,0xf3 ,0xdf ,0xcc ,0x84 ,0x41 ,0xe2 ,0x78 ,0x30 ,0x32 ,0x5a ,0xe9 ,0x89 ,0x61 ,0x75 ,0xe6 ,0xdc ,0x76 ,0xb6 ,0x89 ,0xf6 ,0xba ,0xc0 ,0x31 ,0xf3 ,0x18 ,0xd6 ,0x87 ,0x35 ,0x3b ,0x98 ,0x9a ,0xc8 ,0x38 ,0x25 ,0x60 ,0x44 ,0x54 ,0x0d ,0x3a ,0x2d ,0xd8 ,0x4d ,0x13 ,0x16 ,0x53 ,0x54 ,0x52 ,0x41 ,0x54 ,0x41 ,0x53 ,0x59 ,0x53 ,0x9b ,0xaa ,0xff ,0xf0 ,0x82 ,0x3f ,0xa0 ,0x38 ,0x11 ,0xad ,0x82 ,0x86 ,0xfd ,0xf9 ,0x89};

uint8_t lastCommandNumber;

bool waitForHost = true;

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

enum {
    WRITE_CATRIDGE_WRITE,               /* HOST -> CARTDRIGE */
    WRITE_CATRIDGE_ACK_WRITE,           /* CARTDRIGE -> HOST */
    WRITE_CATRIDGE_WRITE_DONE,          /* CARTDRIGE -> HOST */
    WRITE_CATRIDGE_WAIT_FOR_HOST_ASK,
    WRITE_CATRIDGE_ACK_WRITE_DONE,      /* HOST -> CARTDRIGE */
    WRITE_CATRIDGE_WAIT_FOR_HOST_READ,
    WRITE_CATRIDGE_READ_START          /* HOST -> CARTDRIGE */
};

uint8_t stateWrite;

volatile unsigned int pepe;

int16_t main(void)
{
    uint8_t *pdata1, pdata1_len;
    uint8_t *pdata2, pdata2_len;

    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize IO ports and peripherals */
    InitApp();

    stateWrite = WRITE_CATRIDGE_WRITE;
    
//    pbuf[0] = 'f';
//    pbuf[1] = 'r';
//    pbuf[2] = 'a';
//    pbuf[3] = 'n';
//    pbuf[4] = 'c';
//    pbuf[5] = 'o';
//
//    LATBbits.LATB1 = 0;
//    LATBbits.LATB1 = 1;
//            
//    while(1){
//        pkt_send2(r2, 1, 2, 3, PKTF_RESP_WRITE_CARTDRIGE, 6, pbuf);
//        LATBbits.LATB0 ^= 1;
//        LATBbits.LATB1 ^= 1;
//        delay_ms(1000);
//    }
    
    while(1)
    {
        switch(stateWrite){
            case WRITE_CATRIDGE_ACK_WRITE:
                /* To this command we need to answer with PKTF_RESP_WRITE_CARTDRIGE and 0 data */                
                pdata2 = NULL;
                pdata2_len = 0;
                pkt_send(r2, PKT_SRC(pbuf), PKT_DST(pbuf), PKT_NUMCMD(pbuf), PKTF_RESP_WRITE_CARTDRIGE, pdata2_len, pdata2);
                
                stateWrite = WRITE_CATRIDGE_WRITE_DONE;
            break;
                           
            case WRITE_CATRIDGE_WRITE_DONE:
                /* To this command we need to answer with PKTF_RESP_WRITTEN_CARTDRIGE and 0 data */                
                pdata2 = NULL;
                pdata2_len = 0;
                    
                lastCommandNumber++;
                    
                pkt_send(r2, PKT_SRC(pbuf), PKT_DST(pbuf), lastCommandNumber, PKTF_RESP_WRITTEN_CARTDRIGE, pdata2_len, pdata2);
                
                stateWrite = WRITE_CATRIDGE_WAIT_FOR_HOST_ASK;
                
            break;
                
            case WRITE_CATRIDGE_ACK_WRITE_DONE:    
                /* To this command we need to answer with PKTF_RESP_WRITE_CARTDRIGE and 0 data */                
                pdata2 = NULL;
                pdata2_len = 0;

                pkt_send(r2, PKT_SRC(pbuf), PKT_DST(pbuf), PKT_NUMCMD(pbuf), PKTF_RESP_ASK_WRITE_DONE_CARTDRIGE, pdata2_len, pdata2);
                    
                stateWrite = WRITE_CATRIDGE_READ_START;
            break;
                
            case WRITE_CATRIDGE_READ_START:                            
                /* To this command we need to answer with PKTF_RESP_WRITE_CARTDRIGE and 0 data */                
                pdata2 = cmdWriteData;
                pdata2_len = cmdWriteDataLength;

                lastCommandNumber++;
                
                //Respond as cartridge is useful
//                if(pdata2_len == 3){
//                    uint8_t* p = pdata2;
//                    if((*(++p) == 0x81) && (*(++p) == 0x55)){
//                        *p = 0x00;
//                    }
//                }
                
                pkt_send(r2, PKT_SRC(pbuf), PKT_DST(pbuf), PKT_NUMCMD(pbuf), PKTF_RESP_READ_CARTDRIGE, pdata2_len, pdata2);

                stateWrite = WRITE_CATRIDGE_WRITE;
                    
            break;
        }
                
        /* Host instruction received */
        if(r1->rx){   
            int8_t err;
                    
            /*
            * received an RS485 packet, copy it to our local buffer and
            * reset the rx flag so that new packets can be received.
            */
            DisableIntU1RX;
            memcpy(pbuf, r1->rbuf, PKT_LEN(r1->rbuf));
            err = r1->err;
            r1->rx = 0;
            IFS0bits.U1RXIF = 0;
            EnableIntU1RX;

            pdata1 = NULL;
            pdata1_len = 0;
            
            uint8_t cmd = PKT_COMMAND(pbuf);
            
            if(cmd == PKTF_CMD_ACK){
                /* Save last command number for future use */
                lastCommandNumber = PKT_NUMCMD(pbuf);
            }
        
            if(cmd == PKTF_CMD_WRITE_CARTDRIGE){
                /* Save data as we need to respond with it later on */
                cmdWriteDataLength = PKT_DLEN(pbuf);
                memcpy(cmdWriteData, PKT_DATA(pbuf), cmdWriteDataLength);
                
                //LATBbits.LATB1 ^= 1;
                
                stateWrite = WRITE_CATRIDGE_ACK_WRITE;
            }   
            
            if((cmd == PKTF_CMD_ASK_WRITE_DONE_CARTDRIGE) && (stateWrite == WRITE_CATRIDGE_WAIT_FOR_HOST_ASK)){  
                stateWrite = WRITE_CATRIDGE_ACK_WRITE_DONE;
            }              
            
            if((cmd == PKTF_CMD_ACK) && (stateWrite == WRITE_CATRIDGE_WAIT_FOR_HOST_READ)){                
                stateWrite = WRITE_CATRIDGE_READ_START;
            }

            /* Forward all messages from host to cartridge */
            if(stateWrite == WRITE_CATRIDGE_WRITE){
                pkt_send(r1, PKT_DST(pbuf), PKT_SRC(pbuf), PKT_NUMCMD(pbuf), PKT_COMMAND(pbuf), PKT_DLEN(pbuf), PKT_DATA(pbuf));
                
                waitForHost = false;
            }     
        }
        
        
        
        
        
        /* Cartridge instruction received */
        if(r2->rx){   
            int8_t err2;
            uint16_t len;
                    
            /*
            * received an RS485 packet, copy it to our local buffer and
            * reset the rx flag so that new packets can be received.
            */
            
            DisableIntU2RX;
            memcpy(pbuf2, r2->rbuf, PKT_MAX_PLEN);
            len = PKT_LEN(r2->rbuf);
            
            err2 = r2->err;
            r2->rx = 0;
            IFS1bits.U2RXIF = 0;
            EnableIntU2RX;

            pdata2 = NULL;
            pdata2_len = 0;
            
            uint8_t cmd = PKT_COMMAND(pbuf2);
            
            //Respond as cartridge is useful
            if((cmd == PKTF_RESP_READ_CARTDRIGE) && (PKT_DLEN(pbuf2) == 3)){
                uint8_t* p = PKT_DATA(pbuf2);
                if((*(++p) == 0x81) && (*(++p) == 0x55)){
                    *p = 0x00;
                }
            }
            
//            if((cmd == PKTF_RESP_READ_CARTDRIGE) && (PKT_DLEN(pbuf2) == 0x82)){
//                uint8_t* p = PKT_DATA(pbuf2);
//                memcpy(p, ABS_M30_FULL, 130);
//            }
            
            /* Forward all messages from cartridge to host */
            if(stateWrite == WRITE_CATRIDGE_WRITE){                            
                pkt_send2(r2, PKT_DST(pbuf2), PKT_SRC(pbuf2), PKT_NUMCMD(pbuf2), PKT_COMMAND(pbuf2), PKT_DLEN(pbuf2), PKT_DATA(pbuf2));
            }
        }
    }
}

/* This is UART1 transmit ISR */
void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
    IFS0bits.U1TXIF = 0;
    
    if(i1r){
        U1TXREG = i1r->sbuf[i1r->sidx++]; //Transfer next byte of message
             
        if(i1r->sidx == i1r->slen) {    //Message transfer complete
            i1r->tx_complete = 1;
            //PIE1bits.TXIE = 0; //EUSART transmit interrupt disable
            DisableIntU1TX;
            return;
        }
    }
}

void __attribute__((interrupt, no_auto_psv)) _U1ErrInterrupt(void)
{
    unsigned int rchar;
    
    if(U1STAbits.OERR || U1STAbits.FERR || U1STAbits.PERR ){
        U1STAbits.OERR = 0;
        U1STAbits.FERR = 0;
        U1STAbits.PERR = 0;
        rchar = U1RXREG;
        pkt_abort(i1r);
        return;
    }
}

/* This is UART1 receive ISR */
void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
    unsigned int rchar;
    IFS0bits.U1RXIF = 0;
    
    if(U1STAbits.OERR || U1STAbits.FERR || U1STAbits.PERR ){
        U1STAbits.OERR = 0;
        U1STAbits.FERR = 0;
        U1STAbits.PERR = 0;
        rchar = U1RXREG;
        pkt_abort(i1r);
        return;
    }
   
    if(i1r){
        while(DataRdyUART1()){
            pkt_byte_rx(i1r, ReadUART1());
            //LATBbits.LATB0 ^= 1;
        }
    }
    
//    IEC0bits.U1RXIE = 1;
} 

/* This is UART2 transmit ISR */
void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void)
{
    IFS1bits.U2TXIF = 0;
        
    if(i2r){
        U2TXREG = i2r->sbuf[i2r->sidx++]; //Transfer next byte of message
             
        if(i2r->sidx == i2r->slen) {    //Message transfer complete
            i2r->tx_complete = 1;
            DisableIntU2TX;
            return;
        }
    }
}

void __attribute__((interrupt, no_auto_psv)) _U2ErrInterrupt(void)
{
    unsigned int rchar;
    
    if(U2STAbits.OERR || U2STAbits.FERR || U2STAbits.PERR ){
        U2STAbits.OERR = 0;
        U2STAbits.FERR = 0;
        U2STAbits.PERR = 0;
        rchar = U2RXREG;
        pkt_abort(i2r);
        return;
    }
}

/* This is UART2 receive ISR */
void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{
    unsigned int rchar;
    IFS1bits.U2RXIF = 0;
    
    if(U2STAbits.OERR || U2STAbits.FERR || U2STAbits.PERR ){
        U2STAbits.OERR = 0;
        U2STAbits.FERR = 0;
        U2STAbits.PERR = 0;
        rchar = U2RXREG;
        pkt_abort(i2r);
        return;
    }
    
    if(i2r){
        while(DataRdyUART2()){
            rchar = ReadUART2();
           // if(waitForHost == false){
                pkt_byte_rx2(i2r, rchar);
           // }
            //LATBbits.LATB0 ^= 1;
        }
    }
} 