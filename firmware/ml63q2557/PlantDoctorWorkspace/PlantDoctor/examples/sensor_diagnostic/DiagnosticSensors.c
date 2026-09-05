#include "DiagnosticSensors.h"

#include <limits.h>
#include <stddef.h>

#include "DiagnosticI2c.h"
#include "TimeControl.h"
#include "irq.h"
#include "mcu.h"
#include "rdwr_reg.h"
#include "saAdc0.h"
#include "smpl_common.h"
#include "wdt.h"

#define SEN0206_ADDRESS               (0x5AU)
#define SEN0206_AMBIENT_REGISTER      (0x06U)
#define SEN0206_OBJECT_REGISTER       (0x07U)
#define SEN0206_ERROR_FLAG            (0x8000U)

#define SEN0385_ADDRESS               (0x44U)
#define SEN0385_MEASURE_MSB           (0x24U)
#define SEN0385_MEASURE_LSB           (0x00U)

#define SEN0228_ADDRESS               (0x10U)
#define SEN0228_CONFIG_REGISTER       (0x00U)
#define SEN0228_ALS_REGISTER          (0x04U)

#define SEN0204_INPUT_MASK            (1UL << 4U)
#define PHOTO_COUPLER_INPUT_CONFIG    ((0x01UL << 8U) | (0x01UL << 0U))
#define ADC_TIMEOUT_LOOPS             (480000UL)

static SENSOR_READING_STATUS DiagnosticSensors_MapI2cStatus(
	DIAGNOSTIC_I2C_STATUS status)
{
	switch (status)
	{
		case DIAGNOSTIC_I2C_OK:
			return SENSOR_READING_OK;
		case DIAGNOSTIC_I2C_NACK:
			return SENSOR_READING_NACK;
		case DIAGNOSTIC_I2C_BUS_BUSY:
			return SENSOR_READING_BUS_BUSY;
		case DIAGNOSTIC_I2C_TIMEOUT:
			return SENSOR_READING_TIMEOUT;
		case DIAGNOSTIC_I2C_INVALID_ARGUMENT:
		default:
			return SENSOR_READING_INVALID_DATA;
	}
}

static uint8_t DiagnosticSensors_Crc8(
	const uint8_t *data,
	uint8_t size,
	uint8_t polynomial,
	uint8_t initial)
{
	uint8_t crc = initial;
	uint8_t index;
	uint8_t bit;

	for (index = 0U; index < size; ++index)
	{
		crc ^= data[index];
		for (bit = 0U; bit < 8U; ++bit)
		{
			crc = ((crc & 0x80U) != 0U) ?
				(uint8_t)((crc << 1U) ^ polynomial) :
				(uint8_t)(crc << 1U);
		}
	}
	return crc;
}

static SENSOR_READING_STATUS DiagnosticSensors_ReadMlxRegister(
	uint8_t command,
	uint16_t *value)
{
	uint8_t response[3];
	uint8_t pecInput[5];
	DIAGNOSTIC_I2C_STATUS i2cStatus;

	i2cStatus = DiagnosticI2c_WriteRead(
		SEN0206_ADDRESS, &command, 1U, response, 3U);
	if (i2cStatus != DIAGNOSTIC_I2C_OK)
	{
		return DiagnosticSensors_MapI2cStatus(i2cStatus);
	}

	pecInput[0] = (uint8_t)(SEN0206_ADDRESS << 1U);
	pecInput[1] = command;
	pecInput[2] = (uint8_t)((SEN0206_ADDRESS << 1U) | 1U);
	pecInput[3] = response[0];
	pecInput[4] = response[1];
	if (DiagnosticSensors_Crc8(pecInput, 5U, 0x07U, 0x00U) != response[2])
	{
		return SENSOR_READING_CRC;
	}

	*value = (uint16_t)response[0] | ((uint16_t)response[1] << 8U);
	if ((*value & SEN0206_ERROR_FLAG) != 0U)
	{
		return SENSOR_READING_INVALID_DATA;
	}
	return SENSOR_READING_OK;
}

