#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkt.h"

extern void DelayMs(uint16_t ms);

extern volatile uint32_t ms_count;

#if PKT_TRACE
extern RINGBUF8 uart1buf;
#endif

extern uint8_t pkt_buf[PKT_MAX_PLEN];
extern uint8_t pkt_ibuf[PKT_MAX_PLEN];
extern uint8_t pkt_send_buf[PKT_MAX_PLEN + PKT_N_LEADIN];

extern uint8_t pkt_buf2[PKT_MAX_PLEN];
extern uint8_t pkt_ibuf2[PKT_MAX_PLEN];
extern uint8_t pkt_send_buf2[PKT_MAX_PLEN + PKT_N_LEADIN];

RSTATE *i1r = NULL;
RSTATE *i2r = NULL;

void pkt_init(RSTATE * r, uint8_t id)
{
    r->ridx        = 0;
    r->sidx        = 0;
    r->slen        = 0;
    r->myid        = id;  /* Initial address, must set address before use */
    r->tx_complete = 0;
    r->rx          = 0;
    r->err         = 0;
    r->skip_crc = 0;
    
    if(id==2){
        r->ibuf        = pkt_ibuf;
        r->rbuf        = pkt_buf;
        r->sbuf        = pkt_send_buf;
    } else if(id==1){
        r->ibuf        = pkt_ibuf2;
        r->rbuf        = pkt_buf2;
        r->sbuf        = pkt_send_buf2;
    }
#ifdef NINE_BIT_MODE
    r->state       = PKT_STATE_ADDRESS;   //Start with address in 9-bit mode
    RCSTAbits.ADDEN = 1;    /* Only allow address bytes to interrupt receiver */
#else
    r->state       = PKT_STATE_START;
#endif
    return;
}

uint16_t pkt_compute_cksum(uint8_t *p, uint8_t send, uint8_t offsetPacketLength)
{
    uint16_t len, crc, i;
        
    uint8_t *data = &p[0];
    uint8_t dataToCrc;
            
    if(!p){
        return 0;
    }
    
    len = (((p)[5+offsetPacketLength]<<8) | (p)[4+offsetPacketLength]) + 6;
    
    crc = 0xFFFF; /* Initial value */

    while (len--) {
        dataToCrc = *data++;
        if(send){
          if((dataToCrc == 0x7d) || (dataToCrc == 0x7e)){
              dataToCrc = *data++;
              dataToCrc = 0x70 | (dataToCrc & 0x0f);
          }
        }
        
        crc ^= dataToCrc;
        for (i=0; i<8; i++) {
            if (crc & 1)  crc = (crc >> 1) ^ 0x8408;
            else          crc = (crc >> 1);
        }
    }
    return crc;
}

/*
 * pkt_tx() - transmit the packet contained in the 'sbuf' field of
 * 'r'.  Don't return until the whole packet has been shifted out the
 * UART.
 */
void pkt_tx(RSTATE * r, uint8_t usartNumber){
    /* enable transmitter and possibly the receiver */     

    r->tx_complete  = 0;
    r->sidx = 1;
  
#ifdef NINE_BIT_MODE
    TXSTAbits.TX9D = 1;
#endif
        
    if(usartNumber == 2){
        IFS0bits.U1TXIF = 0;
        EnableIntU1TX; //EUSART transmit interrupt enable
    } else {
        IFS1bits.U2TXIF = 0;
        EnableIntU2TX; //EUSART transmit interrupt enable
    }
    
    /* start transmission by sending the first byte */
    r->tx_start(r->sbuf[0]);

    /* the rest of the transmission is interrupt driven */
    while(!r->tx_complete);
}


/*
 * randno - return a random number between 'lo' and 'hi'-1
 */
uint16_t randno(uint16_t lo, uint16_t hi)
{
    return lo + rand() % (hi - lo);
}


/*
 * pkt_send() - send a packet to the specified recipient using the
 * specified from address and flags byte. 'buf' points to the packet
 * data payload and 'len' contains the data length.
 */
