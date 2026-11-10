#include "simple_stm32g0xx.h"
#include "stm32g031xx.h"
#include <assert.h>


#define RTC_PREDIV_A_MAX 127U
#define RTC_PREDIV_S_MAX 32767U

#define SIMPLE_GPIO_MODER_PUPDR_OSPEEDR_Pos(pin) ((pin) * 2)
#define SIMPLE_GPIO_MODER_PUPDR_OSPEEDR_Msk(pin) ((uint32_t) (3U << ((pin) * 2)))

#define SIMPLE_RCC_IOPENR_GPIOXEN_Pos(port) ((port) - 'A')


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
    if (configuration->enable == true)
        RCC->BDCR |=  RCC_BDCR_LSEON;
    else
    {
        RCC->BDCR &= ~RCC_BDCR_LSEON;
        goto reset_pwr;
    }

    if (configuration->bypassEnable == true)
        RCC->BDCR |=  RCC_BDCR_LSEBYP;
    else
        RCC->BDCR &= ~RCC_BDCR_LSEBYP;

    while (!(RCC->BDCR & RCC_BDCR_LSERDY))
        ;

reset_pwr:
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
    simple_returnStatus retVal = simple_OK;

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

    PWR->CR1 |= PWR_CR1_DBP;
    while (!(PWR->CR1 & PWR_CR1_DBP))
        ;

    // enable RTC peripheral clock
    RCC->APBENR1 |= RCC_APBENR1_RTCAPBEN;

    uint32_t prediv_a, prediv_s;
    // TODO: check that  fPCLK >= 2 * fRTCCLK
    switch (configuration->source)
    {
        case simple_LSE:
        {
            // select LSE as kernel clock
            uint32_t tmp = RCC->BDCR;
            tmp &= ~RCC_BDCR_RTCSEL_Msk;
            tmp |= 1 << RCC_BDCR_RTCSEL_Pos;
            RCC->BDCR = tmp;

            // enable/disable RTC kernel clock
            if (configuration->enable)
            {
                RCC->BDCR |= RCC_BDCR_RTCEN;
            }
            else
            {
                RCC->BDCR &= ~RCC_BDCR_RTCEN;
                goto reset_pwr;   // skip init sequence
            }

            prediv_a = RTC_PREDIV_A_MAX;
            prediv_s = SIMPLE_OSCILLATOR_LSESPEED / (prediv_a + 1);
            --prediv_s;

            if (prediv_s > RTC_PREDIV_S_MAX)
            {
                retVal = simple_ERR;
                goto reset_pwr;
            }

            break;
        }
        // TODO: handle other oscillators
        default:
        {
            retVal = simple_ERR;
            goto reset_pwr;
        }
    }
    // write protection disable
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // enter init mode
    RTC->ICSR |= RTC_ICSR_INIT;
    while (!(RTC->ICSR & RTC_ICSR_INITF))
        ;

    RTC->PRER =  prediv_s << RTC_PRER_PREDIV_S_Pos;
    RTC->PRER |= prediv_a << RTC_PRER_PREDIV_A_Pos;

    /* start time */
    uint32_t tr = 0;
    uint32_t dr = 0;

    // day
    dr |= (configuration->startDate[0] - 48) << RTC_DR_DT_Pos;
    dr |= (configuration->startDate[1] - 48) << RTC_DR_DU_Pos;
                                           // 48 is the offset from which ASCII numbers start
    // month
    dr |= (configuration->startDate[2] - 48) << RTC_DR_MT_Pos;
    dr |= (configuration->startDate[3] - 48) << RTC_DR_MU_Pos;
    // year
    dr |= (configuration->startDate[4] - 48) << RTC_DR_YT_Pos;
    dr |= (configuration->startDate[5] - 48) << RTC_DR_YU_Pos;
    // hour
    tr |= (configuration->startTime[0] - 48) << RTC_TR_HT_Pos;
    tr |= (configuration->startTime[1] - 48) << RTC_TR_HU_Pos;
    // minute
    tr |= (configuration->startTime[2] - 48) << RTC_TR_MNT_Pos;
    tr |= (configuration->startTime[3] - 48) << RTC_TR_MNU_Pos;
    // second
    tr |= (configuration->startTime[4] - 48) << RTC_TR_ST_Pos;
    tr |= (configuration->startTime[5] - 48) << RTC_TR_SU_Pos;

    RTC->TR = tr;
    RTC->DR = dr;

    // configure RTC to use 24-hour mode
    RTC->CR &= ~RTC_CR_FMT;

    /* RM0444 mentioned this bit is cleared by hardware or software, I rather do it in software
     * to not rely on the hardware as it seemed a bit unclear */
    RTC->ICSR &= ~RTC_ICSR_RSF;

    // exit init mode
    RTC->ICSR &= ~RTC_ICSR_INIT;

    RTC->WPR = 0xFF;

