#ifndef Watchdog_H
#define Watchdog_H
#include <stdint.h>

typedef union
{
     uint64_t cfg; // integer Representation
     struct
     {
           unsigned int scale : 4;
           unsigned int rsv0 : 4;
           unsigned int outputen : 1;
           unsigned int zerocmp : 1;
           unsigned int mode : 1;
           unsigned int rsv1 : 1;
           unsigned int enalways : 1;
           unsigned int encoreawake : 1;       // not used in TileLink
           unsigned int rsv2 : 14;
           unsigned int interrupt : 1;
           unsigned int rsv3 : 3;
           unsigned int rsv4 : 32;
      } field;
} WDTConfig;

typedef struct {
    uint32_t key;
    uint32_t inv;
} WDTGlobal;

typedef struct {
    WDTConfig cfg;
    uint64_t count;
    uint64_t s; // Scaled Counter
    uint32_t feed;
    uint32_t rsvd;
    uint32_t compare[2];
    uint32_t pulsewidth;
    uint32_t mux;
} WDTElement;

typedef struct {
    WDTGlobal global;
    WDTElement unit[];
} WDTUnit;


void wdt_disable(void);
void wdt_enable(void);
void wdt_configure(uint64_t counter_start, uint32_t counter_len, uint16_t err_pulse_len);
void wdt_pet(void);

#endif