int8_t pkt_send(RSTATE * r, uint8_t to, uint8_t from, uint8_t commandSequencer, uint8_t command, uint16_t len, uint8_t * buf)
{
    uint8_t offsetPacketLength = 0;
    uint16_t crc;
    
    uint8_t ck, i, leadin, plen;

    if(len + 8 > PKT_MAX_PLEN){
        return -1; /* error: data is too long for a packet */
    }

    i = 0;
#ifndef NINE_BIT_MODE
    r->sbuf[i++] = PKT_START_BYTE;
    leadin = i;
#endif
    r->sbuf[i++] = to;
    r->sbuf[i++] = from;  /* sender address */
    
    /* Command sequencer */
    if((commandSequencer == 0x7d) || (commandSequencer == 0x7e)){
        r->sbuf[i++] = 0x7d;
        r->sbuf[i++] = 0x50 | (commandSequencer & 0x0f);
        offsetPacketLength++;
    } else {
        r->sbuf[i++] = commandSequencer;
    }
    
    r->sbuf[i++] = command;     /* command number */
    r->sbuf[i++] = len & 0xFF;  /* data len byte 1 */
    r->sbuf[i++] = len >> 8;    /* data len byte 2 */

    if(len){
        /* copy data to be sent into buffer */
        while(len){           
            if((*buf == 0x7d) || (*buf == 0x7e)){
                r->sbuf[i++] = 0x7d;
                r->sbuf[i++] = 0x50 | (*buf & 0x0f);
            } else {
                r->sbuf[i++] = *buf;
            }
            ck += *buf++;
            len--;
        }
    }

    /* Start byte must be avoided */
    crc = pkt_compute_cksum(&r->sbuf[1], 1, offsetPacketLength);
            
     /* Verify first byte of CRC */
    if(((crc & 0xff) == 0x7d) || ((crc & 0xff) == 0x7e)){
        r->sbuf[i++] = 0x7d;
        r->sbuf[i++] = 0x50 | (crc & 0x0f);
    } else {
        r->sbuf[i++] = crc & 0xFF;  /* send checksum byte 1 */
    }
    
    /* Verify second byte of CRC */
    crc >>= 8;
    if(((crc & 0xff) == 0x7d) || ((crc & 0xff) == 0x7e)){
        r->sbuf[i++] = 0x7d;
        r->sbuf[i++] = 0x50 | (crc & 0x0f);
    } else {
        r->sbuf[i++] = crc;  /* send checksum byte 2 */
    }
        
    r->sbuf[i++] = PKT_START_BYTE;
  
    plen = i;  /* length of the whole packet */

    r->slen = plen;
 
    pkt_tx(r, r->myid);
    
    return 0;
}

