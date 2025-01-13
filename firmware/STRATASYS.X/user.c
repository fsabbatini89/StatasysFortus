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

#include <stdint.h>          /* For uint32_t definition                       */
#include <stdbool.h>         /* For true/false definition                     */

#include <uart.h>

#include "user.h"            /* variables/params used by user.c               */

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

/* TODO Initialize User Ports/Peripherals/Project here */

void writeUART1Wrapper(uint8_t data)
{
    U1TXREG = data;
}

void writeUART2Wrapper(uint8_t data)
{
    U2TXREG = data;
}

void InitApp(void)
{
    /* Setup analog functionality and port direction */
    TRISBbits.TRISB0 = 0;   //Status LED
    TRISBbits.TRISB1 = 0;   //Write OP LED
    
    LATBbits.LATB0 = 1;
    LATBbits.LATB1 = 0;
    
    RPINR18 = 0b00101100; //RPI44 UART1 RX
    _RP42R =  0b00000001; //RP42  UART1 TX
    
    RPINR19 = 0b00101101; //RPI45 UART2 RX
    _RP43R =  0b00000011; //RP43  UART2 TX
    
    /* Initialize peripherals */
    
    /* Configure receive and transmit interrupt */
    ConfigIntUART1(UART_RX_INT_EN & UART_RX_INT_PR6 & UART_TX_INT_EN & UART_TX_INT_PR2);
    ConfigIntUART2(UART_RX_INT_EN & UART_RX_INT_PR6 & UART_TX_INT_EN & UART_TX_INT_PR2);
    
    OpenUART1(UART_MODE_SIMPLEX & UART_IrDA_DISABLE & UART_UEN_00 & UART_EN & UART_IDLE_CON & UART_EN_WAKE & UART_DIS_LOOPBACK & UART_DIS_ABAUD & UART_NO_PAR_8BIT & UART_1STOPBIT, 
            UART_TX_ENABLE & UART_INT_TX & UART_ADR_DETECT_DIS & UART_RX_OVERRUN_CLEAR & UART_INT_RX_CHAR & UART_BRGH_FOUR, 65);
    
    OpenUART2(UART_MODE_SIMPLEX & UART_IrDA_DISABLE & UART_UEN_00 & UART_EN & UART_IDLE_CON & UART_EN_WAKE & UART_DIS_LOOPBACK & UART_DIS_ABAUD & UART_NO_PAR_8BIT & UART_1STOPBIT, 
            UART_TX_ENABLE & UART_INT_TX & UART_ADR_DETECT_DIS & UART_RX_OVERRUN_CLEAR & UART_INT_RX_CHAR & UART_BRGH_FOUR, 65);
    
    i1r = r1;
    pkt_init(r1, 2);
    r1->tx_start = writeUART1Wrapper;
    
    i2r = r2;
    pkt_init(r2, 1);
    r2->tx_start = writeUART2Wrapper;
}

