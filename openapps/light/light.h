#ifndef __LIGHT_H
#define __LIGHT_H

#include "opentimers.h"

//=========================== define ==========================================

#define LIGHT_SEND_PERIOD_MS       100
#define LIGHT_SEND_RETRIES         5
#define LUX_THRESHOLD              500
#define LUX_HYSTERESIS             100
//#define LUX_THRESHOLD              2000

//#define SINK_ID             0xeca3
//#define SINK_ID             0x13cf
#define SINK_ID           0xed4e
//#define SENSOR_ID           0xed4e
#define SENSOR_ID           0x89a5

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   opentimer_id_t       sendTimerId;    // timer ID for sending multiple packets in every event
   opentimer_id_t       fwTimerId;      // timer ID for forwarding one packet
   uint16_t             counter;        // event sequence number
   uint16_t             lux;            // current lux read
   bool                 state;          // current state
   bool                 initialized;    // flag to indicate the application has been initialized
   bool                 isForwarding;   // flag to indicate if we are forwarding a packet
   uint8_t              n_tx;           // controls the number of packets transmitted in each event
   uint8_t              asn[5];         // holds the ASN of last event
} light_vars_t;

//=========================== prototypes ======================================

void light_init();
void light_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void light_receive_data(OpenQueueEntry_t* msg);
void light_receive_beacon(OpenQueueEntry_t* msg);
void light_send(uint16_t lux, bool state);
void light_initialize(bool state);
bool light_is_initialized(void);
bool light_state(void);
uint16_t light_counter(void);
bool light_checkMyId(uint16_t addr);
void light_prepare_packet(OpenQueueEntry_t* pkt);

#endif
