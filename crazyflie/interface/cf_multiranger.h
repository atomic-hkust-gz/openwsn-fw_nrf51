/**
 * 
 * @file single_status_led.h
 * @brief Because the nRF51822 on Crazyflie 2.X has only one LED, this interface is designed to control it.
 *          The status LED is used to indicate the status of the Crazyflie 2.X and the OpenWSN stack.
 * @author Lan HUANG (YelloooBlue@outlook.com)
 * @date Aug 2024
 *
 */
#ifndef __CF_MUTIRANGER_H__
#define __CF_MUTIRANGER_H__

#include <stdint.h>
#include <stdbool.h>

//=========================== define ==========================================

#define LOG_RANGER_FRONT_ID 148
#define LOG_RANGER_BACK_ID 149
#define LOG_RANGER_UP_ID 150
#define LOG_RANGER_LEFT_ID 151
#define LOG_RANGER_RIGHT_ID 152
#define LOG_RANGER_ZRANGE_ID 153

//=========================== typedef =========================================

struct mutiranger_data {
    uint16_t front;
    uint16_t back;
    uint16_t left;
    uint16_t right;
    uint16_t up;
    uint16_t down;
} __attribute__((packed));

struct ops_setting_v2
{
    uint8_t logType;
    uint16_t id;
} __attribute__((packed));

//=========================== variables =======================================

//=========================== prototypes ======================================

//TODO: mutiranget_inMounted() // according the TOC, CTRP callback (port=2,channal=1)
void multiranger_init();
void multiranger_handle(uint8_t *data, uint8_t len);

// Getter
uint16_t multiranger_get_front_mm();
uint16_t multiranger_get_back_mm();
uint16_t multiranger_get_left_mm();
uint16_t multiranger_get_right_mm();
uint16_t multiranger_get_up_mm();
uint16_t multiranger_get_down_mm();
bool multiranger_front_isClose();
bool multiranger_back_isClose();
bool multiranger_left_isClose();
bool multiranger_right_isClose();
bool multiranger_up_isClose();
bool multiranger_down_isClose();

// Setter
void multiranger_set_close_threshold(uint16_t threshold_mm);

#endif // __CF_MUTIRANGER_H__