reset_pwr:
    PWR->CR1 &= ~PWR_CR1_DBP;
    if (disable_pwr == true)
        RCC->APBENR1 &= ~RCC_APBENR1_PWREN;

    if (retVal != simple_OK)
        RCC->APBENR1 &= ~RCC_APBENR1_RTCAPBEN;
    
    return retVal;
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


/* gpio */
static inline __attribute__((always_inline))
void simple_gpio_setMode(uint32_t* reg, uint8_t pin, uint8_t mode)
{
    uint32_t tmp = *reg;
    tmp = (tmp & ~SIMPLE_GPIO_MODER_PUPDR_OSPEEDR_Msk(pin)) | (1U << SIMPLE_GPIO_MODER_PUPDR_OSPEEDR_Pos(pin));
    *reg = tmp;
}

simple_returnStatus simple_gpio_configure(simple_gpio_typeDef* gpiox, simple_gpio_config_t* config)
{
    assert(config->pin <= 15);

    switch (config->mode)
    {
        case simple_gpio_mode_input:
            simple_gpio_setMode(&(gpiox->MODER), config->pin, 0U);
            break;
        case simple_gpio_mode_output:
            simple_gpio_setMode(&(gpiox->MODER), config->pin, 1U);
            break;
        case simple_gpio_mode_alt:
            simple_gpio_setMode(&(gpiox->MODER), config->pin, 2U);
            break;
        case simple_gpio_mode_analog:
            simple_gpio_setMode(&(gpiox->MODER), config->pin, 3U);
            break;
        default:
            return simple_ERR;
            break;
    }

    switch (config->outputType)
    {
        case simple_gpio_out_pushpull:
            gpiox->OTYPER &= ~(1U << config->pin);
            break;
        case simple_gpio_out_od:
            gpiox->OTYPER |= 1 << config->pin;
            break;
        default:
            return simple_ERR;
            break;
    }

    switch (config->pupd)
    {
        case simple_gpio_pupd_none:
            simple_gpio_setMode(&(gpiox->PUPDR), config->pin, 0U);
            break;
        case simple_gpio_pupd_pu:
            simple_gpio_setMode(&(gpiox->PUPDR), config->pin, 1U);
            break;
        case simple_gpio_pupd_pd:
            simple_gpio_setMode(&(gpiox->PUPDR), config->pin, 2U);
            break;
        default:
            return simple_ERR;
            break;
    }

    switch (config->speed)
    {
        case simple_gpio_speed_vlow:
            simple_gpio_setMode(&(gpiox->OSPEEDR), config->pin, 0U);
            break;
        case simple_gpio_speed_low:
            simple_gpio_setMode(&(gpiox->OSPEEDR), config->pin, 1U);
            break;
        case simple_gpio_speed_high:
            simple_gpio_setMode(&(gpiox->OSPEEDR), config->pin, 2U);
            break;
        case simple_gpio_speed_vhigh:
            simple_gpio_setMode(&(gpiox->OSPEEDR), config->pin, 3U);
            break;
        default:
            return simple_ERR;
            break;
    }

    return simple_OK;
}

void simple_gpio_portEnableDisable(uint8_t port, bool enable)
{
    // STM32G031K8 only has GPIO ports A, B, C, D, and F
    assert((port >= 'A' && port <= 'D') || port == 'F');

    if (enable == true)
        RCC->IOPENR |= 1 << SIMPLE_RCC_IOPENR_GPIOXEN_Pos(port);
    else
        RCC->IOPENR &= ~(1 << SIMPLE_RCC_IOPENR_GPIOXEN_Pos(port));

    return;
}
