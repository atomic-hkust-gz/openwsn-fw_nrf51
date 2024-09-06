/**
\brief This is a program which provide Crazyflie boot and basic operations.

\author Lan HUANG <yelloooblue@outlook.com>, June 2024

 * HighLevel Coordinate System:
 *
 *              ^ Y+
 *              |
 *              |
 *              |
 *    X+ <------+
 *             (0,0)
 *

*/

/*
  IF WITH BOOTLOADER !
  
  1. Softdevice
  Options -> Configuration=Debug -> Debug -> Loader -> Addtional Load File[0]='s130_nrf51_2.0.1_softdevice.hex'

  2. Placement
  Options -> Configuration=Common -> Code -> Linker -> Section Placement Macros=
  '
  FLASH_START=0x1B000
  SRAM_START=0x20001EB0
  '
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// bsp modules required
#include "board.h"
#include "sctimer.h"
#include "leds.h"

// cf
#include "cf_crazyflie.h"
#include "cf_pm.h"
#include "cf_systick.h"
#include "cf_api_commander_high_level.h"
#include "cf_multiranger.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void mainloop();
void _east_go_to(float x, float y, float z);

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void)
{
    board_init();    // initialize the board
    crazyflieInit(); // Crazyflie init

    while (1)
    {
        mainloop();
    }
}

//=========================== callbacks =======================================

int tick;
bool enHighLevel = false;

// make sure onecall
bool a = true;
bool b = true;
bool c = true;
bool d = true;
bool e = true;

void mainloop()
{
    tick = systickGetTick(); // unsigned int

    if (tick == 5000 && !enHighLevel)
    {
        high_level_enable();
        enHighLevel = true;
        leds_all_on();
        mutiranger_init();
    }

    if (mutiranger_up_isClose())
    {
        leds_all_on();
    }else{
        leds_all_off();
    }

    // if (tick == 10000 && a)
    //{
    //    high_level_takeoff(0.5, 1.0, 0.0);
    //    a = false;
    // }

    // if (tick == 11000 && b)
    //{
    //     _east_go_to(0.5, 0.0, 0.0);
    //     b = false;
    // }

    // if (tick == 12000 && c)
    //{
    //     _east_go_to(0.0, 0.5, 0.0);
    //     c = false;
    // }

    // if (tick == 13000 && d)
    //{
    //     _east_go_to(-0.5, -0.5, 0.0);
    //     d = false;
    // }

    // if (tick == 14000 && e)
    //{
    //     high_level_land(0.0, 1.0, 0.0);
    //     e = false;
    // }

    if (tick >= 17000 && tick < 23000)
    {
        crazyflieEmergencyStop();
        leds_all_off();
        crazyflieShutdown();
    }

    crazyflieHandle();
}

//=========================== private =========================================

void _east_go_to(float x, float y, float z)
{
    float distance = sqrt(x * x + y * y + z * z);
    float duration = distance / CF_HL_MOVE_VEL;

    high_level_goto(x, y, z, 0.0, duration, true);
}