#ifndef SIMPLE_STM32G0XX_H
#define SIMPLE_STM32G0XX_H


#include "simple_generic.h"
#include <stdbool.h>


/* oscillators */
#define SIMPLE_OSCILLATOR_LSESPEED          32768UL


/* available clocks */
typedef enum
{
    simple_LSE = 0,
    simple_LSI = 1,
    simple_HSE = 2,
    simple_HSI = 3,
} simple_clock;

/* config */
typedef struct
{
    bool enable;
    bool bypassEnable;
} simple_clock_lseConfig_t;

typedef struct
{
    bool enable;
    simple_clock source;
    // bool enableAlarmA; TODO: alarms
    // bool enableAlarmB;
} simple_rcc_config_t;


/* clock functions */
simple_returnStatus simple_clock_configureLse(simple_clock_lseConfig_t*);

/* RTC functions */
simple_returnStatus simple_rtc_configure(simple_rcc_config_t*);
void simple_rtc_writeProtectionDisable(void);
void simple_rtc_writeProtectionEnable(void);


#endif
