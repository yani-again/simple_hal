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
 * @param `simple_rtc_config_t*` is a pointer to a struct including the desired config
 */
simple_returnStatus simple_rtc_configure(simple_rtc_config_t *configuration)
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

    /* start time */
    RTC->TR = 0;
    RTC->DR = 0;

    // day
    RTC->DR |= (configuration->startDate[0] - 48) << RTC_DR_DT_Pos;
    RTC->DR |= (configuration->startDate[1] - 48) << RTC_DR_DU_Pos;
                                           // 48 is the offset from which ASCII numbers start
    // month
    RTC->DR |= (configuration->startDate[2] - 48) << RTC_DR_MT_Pos;
    RTC->DR |= (configuration->startDate[3] - 48) << RTC_DR_MU_Pos;
    // year
    RTC->DR |= (configuration->startDate[4] - 48) << RTC_DR_YT_Pos;
    RTC->DR |= (configuration->startDate[5] - 48) << RTC_DR_YU_Pos;
    // hour
    RTC->TR |= (configuration->startTime[0] - 48) << RTC_TR_HT_Pos;
    RTC->TR |= (configuration->startTime[1] - 48) << RTC_TR_HU_Pos;
    // minute
    RTC->TR |= (configuration->startTime[2] - 48) << RTC_TR_MNT_Pos;
    RTC->TR |= (configuration->startTime[3] - 48) << RTC_TR_MNU_Pos;
    // second
    RTC->TR |= (configuration->startTime[4] - 48) << RTC_TR_ST_Pos;
    RTC->TR |= (configuration->startTime[5] - 48) << RTC_TR_SU_Pos;

    // configure RTC to use 24-hour mode
    RTC->CR &= ~RTC_CR_FMT;

    /* RM0444 mentioned this bit is cleared by hardware or software, I rather do it in software
     * to not rely on the hardware as it seemed a bit unclear */
    RTC->ICSR &= ~RTC_ICSR_RSF;

    // exit init mode
    RTC->ICSR &= ~RTC_ICSR_INIT;

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

/*
 * @brief Gets the current calendar time of the RTC and stores it in a simple-HAL
 *        struct.
 *
 * @param `simple_rtc_currentTime*` is a pointer to a struct which only contains the
 *        current date/time in string format
 */
void simple_rtc_getTime(simple_rtc_currentTime* currentTime)
{
    // wait for shadow register sync
    while (!(RTC->ICSR & RTC_ICSR_RSF))
        ;
    RTC->ICSR &= ~RTC_ICSR_RSF;

    uint32_t time_register = RTC->TR;
    uint32_t date_register = RTC->DR;

    // hour
    currentTime->time[0] = (time_register & RTC_TR_HT_Msk) >> RTC_TR_HT_Pos;
    currentTime->time[1] = (time_register & RTC_TR_HU_Msk) >> RTC_TR_HU_Pos;
    // minute
    currentTime->time[2] = (time_register & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos;
    currentTime->time[3] = (time_register & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos;
    // second
    currentTime->time[4] = (time_register & RTC_TR_ST_Msk) >> RTC_TR_ST_Pos;
    currentTime->time[5] = (time_register & RTC_TR_SU_Msk) >> RTC_TR_SU_Pos;
    // day
    currentTime->date[0] = (date_register & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos;
    currentTime->date[1] = (date_register & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos;
    // month
    currentTime->date[2] = (date_register & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos;
    currentTime->date[3] = (date_register & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos;
    // year
    currentTime->date[4] = (date_register & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos;
    currentTime->date[5] = (date_register & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos;
}