/* send huge data */
int8_t pkt_send2(RSTATE * r, uint8_t to, uint8_t from, uint8_t commandSequencer, uint8_t command, uint16_t len, uint8_t * buf)
{
    uint8_t offsetPacketLength = 0;
    uint16_t crc, real_len;
    uint8_t skip_crc = 0;
    
    uint8_t ck, i, leadin, plen;

    if(len + 8 > PKT_MAX_PLEN){
        return -1; /* error: data is too long for a packet */
    }
    
//    if((len == 0x82) && (command == PKTF_RESP_READ_CARTDRIGE)){
//        real_len = len + 2;
//        skip_crc = 1;
//    } else {
//        real_len = len;
//    }
    real_len = len;

    i = 0;
#ifndef NINE_BIT_MODE
    r->sbuf[i++] = PKT_START_BYTE;
    leadin = i;
#endif
    r->sbuf[i++] = to;
    r->sbuf[i++] = from;  /* sender address */
    
    /* Command sequencer */
    if((commandSequencer == 0x7d) || (commandSequencer == 0x7e)){
        r->sbuf[i++] = 0x7d;
        r->sbuf[i++] = 0x50 | (commandSequencer & 0x0f);
        offsetPacketLength++;
    } else {
        r->sbuf[i++] = commandSequencer;
    }
    
    r->sbuf[i++] = command;     /* command number */
    r->sbuf[i++] = len & 0xFF;  /* data len byte 1 */
    r->sbuf[i++] = len >> 8;    /* data len byte 2 */

    if(real_len){
        /* copy data to be sent into buffer */
        while(real_len){
            if((*buf == 0x7d) || (*buf == 0x7e)){
                r->sbuf[i++] = 0x7d;
                r->sbuf[i++] = 0x50 | (*buf & 0x0f);
            } else {
                r->sbuf[i++] = *buf;
            }
            ck += *buf++;
            real_len--;
        }
    }

    /* Start byte must be avoided */
    if(!skip_crc){
        crc = pkt_compute_cksum(&r->sbuf[1], 1, offsetPacketLength);
    } else {
        crc = r->rbuf[i]<<8 | r->rbuf[i-1];
    }
            
    /* Verify first byte of CRC */
    if(((crc & 0xff) == 0x7d) || ((crc & 0xff) == 0x7e)){
        r->sbuf[i++] = 0x7d;
        r->sbuf[i++] = 0x50 | (crc & 0x0f);
    } else {
        r->sbuf[i++] = crc & 0xFF;  /* send checksum byte 1 */
    }
    
    /* Verify second byte of CRC */
    crc >>= 8;
    if(((crc & 0xff) == 0x7d) || ((crc & 0xff) == 0x7e)){
        r->sbuf[i++] = 0x7d;
        r->sbuf[i++] = 0x50 | (crc & 0x0f);
    } else {
        r->sbuf[i++] = crc;  /* send checksum byte 2 */
    }
    
    r->sbuf[i++] = PKT_START_BYTE;
  
    plen = i;  /* length of the whole packet */

    r->slen = plen;
 
    pkt_tx(r, r->myid);
    
    return 0;
}


/*
 * pkt_wait() - wait for a packet to arrive, or timeout after
 * 'timeout' milliseconds.  If a packet arrives, return 0.  If we
 * timeout instead, return -1.
 */
//int8_t pkt_wait(RSTATE * r, const uint16_t timeout){
//    ms_count = TickGet();
//    while(r->rx == 0){
//        if(((double)((TickGet() - ms_count)/TICKS_PER_MS)) >= timeout+1){
//            break;
//        }
//    }
//    
//    if(r->rx == 0){
//        return -1;
//    }
//
//    return 0;
//}


/*
 * pkt_wait_ack() - wait for an ACK packet to arrive or timeout after
 * 'timeout' ms.  If we receive an ACK packet, return 0.  If we
 * timeout, return -1.  If we receive a packet, but the ACK or ACK bit
 * was not set, return -2.
 */
//int8_t pkt_wait_ack(RSTATE * r, const uint16_t timeout){
//    int8_t rc;
//
//    rc = pkt_wait(r, timeout);
//    if(rc){
//        return -1;
//    }
//
//    if(PKT_ACK(r->rbuf)){
//        return 0;
//    }
//
//    return -2;
//}


/*
 * pkt_send_wait_ack() - send a packet as with 'pkt_send()', and wait for
 * an ACK response, as with 'pkt_wait_ack()'.  If the 'clear' flag is
 * non-zero, and it looks like we received a good ACK packet in
 * response, then reset the 'pkt_rx' flag to indicate that no more
 * packets are waiting, effectively throwing the ACK packet away since
 * it has served its purpose by this point.
 */
//int8_t pkt_send_wait_ack(RSTATE * r, uint8_t collision_detect, uint8_t to,
//                         uint8_t from, uint8_t flags, uint8_t * buf,
//                         uint8_t len, uint8_t tries, uint16_t timeout,
//                         uint8_t clear){
//    int8_t rc=0;
//    uint8_t i;
//
//    for(i=0; i<tries; i++){
//        r->rx = 0;
//        rc = pkt_send(r, collision_detect, to, from, flags, buf, len);
//        if(rc < 0){
//            return -1;
//        }
//
//        rc = pkt_wait_ack(r, timeout);
//        if(rc == 0) {
//            if(clear){
//                r->rx = 0;
//            }
//            return 0;
//        }
//    }
//
//    return -1;
//}



