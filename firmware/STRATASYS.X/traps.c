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

/******************************************************************************/
/* Trap Function Prototypes                                                   */
/******************************************************************************/

/* <Other function prototypes for debugging trap code may be inserted here> */

/* Use if INTCON2 ALTIVT=1 */
void __attribute__((interrupt,no_auto_psv)) _OscillatorFail(void);
void __attribute__((interrupt,no_auto_psv)) _AddressError(void);
void __attribute__((interrupt,no_auto_psv)) _StackError(void);
void __attribute__((interrupt,no_auto_psv)) _MathError(void);
void __attribute__((interrupt,no_auto_psv)) _SoftTrapError(void);

/* Use if INTCON2 ALTIVT=0 */
void __attribute__((interrupt,no_auto_psv)) _AltOscillatorFail(void);
void __attribute__((interrupt,no_auto_psv)) _AltAddressError(void);
void __attribute__((interrupt,no_auto_psv)) _AltStackError(void);
void __attribute__((interrupt,no_auto_psv)) _AltMathError(void);

/* Default interrupt handler */
void __attribute__((interrupt,no_auto_psv)) _DefaultInterrupt(void);

/******************************************************************************/
/* Trap Handling                                                              */
/*                                                                            */
/* These trap routines simply ensure that the device continuously loops       */
/* within each routine.  Users who actually experience one of these traps     */
/* can add code to handle the error.  Some basic examples for trap code,      */
/* including assembly routines that process trap sources, are available at    */
/* www.microchip.com/codeexamples                                             */
/******************************************************************************/

/* Primary (non-alternate) address error trap function declarations */
void __attribute__((interrupt,no_auto_psv)) _OscillatorFail(void)
{
        INTCON1bits.OSCFAIL = 0;        /* Clear the trap flag */
        
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        LATBbits.LATB1 ^= 1;
        delay_ms(250);
    }
}

void __attribute__((interrupt,no_auto_psv)) _AddressError(void)
{
        INTCON1bits.ADDRERR = 0;        /* Clear the trap flag */

    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        delay_ms(250);
    }
}
void __attribute__((interrupt,no_auto_psv)) _StackError(void)
{
        INTCON1bits.STKERR = 0;         /* Clear the trap flag */

    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        delay_ms(1000);
    }
}

void __attribute__((interrupt,no_auto_psv)) _MathError(void)
{
        INTCON1bits.MATHERR = 0;        /* Clear the trap flag */
        while (1);
}

void __attribute__((interrupt,no_auto_psv)) _SoftTrapError(void)
{
    if(INTCON3bits.NAE){
      INTCON3bits.NAE = 0;  //Clear the trap flag
    }
    if(INTCON3bits.DOOVR)
    {
      INTCON3bits.DOOVR = 0;  //Clear the trap flag
    }
    
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        delay_ms(500);
    }
}

/* Alternate address error trap function declarations */
void __attribute__((interrupt,no_auto_psv)) _AltOscillatorFail(void)
{
        INTCON1bits.OSCFAIL = 0;        /* Clear the trap flag */
        
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        LATBbits.LATB1 ^= 1;
        delay_ms(250);
    }
}

void __attribute__((interrupt,no_auto_psv)) _AltAddressError(void)
{
    INTCON1bits.ADDRERR = 0;        /* Clear the trap flag */
    
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        delay_ms(250);
    }
}

void __attribute__((interrupt,no_auto_psv)) _AltStackError(void)
{
    INTCON1bits.STKERR = 0;         /* Clear the trap flag */
    
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB0 ^= 1;
        delay_ms(1000);
    }
}

void __attribute__((interrupt,no_auto_psv)) _AltMathError(void)
{
        INTCON1bits.MATHERR = 0;        /* Clear the trap flag */
        while (1);
}

/******************************************************************************/
/* Default Interrupt Handler                                                  */
/*                                                                            */
/* This executes when an interrupt occurs for an interrupt source with an     */
/* improperly defined or undefined interrupt handling routine.                */
/******************************************************************************/
void __attribute__((interrupt,no_auto_psv)) _DefaultInterrupt(void)
{
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
    while(1){
        LATBbits.LATB1 ^= 1;
        delay_ms(500);
    }
}

