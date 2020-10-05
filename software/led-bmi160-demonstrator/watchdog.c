#include "watchdog.h"
#include "platform.h"

volatile WDTUnit *wdt_array = (WDTUnit*)0x10000200;
volatile WDTElement *wdt = (WDTElement*)0x10000208;

const uint32_t WDT_FOOD = 0xD09F00D;
const uint32_t WDT_KEY = 0x51F15E;

const uint8_t WDT_SYSTEM_RST = (1 << 0);
const uint8_t WDT_LED        = (1 << 1);

enum {
  WDT_MODE_TIMEOUT = 0,
  WDT_MODE_WINDOW = 1,
};

static void wdt_unlock(void)
{
    wdt_array->global.key = WDT_KEY;
    for(uint32_t i = 0; i<500; i++){// Add a little idle time before writing next Register
        asm("NOP");
    }
}

static void wdt_lock(void)
{
    wdt_array->global.key = 0;
}

void wdt_disable(void)
{
    wdt_unlock();
    wdt->cfg.field.outputen = 0;
    wdt_unlock();
    wdt->cfg.field.enalways = 0;
    wdt_unlock();
    wdt->cfg.field.encoreawake = 0;
}

void wdt_configure(uint64_t counter_start, uint32_t counter_len,
                   uint16_t err_pulse_len)
{
  GPIO_REG(GPIO_IOF_SEL) &= ~(1 << 19);
  GPIO_REG(GPIO_IOF_EN) |= (1 << 19);
  WDTConfig tmpconf;
  tmpconf.cfg = 0;

  tmpconf.field.scale = 0;               // no scaling
  tmpconf.field.zerocmp = 1;             // flush counter to zero after reset
  tmpconf.field.mode = WDT_MODE_TIMEOUT; // deglitch
  tmpconf.field.interrupt = 0;           // no interrupt pending
  tmpconf.field.outputen = 1;            // no global rst on exception

  wdt_unlock();
  wdt->cfg = tmpconf;

  wdt_unlock();
  wdt->feed = WDT_FOOD;

  wdt_unlock();
  wdt->count = counter_start;

  wdt_unlock();
  wdt->compare[0] = counter_len;

  wdt_unlock();
  wdt->pulsewidth = err_pulse_len;

  wdt_unlock();
  wdt->mux = WDT_LED;
}

void wdt_enable(void)
{
    wdt_unlock();
    wdt->cfg.field.enalways = 1; // WDT is running always
}

void wdt_pet(void)
{
    wdt_unlock();
    wdt->feed = WDT_FOOD;
}
