/**
\brief This project runs the full OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "config.h"
#include "board.h"
#include "scheduler.h"
#include "openstack.h"
#include "opendefs.h"

#if OPENWSN_CCRAZYFLIE_C
#include "cf_crazyflie.h"
#endif

int mote_main(void) {
   
   // initialize
   board_init();

#if OPENWSN_CCRAZYFLIE_C
   crazyflieInit();
#endif

   scheduler_init();
   openstack_init();
   
   // start
   scheduler_start();
   return 0; // this line should never be reached
}

