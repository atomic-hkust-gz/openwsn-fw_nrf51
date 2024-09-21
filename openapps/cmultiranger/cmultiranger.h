#ifndef __CMULTIRANGER_H
#define __CMULTIRANGER_H

/**
\addtogroup AppUdp
\{
\addtogroup cmultiranger
\{
*/

#include "config.h"
#include "coap.h"
#include "opentimers.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
    coap_resource_desc_t desc;
    opentimers_id_t timerId;
    uint16_t period;   ///< inter-packet period (in ms)
    bool busySendingCmultiranger;   ///< TRUE when busy sending cmultiranger packet
} cmultiranger_vars_t;

//=========================== prototypes ======================================

void cmultiranger_init(void);

/**
\}
\}
*/

#endif

