/******************************************************************************/
/* System Level #define Macros                                                */
/******************************************************************************/

/* TODO Define system operating frequency */

/* Microcontroller MIPs (FCY) */
#define SYS_FREQ        60825600LL
#define FCY             SYS_FREQ/2

/******************************************************************************/
/* System Function Prototypes                                                 */
/******************************************************************************/

/* Custom oscillator configuration funtions, reset source evaluation
functions, and other non-peripheral microcontroller initialization functions
go here. */

#define delay_us(x)	__delay32(((x*FCY)/1000000L))	// delays x us
#define delay_ms(x)	__delay32(((x*FCY)/1000L))		// delays x ms

void ConfigureOscillator(void); /* Handles clock switching/osc initialization */

