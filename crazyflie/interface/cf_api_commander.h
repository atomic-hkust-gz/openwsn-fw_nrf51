/**
 * 
 * @file cf_api_commander.h
 * @brief The Generic setpoint commander API for Crazyflie 2.X.
 *    this API is migrated from the 
 *      - cflib/crazyflie/commander.py
 *      - cflib/positioning/motion_commander.py
 *    this API can be used for Crazyflie 2.X self-control via the nRF51822 on-board chip. (now for the OpenWSN project)
 * @author Lan HUANG (YelloooBlue@outlook.com)
 * @date Sep 2024
 *
 */

#ifndef __CF_API_COMMANDER__
#define __CF_API_COMMANDER__

#include <stdint.h>
#include <stdbool.h>

//=========================== define ==========================================

#define CF_MOVE_VEL 0.5 // Crazyflie move velocity (m/s)

//=========================== typedef =========================================

// CTRP Channal
enum crtpSetpointGenericChannel {
  SET_SETPOINT_CHANNEL = 0,
  META_COMMAND_CHANNEL = 1,
};

// Command types
enum packet_type {
  stopType          = 0,
  velocityWorldType = 1,
  zDistanceType     = 2,
  cppmEmuType       = 3,
  altHoldType       = 4,
  hoverType         = 5,
  fullStateType     = 6,
  positionType      = 7,
};

//【Stop】

// Meta command types
enum metaCommand_e {
  metaNotifySetpointsStop = 0,
  nMetaCommands,
};

// Notify the Crazyflie to stop the current setpoints
struct notifySetpointsStopPacket {
  uint32_t remainValidMillisecs;
} __attribute__((packed));

//【Setpoint】

// Set the Crazyflie absolute height and velocity in the body coordinate system
struct hoverPacket_s {
  float vx;           // m/s in the body frame of reference
  float vy;           // ...
  float yawrate;      // deg/s
  float zDistance;    // m in the world frame of reference
} __attribute__((packed));

//=========================== variables =======================================

//=========================== prototypes ======================================

// void send_setpoint(float roll, float pitch, float yawrate, float thrust);
void send_notify_setpoints_stop(uint32_t remainValidMillisecs);
void send_stop_setpoint();
// void send_velocity_world_setpoint(float vx, float vy, float vz, float yawrate);
// void send_zdistance_setpoint(float roll, float pitch, float yawrate, float zDistance);
void send_hover_setpoint(float vx, float vy, float yawrate, float zDistance);
// void send_full_state_setpoint(...);
// viud send_position_setpoint(float x, float y, float z, float yaw);

#endif // __CF_API_COMMANDER__
