#include "config.h"

#if OPENWSN_CMULTIRANGER_C

#include "opendefs.h"
#include "cmultiranger.h"
#include "coap.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "schedule.h"
#include "icmpv6rpl.h"
#include "idmanager.h"

#include "leds.h"

//=========================== defines =========================================

const uint8_t cmultiranger_path0[] = "multiranger";
const uint8_t cmultiranger_payload[] = "TEST";
static const uint8_t dst_addr[] = {
        0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x93, 0xf3, 0x19, 0x83, 0x1f, 0xad, 0x99, 0xd6
};

// 51-6309     0x55, 0x17, 0x56, 0x86, 0x7f, 0xfc, 0xed, 0xf4
// T           0x93, 0xf3, 0x19, 0x83, 0x1f, 0xad, 0x99, 0xd6

#define PERIODIC_SENDING 0

//=========================== variables =======================================

cmultiranger_vars_t cmultiranger_vars;

//=========================== prototypes ======================================

owerror_t cmultiranger_receive(
        OpenQueueEntry_t *msg,
        coap_header_iht *coap_header,
        coap_option_iht *coap_incomingOptions,
        coap_option_iht *coap_outgoingOptions,
        uint8_t *coap_outgoingOptionsLen);

void cmultiranger_timer_cb(opentimers_id_t id);

void cmultiranger_task_cb(void);

void cmultiranger_sendDone(OpenQueueEntry_t *msg, owerror_t error);

//=========================== public ==========================================

void cmultiranger_init(void) {

    // register to OpenCoAP module
    cmultiranger_vars.desc.path0len = sizeof(cmultiranger_path0) - 1;
    cmultiranger_vars.desc.path0val = (uint8_t * )(&cmultiranger_path0);
    cmultiranger_vars.desc.path1len = 0;
    cmultiranger_vars.desc.path1val = NULL;
    cmultiranger_vars.desc.componentID = COMPONENT_CMULTIRANGER;
    cmultiranger_vars.desc.securityContext = NULL;
    cmultiranger_vars.desc.discoverable = TRUE;
    cmultiranger_vars.desc.callbackRx = &cmultiranger_receive;
    cmultiranger_vars.desc.callbackSendDone = &cmultiranger_sendDone;
    coap_register(&cmultiranger_vars.desc);

    //start a periodic timer
    //comment : not running by default
#if PERIODIC_SENDING
    cmultiranger_vars.period           = 500;

    cmultiranger_vars.timerId          = opentimers_create(TIMER_GENERAL_PURPOSE, TASKPRIO_COAP);
    opentimers_scheduleIn(
        cmultiranger_vars.timerId,
        cmultiranger_vars.period,
        TIME_MS,
        TIMER_PERIODIC,
        cmultiranger_timer_cb
    );
#endif
}

//=========================== private =========================================

owerror_t cmultiranger_receive(
        OpenQueueEntry_t *msg,
        coap_header_iht *coap_header,
        coap_option_iht *coap_incomingOptions,
        coap_option_iht *coap_outgoingOptions,
        uint8_t *coap_outgoingOptionsLen
) {
    owerror_t outcome;

    leds_error_toggle();

    switch (coap_header->Code) {

        case COAP_CODE_REQ_GET:

            // reset packet payload
            msg->payload = &(msg->packet[127]);
            msg->length = 0;

            // add CoAP payload
            if (packetfunctions_reserveHeader(&msg, 2) == E_FAIL) {
                openqueue_freePacketBuffer(msg);
                return E_FAIL;
            }
            // return as big endian
            msg->payload[0] = (uint8_t)(cmultiranger_vars.period >> 8);
            msg->payload[1] = (uint8_t)(cmultiranger_vars.period & 0xff);

            // set the CoAP header
            coap_header->Code = COAP_CODE_RESP_CONTENT;

            outcome = E_SUCCESS;
            break;

        case COAP_CODE_REQ_PUT:

            if (msg->length != 2) {
                outcome = E_FAIL;
                coap_header->Code = COAP_CODE_RESP_BADREQ;
            }

            // read the new period
            cmultiranger_vars.period = 0;
            cmultiranger_vars.period |= (msg->payload[0] << 8);
            cmultiranger_vars.period |= msg->payload[1];

            /*
            // stop and start again only if period > 0
            opentimers_cancel(cmultiranger_vars.timerId);

            if(cmultiranger_vars.period > 0) {
                  opentimers_scheduleIn(
                      cmultiranger_vars.timerId,
                      cmultiranger_vars.period,
                      TIME_MS,
                      TIMER_PERIODIC,
                      cmultiranger_timer_cb
                  );
            }
            */

            // reset packet payload
            msg->payload = &(msg->packet[127]);
            msg->length = 0;

            // set the CoAP header
            coap_header->Code = COAP_CODE_RESP_CHANGED;

            outcome = E_SUCCESS;
            break;

        default:
            outcome = E_FAIL;
            break;
    }

    return outcome;
}

