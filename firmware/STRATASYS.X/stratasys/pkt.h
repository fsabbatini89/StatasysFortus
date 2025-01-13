#ifndef __pkt_h__
#define __pkt_h__

#include <stdint.h>
#include <string.h>

#include <uart.h>

#define PKT_TRACE 0 /* 1 = turn node into an RS485 line monitor */

/* states for the packet reception state machine */
enum {
    PKT_STATE_START,
    PKT_STATE_DEST,
    PKT_STATE_SRC,
    PKT_STATE_NUMCMD,
    PKT_STATE_CMD,
    PKT_STATE_LEN,
    PKT_STATE_DATA,
    PKT_STATE_CKSUM,
    PKT_STATE_END
};

typedef struct rstate {
    uint8_t * ibuf;                 /* pointer to active receive buffer */
    uint8_t * sbuf;                 /* pointer to transmit buffer */
    uint8_t * rbuf;                 /* pointer to the buffer holding the last rx'd packet */
    volatile uint8_t ridx;          /* index into rx buf */
    volatile uint8_t sidx;          /* index into tx buf */
    volatile uint8_t slen;          /* length ot the packet being transmitted */
    volatile uint8_t state;         /* packet rx state */
    volatile uint16_t cksum;        /* packet checksum */
    uint8_t myid;                   /* our network address */
    volatile uint8_t tx_complete;   /* transmit complete flag */
    volatile uint8_t rx;            /* packet received flag */
    volatile uint8_t err;           /* packet error flag */
    volatile uint8_t received_7e_7d;/* 7e or 7d character received */
    volatile uint8_t secondChecksumByte; /* Second checksum byte received */
    volatile uint8_t skip_crc;      /* skip_crc calculation */
    void (*tx_start)(uint8_t c);
} RSTATE;


#define PKT_OVERHEAD  8 /*10 bytes of overhead for each packet */
#define PKT_MAX_PLEN 160 /* max packet length */
#define PKT_MAX_DLEN (PKT_MAX_PLEN - PKT_OVERHEAD) /* max packet data length */

/* initial settings */
#define PKT_N_LEADIN    0     /* no. bytes of leading */
#define PKT_LEADIN_BYTE 0x0
#define PKT_START_BYTE  0x7e

/* Packet commands */
#define PKTF_CMD_ACK    0x02
#define PKTF_CMD_WRITE_CARTDRIGE    0x57
#define PKTF_CMD_ASK_WRITE_DONE_CARTDRIGE     0x52

/* Packet response */
#define PKTF_RESP_WRITE_CARTDRIGE    0x03
#define PKTF_RESP_ASK_WRITE_DONE_CARTDRIGE    0x02
#define PKTF_RESP_WRITTEN_CARTDRIGE    0x04
#define PKTF_RESP_READ_CARTDRIGE     0x56


/* commands */
#define CMD_SETID       0x01
#define CMD_RESET       0x02

/* commands length */
#define CMD_SETID_LENGTH    0x01

/* data length field length*/
#define DATAFIELD_LENGTH    0x02

/* checksum length*/
#define CHECKSUM_LENGTH    0x02

/* accessor macros to get at packet fields */
#define PKT_DST(x)      ((x)[0])
#define PKT_SRC(x)      ((x)[1])
#define PKT_NUMCMD(x)   ((x)[2])
#define PKT_COMMAND(x)  ((x)[3])
#define PKT_DLEN(x)     (((x)[5]<<8) | (x)[4])
#define PKT_DATA(x)     (&((x)[6]))
#define PKT_LEN(x)      (PKT_DLEN(x) + 6)
#define PKT_CKSUM(x)    (((x)[PKT_LEN(x) + 1]<<8) | (x)[PKT_LEN(x)])

extern RSTATE * i1r; /* interface 1 ROBIN state */
extern RSTATE * i2r; /* interface 1 ROBIN state */

void pkt_init(RSTATE * r, uint8_t id);
int8_t pkt_send(RSTATE * r, uint8_t to, uint8_t from, uint8_t commandSequencer, uint8_t command, uint16_t len, uint8_t * buf);
int8_t pkt_send2(RSTATE * r, uint8_t to, uint8_t from, uint8_t commandSequencer, uint8_t command, uint16_t len, uint8_t * buf);
uint16_t pkt_compute_cksum(uint8_t *p, uint8_t send, uint8_t offsetPacketLength);
int8_t pkt_wait(RSTATE *r, const uint16_t timeout);
int8_t pkt_wait_ack(RSTATE *r, uint16_t timeout);
int8_t pkt_send_wait_ack(RSTATE *r, uint8_t collision_detect, uint8_t to, uint8_t from,
                         uint8_t flags, uint8_t *buf, uint8_t len, 
                         uint8_t tries, uint16_t timeout, uint8_t clear);

/*
 * pkt_ack() - inline function to send an empty ACK response to the
 * sender of the packet pointed to by 'p'.
 */
//static inline int8_t pkt_ack(RSTATE * r, uint8_t * p){
//    return pkt_send(r, 0, PKT_SRC(p), r->myid, PKTF_ACK, 0, 0);
//}

#if !PKT_TRACE
//uint32_t new_baud = 0;
static inline void pkt_recvd(RSTATE * r){
    
//    uint16_t len = PKT_LEN(r->ibuf);
//    if((PKT_DLEN(r->ibuf) == 0x82) && (PKT_COMMAND(r->ibuf) == PKTF_RESP_READ_CARTDRIGE)){
//        len += 2;
//    }
    
    memcpy(r->rbuf, r->ibuf, PKT_MAX_PLEN);
    r->rx = 1;
    //r->state = PKT_STATE_START;
}

static inline void pkt_abort(RSTATE * r){
#ifdef NINE_BIT_MODE
    r->state = PKT_STATE_ADDRESS;   //Start with address in 9-bit mode
    RCSTAbits.ADDEN = 1;    /* Only allow address bytes to interrupt receiver */
#else
    r->state = PKT_STATE_START;
#endif
}
#endif

void pkt_byte_rx(RSTATE * r, uint8_t c);
void pkt_byte_rx2(RSTATE * r, uint8_t c);

#endif