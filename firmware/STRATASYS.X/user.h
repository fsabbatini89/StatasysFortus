#ifndef __user_h__
#define __user_h__

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/

/* TODO Application specific user parameters used in user.c may go here */

/******************************************************************************/
/* User Function Prototypes                                                   */
/******************************************************************************/

/* TODO User level functions prototypes (i.e. InitApp) go here */
#include "stratasys/pkt.h"

extern RSTATE host; 
extern RSTATE cartdrige; 
extern RSTATE *r1;
extern RSTATE *r2;

void InitApp(void); /* I/O and Peripheral Initialization */

#endif