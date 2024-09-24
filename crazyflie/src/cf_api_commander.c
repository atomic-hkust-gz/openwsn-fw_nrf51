/**
 * 
 * @file cf_api_commander.c
 * @brief The Generic setpoint commander API for Crazyflie 2.X.
 *    this API is migrated from the 
 *      - cflib/crazyflie/commander.py
 *      - cflib/positioning/motion_commander.py
 *    this API can be used for Crazyflie 2.X self-control via the nRF51822 on-board chip. (now for the OpenWSN project)
 * @author Lan HUANG (YelloooBlue@outlook.com)
 * @date Sep 2024
 *
 */

/*
 * Coordinate System:
 *
 *              ^ X+
 *              |
 *              |
 *              |
 *    Y+ <------+
 *             (0,0)
 *
 */

#include <string.h>
#include <stdbool.h>

// crazyflie bsp
#include "cf_api_commander.h"
// #include "cf_param.h"
#include "cf_ctrp.h"

//=========================== defines =========================================

#define DEFAULT_TAKEOFF_HEIGHT 0.5

//=========================== variables =======================================

bool is_flying = false;

//=========================== prototypes ======================================

void _generic_setpoint_sendCommand(const enum packet_type command_type, void *values, uint8_t size);
void _generic_setpoint_sendMetaCommand(const enum metaCommand_e command_type, void *values, uint8_t size);

//=========================== public ==========================================

// 【Stop】
void send_notify_setpoints_stop(uint32_t remainValidMillisecs)
{
  /*
    Sends a packet so that the priority of the current setpoint to the lowest non-disabled value,
    so any new setpoint regardless of source will overwrite it.
  */
  struct notifySetpointsStopPacket notifySetpointsStopPacket;
  notifySetpointsStopPacket.remainValidMillisecs = remainValidMillisecs;

  // Port 7, Channel 1, Command 0
  _generic_setpoint_sendMetaCommand(metaNotifySetpointsStop, &notifySetpointsStopPacket, sizeof(notifySetpointsStopPacket));
}

void send_stop_setpoint()
{
  _generic_setpoint_sendCommand(stopType, NULL, 0);// Port 7, Channel 0, Command 0
}

// 【Setpoint】
void send_hover_setpoint(float vx, float vy, float yawrate, float zDistance)
{
  struct hoverPacket_s hoverPacket;
  hoverPacket.vx = vx;
  hoverPacket.vy = vy;
  hoverPacket.yawrate = yawrate;
  hoverPacket.zDistance = zDistance;

  _generic_setpoint_sendCommand(hoverType, &hoverPacket, sizeof(hoverPacket));
}

//=========================== private =========================================

// ===== CTRL API =====

// Set-Setpoint channel
void _generic_setpoint_sendCommand(const enum packet_type command_type, void *values, uint8_t size)
{
  uint8_t data[CRTP_MAX_DATA_SIZE];
  data[0] = command_type;
  memcpy(data + 1, values, size);
  data[size + 1] = 0;

  CTRPSend(data, size + 1, CRTP_PORT_SETPOINT_GENERIC, SET_SETPOINT_CHANNEL); // ctrp_data_len = values_len + command_type_len
}

// Meta-Command channel
void _generic_setpoint_sendMetaCommand(const enum metaCommand_e command_type, void *values, uint8_t size)
{
  uint8_t data[CRTP_MAX_DATA_SIZE];
  data[0] = command_type;
  memcpy(data + 1, values, size);
  data[size + 1] = 0;

  CTRPSend(data, size + 1, CRTP_PORT_SETPOINT_GENERIC, META_COMMAND_CHANNEL); // ctrp_data_len = values_len + command_type_len
}

//=========================== interrup handlers ===============================


// ===== Deprecated =====

// void reset_position_estimator()
// {
//   uint8_t value = 1;
//   param_set(index_kalman_resetEstimation, &value, 1);

//   value = 0;
//   param_set(index_kalman_resetEstimation, &value, 1);
// }