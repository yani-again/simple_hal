#include "simple_stm32g0xx.h"
#include "stm32g031xx.h"


#define RTC_PREDIV_A_MAX 127U
#define RTC_PREDIV_S_MAX 32767U


/* ------ */
/* clocks */
/* ------ */

/*
 * @brief Sets the desired configuration of the LSE clock
 *
 * @param `simple_clock_lseConfig_t*` is a pointer to a struct including the desired config
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


/* --- */
/* RTC */
/* --- */

/*
 * @brief Sets the desired configuration of the RTC
 *
 * @param `simple_rcc_config_t*` is a pointer to a struct including the desired config
 */
simple_returnStatus simple_rtc_configure(simple_rcc_config_t *configuration)
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

    // enter init mode
    RTC->ICSR |= RTC_ICSR_INIT;
    while (!(RTC->ICSR & RTC_ICSR_INITF))
        ;

    uint32_t prediv_a, prediv_s;
    // TODO: check that  fPCLK >= 2 * fRTCCLK
    switch (configuration->source)
    {
        case simple_LSE:
            prediv_a = RTC_PREDIV_A_MAX;
            prediv_s = SIMPLE_OSCILLATOR_LSESPEED / (prediv_a + 1);
            --prediv_s;

            if (prediv_s > RTC_PREDIV_S_MAX)
                return simple_ERR;
            break;
        // TODO: handle other oscillators
        default:
            return simple_ERR;
    }
    prediv_a <<= RTC_PRER_PREDIV_A_Pos;
    prediv_s <<= RTC_PRER_PREDIV_S_Pos;
    prediv_a &= RTC_PRER_PREDIV_A_Msk;
    prediv_s &= RTC_PRER_PREDIV_S_Msk;

    RTC->PRER = prediv_a | prediv_s;

    return simple_OK;
}

void simple_rtc_writeProtectionDisable(void)
{
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
}

void simple_rtc_writeProtectionEnable(void)
{
    RTC->WPR = 0xFF;
}
