/** =================================================================*
 * @file   RtcRx4111.c
 * @brief  Epson RX4111CE リアルタイムクロックドライバ
 * ================================================================= */
#include "RtcRx4111.h"

#define BANK_MIN                           (1U)
#define BANK_MAX                           (7U)
#define ADDRESS_MAX                        (0x0FU)

static uint8_t bcdToDec(uint8_t n)
{
    return (uint8_t)(n - 6U * (n >> 4U));
}

static uint8_t decToBcd(uint8_t n)
{
    return (uint8_t)(n + 6U * (n / 10U));
}

static const uint16_t s_daysBeforeMonth[12] = {
    0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U, 243U, 273U, 304U, 334U
};

static bool isLeapYear(uint16_t year)
{
    return ((year % 4U == 0U) && (year % 100U != 0U)) || (year % 400U == 0U);
}

uint32_t RtcRx4111_TimeToUnix(const RTC_RX4111_TIME *time)
{
    if (time == (void *)0) {
        return 0U;
    }
    uint16_t fullYear = (uint16_t)(2000U + (uint16_t)time->Year);
    uint32_t days = 0U;
    for (uint16_t y = 1970U; y < fullYear; y++) {
        days += isLeapYear(y) ? 366U : 365U;
    }
    uint8_t m = (time->Month >= 1U && time->Month <= 12U) ? time->Month : 1U;
    days += s_daysBeforeMonth[m - 1U];
    if (m > 2U && isLeapYear(fullYear)) {
        days++;
    }
    uint8_t d = (time->Day >= 1U) ? (time->Day - 1U) : 0U;
    days += d;

    uint32_t seconds = days * 86400UL;
    seconds += (uint32_t)time->Hour * 3600UL;
    seconds += (uint32_t)time->Minute * 60UL;
    seconds += (uint32_t)time->Sec;
    return seconds;
}

void RtcRx4111_UnixToTime(uint32_t unixSec, RTC_RX4111_TIME *time)
{
    if (time == (void *)0) {
        return;
    }
    uint32_t remaining = unixSec;
    uint32_t days = remaining / 86400UL;
    remaining %= 86400UL;

    time->Hour = (uint8_t)(remaining / 3600UL);
    remaining %= 3600UL;
    time->Minute = (uint8_t)(remaining / 60UL);
    time->Sec = (uint8_t)(remaining % 60UL);

    uint16_t year = 1970U;
    while (1) {
        uint16_t daysInYear = isLeapYear(year) ? 366U : 365U;
        if (days >= daysInYear) {
            days -= daysInYear;
            year++;
        } else {
            break;
        }
    }
    time->Year = (year >= 2000U) ? (uint8_t)(year - 2000U) : 0U;

    bool leap = isLeapYear(year);
    uint8_t month = 1U;
    for (uint8_t m = 12U; m >= 1U; m--) {
        uint16_t threshold = s_daysBeforeMonth[m - 1U];
        if (m > 2U && leap) {
            threshold++;
        }
        if (days >= threshold) {
            month = m;
            days -= threshold;
            break;
        }
    }
    time->Month = month;
    time->Day = (uint8_t)(days + 1U);
}

#if defined(__arm__)
#include "SoftSpi.h"

static int readReg(void *readBuf, uint8_t bank, uint8_t address, int length)
{
    uint8_t modeAddress;
    if (bank < BANK_MIN || bank > BANK_MAX || address > ADDRESS_MAX) {
        return -1;
    }
    modeAddress = (uint8_t)(((bank | 0x08U) << 4U) | (address & 0x0FU));
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_RTC);
    SoftSpiWrite(&modeAddress, 1);
    SoftSpiRead(readBuf, length, 0x00U);
    SoftSpiDeviceDisable();
    return 0;
}

static int writeReg(const void *writeBuf, uint8_t bank, uint8_t address, int length)
{
    uint8_t modeAddress;
    if (bank < BANK_MIN || bank > BANK_MAX || address > ADDRESS_MAX) {
        return -1;
    }
    modeAddress = (uint8_t)((bank << 4U) | (address & 0x0FU));
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_RTC);
    SoftSpiWrite(&modeAddress, 1);
    SoftSpiWrite(writeBuf, length);
    SoftSpiDeviceDisable();
    return 0;
}

void RtcRx4111_Init(void)
{
    uint8_t flagByte = 0U;
    uint8_t writeVal;
    RTC_RX4111_TIME initTime;

    /* 電源投入後の発振安定化待ち */
    for (volatile uint32_t i = 0U; i < 40000UL; i++) {
    }

    if (readReg(&flagByte, 1U, 0x0EU, 1) == 0) {
        /* Bit 1: VLF (Voltage Low Flag) */
        if ((flagByte & 0x02U) != 0U) {
            writeVal = 0x04U;
            (void)writeReg(&writeVal, 3U, 0x02U, 1);
            writeVal = 0x00U;
            (void)writeReg(&writeVal, 1U, 0x0DU, 1);
            (void)writeReg(&writeVal, 1U, 0x0EU, 1);

            initTime.Year = 26U;  /* 2026年 */
            initTime.Month = 1U;
            initTime.Day = 1U;
            initTime.Hour = 0U;
            initTime.Minute = 0U;
            initTime.Sec = 0U;
            (void)RtcRx4111_SetTime(&initTime);
        }
    }
}

bool RtcRx4111_SetTime(const RTC_RX4111_TIME *time)
{
    if (time == (void *)0) {
        return false;
    }
    uint8_t writeBuf[7];
    writeBuf[0] = decToBcd(time->Sec);
    writeBuf[1] = decToBcd(time->Minute);
    writeBuf[2] = decToBcd(time->Hour);
    writeBuf[3] = 1U;  /* WEEK: 固定1 */
    writeBuf[4] = decToBcd(time->Day);
    writeBuf[5] = decToBcd(time->Month);
    writeBuf[6] = decToBcd(time->Year);
    return (writeReg(writeBuf, 1U, 0x00U, 7) == 0);
}

bool RtcRx4111_GetTime(RTC_RX4111_TIME *time)
{
    if (time == (void *)0) {
        return false;
    }
    uint8_t readBuf[7];
    if (readReg(readBuf, 1U, 0x00U, 7) != 0) {
        return false;
    }
    time->Sec = bcdToDec(readBuf[0] & 0x7FU);
    time->Minute = bcdToDec(readBuf[1] & 0x7FU);
    time->Hour = bcdToDec(readBuf[2] & 0x3FU);
    time->Day = bcdToDec(readBuf[4] & 0x3FU);
    time->Month = bcdToDec(readBuf[5] & 0x1FU);
    time->Year = bcdToDec(readBuf[6]);
    return true;
}

#else
/* ホストシミュレーション環境用 */
static RTC_RX4111_TIME s_simulatedRtcTime = {26U, 9U, 12U, 0U, 0U, 0U};

void RtcRx4111_Init(void)
{
}

bool RtcRx4111_SetTime(const RTC_RX4111_TIME *time)
{
    if (time == (void *)0) {
        return false;
    }
    s_simulatedRtcTime = *time;
    return true;
}

bool RtcRx4111_GetTime(RTC_RX4111_TIME *time)
{
    if (time == (void *)0) {
        return false;
    }
    *time = s_simulatedRtcTime;
    return true;
}
#endif
