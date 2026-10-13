#include "simple_stm32g0xx.h"
#include "stm32g031xx.h"


/*
 * @brief Sets the desired configuration of the LSE clock
 *
 * @param `simple_clockConfig_LSE` is a struct including the desired config
 */
simple_returnStatus simple_clock_configureLse(simple_clock_lseConfig_t *configuration)
{
    // if PWR clock is disabled at the start, disable it at the end again
    bool disable_pwr;

    if (RCC->APBENR1 & RCC_APBENR1_PWREN)
    {
        disable_pwr = false;
    }
    else
    {
        disable_pwr = true;
        RCC->APBENR1 |= RCC_APBENR1_PWREN;
    }

    /* note: DBP isn't checked before modifying as it's assumed all backup domain writes
     * are responsible enough to disable DBP after modifications */
    PWR->CR1 |= PWR_CR1_DBP;

    /* note: HAL waits for this to be set but no mention of it being necessary is made
     * anywhere, doing it in case as it doesn't hurt */
    while (!(PWR->CR1 & PWR_CR1_DBP))
        ;


    // configure LSE
    if (configuration->bypassEnable == true)
        RCC->BDCR |=  RCC_BDCR_LSEBYP;
    else
        RCC->BDCR &= ~RCC_BDCR_LSEBYP;

    if (configuration->enable == true)
        RCC->BDCR |=  RCC_BDCR_LSEON;
    else
        RCC->BDCR &= ~RCC_BDCR_LSEON;


    while (!(RCC->BDCR & RCC_BDCR_LSERDY))
        ;

    if (disable_pwr == true)
    {
        RCC->APBENR1 &= ~RCC_APBENR1_PWREN;
    }

    return simple_OK;
}
