#ifndef SIMPLE_STM32G0XX_H
#define SIMPLE_STM32G0XX_H


#include "simple_generic.h"
#include <stdbool.h>


/* ------- */
/* generic */
/* ------- */

/* config */

typedef struct
{
    bool enable;
    bool bypassEnable;
} simple_clock_lseConfig_t;


/* --------- */
/* STM32G0xx */
/* --------- */

/* clock functions */
simple_returnStatus simple_clock_configureLse(simple_clock_lseConfig_t*);


#endif