void pkt_byte_rx(RSTATE * r, uint8_t c){
    static uint8_t to;
    static uint16_t dlen, checksum;
    
    switch (r->state) {
        case PKT_STATE_START:  /* start state - 1 or more start bytes */
            if(c == PKT_START_BYTE){
                r->state = PKT_STATE_DEST;
            }
        break;
               
        case PKT_STATE_DEST:  /* expecting an address packet */
            
            /* After a packet abort it could happen that the packet end arrives so skip it */
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
                        
            to = c;
            //if(to == r->myid){
                r->ridx = 0;
                r->err = 0;
                r->rx  = 0;
                r->ibuf[r->ridx++] = c; /* destination address */
                r->cksum = c;
                r->state = PKT_STATE_SRC;
           // } else {
            //    pkt_abort(r);
           // }
        break;
        
        case PKT_STATE_SRC: /* expecting sender address */
         //   if(to == r->myid){
                r->ibuf[r->ridx++] = c; /* src address */
                r->cksum += c;
        //    }
            r->state = PKT_STATE_NUMCMD;
        break;
        
        case PKT_STATE_NUMCMD: /* expecting command sequence number */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
                        
        //    if (to == r->myid) {
                r->ibuf[r->ridx++] = c; /* command sequence number */
                r->cksum += c;
         //   }
            r->state = PKT_STATE_CMD;
        break;
        
        case PKT_STATE_CMD: /* command number */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
                        
        //    if (to == r->myid) {
                r->ibuf[r->ridx++] = c; /* command number */
                r->cksum += c;
        //    }
            r->state = PKT_STATE_LEN;
            
            /* Data length occupies DATAFIELD_LENGTH bytes */
            dlen = DATAFIELD_LENGTH;
        break;
        
        case PKT_STATE_LEN: /* expecting data length */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
                        
            r->ibuf[r->ridx++] = c; /* data length field */
            r->cksum += c;
            
            if(--dlen == 0){ /* that was the last of the data length */
                
                dlen = PKT_DLEN(r->ibuf);
                
                if(dlen == 0){ /* zero length packet, skip data state */
                    dlen = CHECKSUM_LENGTH;
                    r->state = PKT_STATE_CKSUM;
                } else { /* packet has 'dlen' bytes of data to follow + a cksum */
                    if(dlen + 4 > PKT_MAX_PLEN){
                        pkt_abort(r); /* packet too long for use */
                        break;
                    }
                    r->state = PKT_STATE_DATA;
                }
            }
            
        break;
        
        case PKT_STATE_DATA: /* expecting 'dlen' data bytes */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
                        
            r->ibuf[r->ridx++] = c; /* data byte */
            r->cksum += c;
            if(--dlen == 0){ /* that was the last of the data */
                dlen = CHECKSUM_LENGTH;            
                r->state = PKT_STATE_CKSUM;
            }
        break;
        
        case PKT_STATE_CKSUM: /* expecting a checksum */
                        
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
                        
            r->ibuf[r->ridx++] = c; /* checksum byte */
            r->secondChecksumByte = 1;
            
            if(--dlen == 0){ /* that was the last of the checksum data */                
                //if(to == r->myid){
                    
                    /* Return to last character */
                    r->ridx--;
                    
                    /* Calculate checksum */
                    r->cksum = pkt_compute_cksum(r->ibuf, 0, 0);
                    
                    checksum = PKT_CKSUM(r->ibuf);
                    
                    r->secondChecksumByte = 0;
                    
                    if(r->cksum != checksum){ /* compare checksums */
                        r->err = 1;
                    }
                //}

                r->state = PKT_STATE_END;
            }
        break;
        
        case PKT_STATE_END: /* expecting a packet end */   
            
            if(c == PKT_START_BYTE){
                pkt_recvd(r);
                LATBbits.LATB1 ^= 1;
            } else {
                pkt_abort(r);
            }
            
            r->state = PKT_STATE_START;
        break;
        
        default: /* unknown state - state machine error */
                        
            pkt_abort(r);
        break;
  }
}