void cmultiranger_timer_cb(opentimers_id_t id) {
    // calling the task directly as the timer_cb function is executed in
    // task mode by opentimer already
    cmultiranger_task_cb();
}

void cmultiranger_task_cb(void) {
    OpenQueueEntry_t *pkt;
    owerror_t outcome;
    coap_option_iht options[2];
    uint8_t medType;

    open_addr_t parentNeighbor;
    bool foundNeighbor;

    // don't run if not synch
    if (ieee154e_isSynch() == FALSE) {
        return;
    }

    // don't run on dagroot
    if (idmanager_getIsDAGroot()) {
        opentimers_destroy(cmultiranger_vars.timerId);
        return;
    }

    foundNeighbor = icmpv6rpl_getPreferredParentEui64(&parentNeighbor);
    if (foundNeighbor == FALSE) {
        return;
    }

    if (schedule_hasNegotiatedCellToNeighbor(&parentNeighbor, CELLTYPE_TX) == FALSE) {
        return;
    }

    if (cmultiranger_vars.busySendingCmultiranger == TRUE) {
        // don't continue if I'm still sending a previous cmultiranger
        return;
    }

    if (cmultiranger_vars.period == 0) {
        // stop the periodic timer
        opentimers_cancel(cmultiranger_vars.timerId);
        return;
    }

    // if you get here, send a packet

    // get a packet
    pkt = openqueue_getFreePacketBuffer(COMPONENT_CMULTIRANGER);
    if (pkt == NULL) {
        LOG_ERROR(COMPONENT_CMULTIRANGER, ERR_NO_FREE_PACKET_BUFFER, (errorparameter_t) 0, (errorparameter_t) 0);
        return;
    }

    // take ownership over that packet
    pkt->creator = COMPONENT_CMULTIRANGER;
    pkt->owner = COMPONENT_CMULTIRANGER;

    //The contents of the message are written in reverse order : the payload first

    // add payload
    if (packetfunctions_reserveHeader(&pkt, sizeof(cmultiranger_payload) - 1) == E_FAIL) {
        openqueue_freePacketBuffer(pkt);
        return;
    }
    memcpy(&pkt->payload[0], cmultiranger_payload, sizeof(cmultiranger_payload) - 1);

    // location-path option
    options[0].type = COAP_OPTION_NUM_URIPATH;
    options[0].length = sizeof(cmultiranger_path0) - 1;
    options[0].pValue = (uint8_t *) cmultiranger_path0;


    // content-type option
    medType = COAP_MEDTYPE_APPOCTETSTREAM;
    options[1].type = COAP_OPTION_NUM_CONTENTFORMAT;
    options[1].length = 1;
    options[1].pValue = &medType;

    // metadata
    pkt->l4_destination_port = WKP_UDP_COAP;
    pkt->l3_destinationAdd.type = ADDR_128B;
    memcpy(&pkt->l3_destinationAdd.addr_128b[0], &dst_addr, 16);

    // send
    outcome = coap_send(
            pkt,
            COAP_TYPE_NON,
            COAP_CODE_REQ_PUT,
            1, // token len
            options,
            2, // options len
            &cmultiranger_vars.desc
    );

    // avoid overflowing the queue if fails
    if (outcome == E_FAIL) {
        openqueue_freePacketBuffer(pkt);
    } else {
        cmultiranger_vars.busySendingCmultiranger = FALSE;
        leds_error_toggle();
    }
}

void cmultiranger_sendDone(OpenQueueEntry_t *msg, owerror_t error) {
    // free the packet buffer entry
    openqueue_freePacketBuffer(msg);

    // allow to send next cmultiranger packet
    cmultiranger_vars.busySendingCmultiranger = FALSE;
}

#endif /* OPENWSN_CMULTIRANGER_C */