static bool DiagnosticSensors_MlxToCentiC(uint16_t raw, int16_t *temperatureCentiC)
{
	int32_t converted = ((int32_t)raw * 2L) - 27315L;

	if ((temperatureCentiC == NULL) || (converted < INT16_MIN) ||
		(converted > INT16_MAX))
	{
		return false;
	}
	*temperatureCentiC = (int16_t)converted;
	return true;
}

void DiagnosticSensors_Init(void)
{
	initAdc_t adcConfig = {0};
	enableAdcChannel_t channels = {0};

	smpl_enablePeripheral(SAD0_PERI);
	irq_sad0_dis();
	irq_sad0_clearIRQ();

	adcConfig.discharge = SAADC_SAINIT_DISCHARGE;
	adcConfig.holdTime = 0x03U;
	adcConfig.clock = SAADC_SACK_OSCLK_DIV16;
	adcConfig.mode = SAADC_SALP_ONESHOT;
	adcConfig.limitInterrupt = SAADC_SALMD_INSIDE_LIMIT;
	adcConfig.limitMode = SAADC_SALEN_DISABLE;
	adcConfig.ampStabilityTime = 0x02U;
	adcConfig.interruptMode = SAADC_SADIMD0_ALL_CH;
	adcConfig.interruptLimitMode = SAADC_SADIMD1_LIMIT_MATCH;
	adcConfig.interval = 0U;
	adcConfig.channelSync = SAADC_SYNC_NORMAL;
	saAdc0_init(&adcConfig);

	channels.ch0 = SAADC_RUN;
	saAdc0_setEnableChannel(&channels);

	set_reg32(PORT6->P6MOD1, PHOTO_COUPLER_INPUT_CONFIG);
}

void DiagnosticSensors_ReadSen0206(SEN0206_READING *reading)
{
	uint16_t ambientRaw;
	uint16_t objectRaw;

	if (reading == NULL)
	{
		return;
	}
	reading->status = SENSOR_READING_INVALID_DATA;
	reading->objectTemperatureCentiC = 0;
	reading->ambientTemperatureCentiC = 0;
	reading->status = DiagnosticSensors_ReadMlxRegister(
		SEN0206_AMBIENT_REGISTER, &ambientRaw);
	if (reading->status == SENSOR_READING_OK)
	{
		reading->status = DiagnosticSensors_ReadMlxRegister(
			SEN0206_OBJECT_REGISTER, &objectRaw);
	}
	if (reading->status == SENSOR_READING_OK)
	{
		if (!DiagnosticSensors_MlxToCentiC(ambientRaw,
			&reading->ambientTemperatureCentiC) ||
			!DiagnosticSensors_MlxToCentiC(objectRaw,
				&reading->objectTemperatureCentiC))
		{
			reading->status = SENSOR_READING_INVALID_DATA;
		}
	}
}

void DiagnosticSensors_ReadSen0385(SEN0385_READING *reading)
{
	const uint8_t command[2] = {SEN0385_MEASURE_MSB, SEN0385_MEASURE_LSB};
	uint8_t response[6];
	uint16_t temperatureRaw;
	uint16_t humidityRaw;
	DIAGNOSTIC_I2C_STATUS i2cStatus;

	if (reading == NULL)
	{
		return;
	}
	reading->status = SENSOR_READING_INVALID_DATA;
	reading->temperatureCentiC = 0;
	reading->humidityCentiPercent = 0U;

	i2cStatus = DiagnosticI2c_Write(SEN0385_ADDRESS, command, 2U);
	if (i2cStatus != DIAGNOSTIC_I2C_OK)
	{
		reading->status = DiagnosticSensors_MapI2cStatus(i2cStatus);
		return;
	}
	if (!TimeControlDelayMs(20U))
	{
		reading->status = SENSOR_READING_TIMEOUT;
		return;
	}
	i2cStatus = DiagnosticI2c_Read(SEN0385_ADDRESS, response, 6U);
	if (i2cStatus != DIAGNOSTIC_I2C_OK)
	{
		reading->status = DiagnosticSensors_MapI2cStatus(i2cStatus);
		return;
	}
	if ((DiagnosticSensors_Crc8(response, 2U, 0x31U, 0xFFU) != response[2]) ||
		(DiagnosticSensors_Crc8(&response[3], 2U, 0x31U, 0xFFU) != response[5]))
	{
		reading->status = SENSOR_READING_CRC;
		return;
	}

	temperatureRaw = ((uint16_t)response[0] << 8U) | response[1];
	humidityRaw = ((uint16_t)response[3] << 8U) | response[4];
	reading->temperatureCentiC = (int16_t)(-4500L +
		(((int32_t)17500L * temperatureRaw + 32767L) / 65535L));
	reading->humidityCentiPercent = (uint16_t)(
		((uint32_t)10000UL * humidityRaw + 32767UL) / 65535UL);
	reading->status = SENSOR_READING_OK;
}