void pkt_byte_rx2(RSTATE * r, uint8_t c){
    static uint8_t to;
    static uint16_t dlen, checksum;
    
    switch (r->state) {
        case PKT_STATE_START:  /* start state - 1 or more start bytes */
            if(c == PKT_START_BYTE){
                r->state = PKT_STATE_DEST;
            }
        break;
               
        case PKT_STATE_DEST:  /* expecting an address packet */

            /* After a packet abort it could happen that the packet end arrives so skip it */
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
                        
            to = c;

            //if(to == r->myid){
                r->ridx = 0;
                r->err = 0;
                r->rx  = 0;
                r->ibuf[r->ridx++] = c; /* destination address */
                r->cksum = c;
                r->state = PKT_STATE_SRC;
            //} else {
            //    pkt_abort(r);
            //}
        break;
        
        case PKT_STATE_SRC: /* expecting sender address */
            //if(to == r->myid){
                r->ibuf[r->ridx++] = c; /* src address */
                r->cksum += c;
            //}
            r->state = PKT_STATE_NUMCMD;
        break;
        
        case PKT_STATE_NUMCMD: /* expecting command sequence number */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
            
            //if (to == r->myid) {
                r->ibuf[r->ridx++] = c; /* command sequence number */
                r->cksum += c;
            //}
            r->state = PKT_STATE_CMD;
        break;
        
        case PKT_STATE_CMD: /* command number */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            //if (to == r->myid) {
                r->ibuf[r->ridx++] = c; /* command number */
                r->cksum += c;
            //}
            r->state = PKT_STATE_LEN;
            
            /* Data length ocuppies DATAFIELD_LENGTH bytes */
            dlen = DATAFIELD_LENGTH;
        break;
        
        case PKT_STATE_LEN: /* expecting data length */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
            
            r->ibuf[r->ridx++] = c; /* data length field */
            r->cksum += c;
            
            if(--dlen == 0){ /* that was the last of the data length */
                
                dlen = PKT_DLEN(r->ibuf);
                                
                if(dlen == 0){ /* zero length packet, skip data state */
                    dlen = CHECKSUM_LENGTH;
                    r->state = PKT_STATE_CKSUM;
                } else { /* packet has 'dlen' bytes of data to follow + a cksum */
                    if(dlen + 4 > PKT_MAX_PLEN){
                        pkt_abort(r); /* packet too long for use */
                        break;
                    }
                    r->state = PKT_STATE_DATA;
                }
            }
            
        break;
        
        case PKT_STATE_DATA: /* expecting 'dlen' data bytes */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
                        
            r->ibuf[r->ridx++] = c; /* data byte */
            r->cksum += c;
            if(--dlen == 0){ /* that was the last of the data */
                dlen = CHECKSUM_LENGTH;            
                r->state = PKT_STATE_CKSUM;
            }
        break;
        
        case PKT_STATE_CKSUM: /* expecting a checksum */
            
            if(c == PKT_START_BYTE){
                pkt_abort(r);
                break;
            }
            
            if(c == 0x7d){
                r->received_7e_7d = 1;
                break;
            }
            
            if(r->received_7e_7d){
                c = 0x70 | (0x0f & c);
                r->received_7e_7d = 0;
            }
                        
            r->ibuf[r->ridx++] = c; /* checksum byte */
            r->secondChecksumByte = 1;
            
            if(--dlen == 0){ /* that was the last of the checksum data */                
                //if(to == r->myid){
                    
                    /* Return to last character */
                    r->ridx--;
                    
                    /* Calculate checksum */
                    r->cksum = pkt_compute_cksum(r->ibuf, 0, 0);
                    
                    checksum = PKT_CKSUM(r->ibuf);
                    
                    r->secondChecksumByte = 0;
                    
                    if((r->cksum != checksum) && (!r->skip_crc)){ /* compare checksums */
                        r->err = 1;
                    }
                //}

                r->skip_crc = 0;
                r->state = PKT_STATE_END;
            }
        break;
        
        case PKT_STATE_END: /* expecting a packet end */   
            
            if(c == PKT_START_BYTE){
                pkt_recvd(r);
                LATBbits.LATB0 ^= 1;
            } else {
                pkt_abort(r);
            }
            
            r->state = PKT_STATE_START;
        break;
        
        default: /* unknown state - state machine error */
                        
            pkt_abort(r);
        break;
  }
}