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
#include "cf_multiranger.h"
#include "cf_api_commander.h"
// #include "cf_param.h"
//=========================== defines =========================================

const uint8_t cmultiranger_path0[] = "multiranger";
// const uint8_t cmultiranger_payload[] = "TEST";
struct mutiranger_isClose_data cmultiranger_payload = {0, 0, 0, 0, 0, 0};

static const uint8_t dst_addr[] = {
        0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x93, 0xf3, 0x19, 0x83, 0x1f, 0xad, 0x99, 0xd6
};

// 51-6309     0x55, 0x17, 0x56, 0x86, 0x7f, 0xfc, 0xed, 0xf4
// T           0x93, 0xf3, 0x19, 0x83, 0x1f, 0xad, 0x99, 0xd6

#define PERIODIC_SENDING 0
#define PUSH_ENABLED 1

#define DEFAULT_VELOCITY 0.5
#define DEFAULT_HEIGHT 0.5

struct hoverPacket_s current_setpoint = {0, 0, 0, 0};

//=========================== variables =======================================

cmultiranger_vars_t cmultiranger_vars;
struct mutiranger_isClose_data cmultiranger_isClose_data; //received data

opentimers_id_t moveDistance_timerId;
opentimers_id_t sendSetpoint_timerId;
bool isMoving = false;
bool isFlying = false;

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

void multiranger_isClose_callback(struct mutiranger_isClose_data *data);

void vel_land();
void vel_take_off();
void periodic_send_setpoint(opentimers_id_t id);
//void cmultiranger_reset_position_estimator();

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
    cmultiranger_vars.period           = 1; //skip check
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

#if 1

    sendSetpoint_timerId = opentimers_create(TIMER_GENERAL_PURPOSE, TASKPRIO_COAP);
    opentimers_scheduleIn(
        sendSetpoint_timerId,
        100,
        TIME_MS,
        TIMER_PERIODIC,
        periodic_send_setpoint
    );

#endif

    // Move Distance Timer
    moveDistance_timerId = opentimers_create(TIMER_GENERAL_PURPOSE, TASKPRIO_COAP);

    // register the callback
    multiranger_set_close_threshold(300);
    multiranger_set_isClose_callback(&multiranger_isClose_callback);
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

            //Get Multi-Ranger isClose data
            cmultiranger_isClose_data = *((struct mutiranger_isClose_data *) msg->payload);

            if (cmultiranger_isClose_data.up) {
                leds_error_on();
            } else {
                leds_error_off();
            }

            // // reset packet payload
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
    memcpy(&pkt->payload[0], &cmultiranger_payload, sizeof(cmultiranger_payload));

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
    }
}

void cmultiranger_sendDone(OpenQueueEntry_t *msg, owerror_t error) {
    // free the packet buffer entry
    openqueue_freePacketBuffer(msg);

    // allow to send next cmultiranger packet
    cmultiranger_vars.busySendingCmultiranger = FALSE;
}

// =============================================================

void periodic_send_setpoint(opentimers_id_t id) {
    if (!isFlying) {
        return;
    }

    // Send CoAP message
    send_hover_setpoint(current_setpoint.vx, current_setpoint.vy, current_setpoint.yawrate, current_setpoint.zDistance);
}

void set_setpoint(float vx, float vy, float yawrate, float zDistance) {
    current_setpoint.vx = vx;
    current_setpoint.vy = vy;
    current_setpoint.yawrate = yawrate;
    current_setpoint.zDistance = zDistance;
}

// Multiranger IsClose Change Callback
// Receive the data from the Multiranger bsp
void multiranger_isClose_callback(struct mutiranger_isClose_data *data) {

#if PUSH_ENABLED

    // Takeoff or Landing?
    if (isFlying && data->up) {
        vel_land();
    }else if (!isFlying && data->up) {
        vel_take_off();
    }

    if(!isFlying){
        return;
    }

    // Change?
    float velocity = DEFAULT_VELOCITY;
    float velocity_x = 0.0;
    float velocity_y = 0.0;

    // Direction?
    if (data->front) {
        velocity_x -= velocity;
    }
    if (data->back) {
        velocity_x += velocity;
    }

    if (data->left) {
        velocity_y -= velocity;
    }
    if (data->right) {
        velocity_y += velocity;
    }

    set_setpoint(velocity_x, velocity_y, 0, DEFAULT_HEIGHT);

#endif
//Push Enabled

    // === Send the velocity to other Crazyflie ===

    // // Update the payload
    // cmultiranger_payload = *data;

    // // Send CoAP message
    // cmultiranger_task_cb();
    // leds_error_toggle();
}

// =============================================================

void vel_take_off_done(opentimers_id_t id) {
    isMoving = false;
}

void vel_take_off() {

    if (isFlying || isMoving) {
        return;
    }

    float time_ms = DEFAULT_HEIGHT / DEFAULT_VELOCITY * 1000;

    isFlying = true;
    isMoving = true;
    set_setpoint(0, 0, 0, DEFAULT_HEIGHT);

    opentimers_scheduleIn(
        moveDistance_timerId,
        time_ms,
        TIME_MS,
        TIMER_ONESHOT,
        vel_take_off_done
    );
}

void vel_land_done(opentimers_id_t id) {
    send_notify_setpoints_stop(0);
    send_stop_setpoint();
    isFlying = false;
    isMoving = false;
}

void vel_land() {
    if (!isFlying || isMoving) {
        return;
    }

    // current height
    float height_m = multiranger_get_down_mm() / 1000.0;
    float time_ms = height_m / DEFAULT_VELOCITY * 1000;

    isMoving = true;
    set_setpoint(0, 0, 0, 0);
    
    opentimers_scheduleIn(
        moveDistance_timerId,
        time_ms,
        TIME_MS,
        TIMER_ONESHOT,
        vel_land_done
    );
}

// =============================================================

//void cmultiranger_reset_position_estimator()
//{
//  uint8_t value = 1;
//  param_set(index_kalman_resetEstimation, &value, 1);

//  value = 0;
//  param_set(index_kalman_resetEstimation, &value, 1);
//}

//【Move】

// void moveDistance_xy(float distance_x_m, float distance_y_m){
    
//     if (!isFlying || isMoving ) {
//         return;
//     }

//     // Calculate the time to move
//     float distance = sqrt(distance_x_m * distance_x_m + distance_y_m * distance_y_m);
//     uint16_t time_ms = distance / DEFAULT_VELOCITY * 1000;

//     // Calculate the velocity
//     float velocity_x = distance_x_m / distance * DEFAULT_VELOCITY;
//     float velocity_y = distance_y_m / distance * DEFAULT_VELOCITY;

//     // Send the velocity to the Crazyflie
//     isMoving = true;
//     send_hover_setpoint(velocity_x, velocity_y, 0, DEFAULT_HEIGHT);

//     // Start the timer
//     opentimers_scheduleIn(
//         moveDistance_timerId,
//         time_ms,
//         TIME_MS,
//         TIMER_ONESHOT,
//         moveDistance_xy_done
//     );
// }

// void moveDistance_xy_done() {
//     send_hover_setpoint(0, 0, 0, DEFAULT_HEIGHT);
//     isMoving = false;
// }

#endif /* OPENWSN_CMULTIRANGER_C */