void DiagnosticSensors_ReadSen0228(SEN0228_READING *reading)
{
	const uint8_t config[3] = {SEN0228_CONFIG_REGISTER, 0x00U, 0x00U};
	const uint8_t alsRegister = SEN0228_ALS_REGISTER;
	uint8_t response[2];
	DIAGNOSTIC_I2C_STATUS i2cStatus;

	if (reading == NULL)
	{
		return;
	}
	reading->status = SENSOR_READING_INVALID_DATA;
	reading->illuminanceCentiLux = 0UL;
	reading->raw = 0U;

	/* Gain x1, 100 ms integration, interrupt disabled, sensor enabled. */
	i2cStatus = DiagnosticI2c_Write(SEN0228_ADDRESS, config, 3U);
	if (i2cStatus != DIAGNOSTIC_I2C_OK)
	{
		reading->status = DiagnosticSensors_MapI2cStatus(i2cStatus);
		return;
	}
	if (!TimeControlDelayMs(120U))
	{
		reading->status = SENSOR_READING_TIMEOUT;
		return;
	}
	i2cStatus = DiagnosticI2c_WriteRead(
		SEN0228_ADDRESS, &alsRegister, 1U, response, 2U);
	if (i2cStatus != DIAGNOSTIC_I2C_OK)
	{
		reading->status = DiagnosticSensors_MapI2cStatus(i2cStatus);
		return;
	}

	reading->raw = (uint16_t)response[0] | ((uint16_t)response[1] << 8U);
	/* At gain x1 and 100 ms, resolution is 0.0576 lx/count. */
	reading->illuminanceCentiLux =
		(((uint32_t)reading->raw * 576UL) + 50UL) / 100UL;
	reading->status = SENSOR_READING_OK;
}

void DiagnosticSensors_ReadSen0193(SEN0193_READING *reading)
{
	uint32_t remaining = ADC_TIMEOUT_LOOPS;
	uint16_t adcRaw;

	if (reading == NULL)
	{
		return;
	}
	reading->status = SENSOR_READING_INVALID_DATA;
	reading->raw = 0U;
	reading->millivolts = 0U;

	saAdc0_start();
	while ((saAdc0_getRunning() != 0U) && (remaining > 0UL))
	{
		--remaining;
		if ((remaining & 0x3FFFUL) == 0UL)
		{
			wdt_clear();
		}
	}
	if (remaining == 0UL)
	{
		saAdc0_stop();
		reading->status = SENSOR_READING_TIMEOUT;
		return;
	}

	adcRaw = (uint16_t)((saAdc0_getResult0() & 0xFFF0UL) >> 4U);
	/* The board's three inverting amplifier stages invert the CN6 voltage. */
	reading->raw = (uint16_t)(4095U - adcRaw);
	reading->millivolts = (uint16_t)(
		((uint32_t)reading->raw * 3300UL + 2047UL) / 4095UL);
	reading->status = SENSOR_READING_OK;
}

bool DiagnosticSensors_ReadSen0204(void)
{
	/* IN0 uses a pull-up after the photocoupler, so an asserted input is low. */
	return (get_bit(PORT6->P6DI, SEN0204_INPUT_MASK) == 0U);
}

const char *DiagnosticSensors_StatusText(SENSOR_READING_STATUS status)
{
	switch (status)
	{
		case SENSOR_READING_OK:
			return "OK";
		case SENSOR_READING_NACK:
			return "NACK";
		case SENSOR_READING_BUS_BUSY:
			return "BUSY";
		case SENSOR_READING_TIMEOUT:
			return "TIMEOUT";
		case SENSOR_READING_CRC:
			return "CRC";
		case SENSOR_READING_INVALID_DATA:
		default:
			return "DATA";
	}
}
