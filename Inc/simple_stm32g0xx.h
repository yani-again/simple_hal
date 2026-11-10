#ifndef SIMPLE_STM32G0XX_H
#define SIMPLE_STM32G0XX_H


#include "simple_generic.h"
#include <stdbool.h>
#include <stdint.h>


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


/* rtc */
typedef struct
{
    bool enable;
    simple_clock source;
    // bool enableAlarmA; TODO: alarms
    // bool enableAlarmB;
    uint8_t startDate[6];  // expected in string format "DDMMYY"
    uint8_t startTime[6];  // expected in string format "HHMMSS"
} simple_rtc_config_t;

typedef struct
{
    uint8_t date[6];    // expected in string format "DDMMYY"
    uint8_t time[6];    // expected in string format "HHMMSS"
} simple_rtc_currentTime;


/* gpio */
typedef enum
{
    simple_gpio_mode_analog = 0,
    simple_gpio_mode_input  = 1,
    simple_gpio_mode_output = 2,
    simple_gpio_mode_alt    = 3
} simple_gpio_mode;

typedef enum
{
    simple_gpio_out_od       = 0,
    simple_gpio_out_pushpull = 1
} simple_gpio_out;

typedef enum
{
    simple_gpio_speed_vlow  = 0,
    simple_gpio_speed_low   = 1,
    simple_gpio_speed_high  = 2,
    simple_gpio_speed_vhigh = 3
} simple_gpio_speed;

typedef enum
{
    simple_gpio_pupd_none = 0,
    simple_gpio_pupd_pu   = 1,
    simple_gpio_pupd_pd   = 2
} simple_gpio_pupd;

typedef struct
{
    // TODO: decide what's needed in the config struct
    // NOTE! open-drain output = NO PULL-DOWN ALLOWED, only pull-up
    //       EXCEPT in alt func mode, then pd is allowed?
    // NOTE! in analog mode, output mode isn't selectable and pupd is set to NONE
    simple_gpio_mode mode;
    simple_gpio_out outputType;
    simple_gpio_speed speed;
    simple_gpio_pupd pupd;
    uint8_t pin;
} simple_gpio_config_t;

typedef struct
{
  uint32_t MODER;
  uint32_t OTYPER;
  uint32_t OSPEEDR;
  uint32_t PUPDR;
  uint32_t IDR;
  uint32_t ODR;
  uint32_t BSRR;
  uint32_t LCKR;
  uint32_t AFRL;
  uint32_t AFRH;
  uint32_t BRR;
} simple_gpio_typeDef;


/* clock functions */
simple_returnStatus simple_clock_configureLse(simple_clock_lseConfig_t*);

/* RTC functions */
simple_returnStatus simple_rtc_configure(simple_rtc_config_t*);
void simple_rtc_writeProtectionDisable(void);
void simple_rtc_writeProtectionEnable(void);
void simple_rtc_getTime(simple_rtc_currentTime*);

/* GPIO functions */
simple_returnStatus simple_gpio_configure(simple_gpio_typeDef*, simple_gpio_config_t*);
void simple_gpio_portEnableDisable(uint8_t, bool);
void simple_gpio_portReset(uint8_t);


#endif
