#ifndef SRC_DEVICES_SENSORS_H_
#define SRC_DEVICES_SENSORS_H_

#include "drivers/sensors/bme280.h"
#include "drivers/sensors/vcnl_4200.h"

//#define MOCK_DATA
//#define TEST_PROX_MODE

using EnvData = Libp::Bme280::Measurements;

/**
 * Initialize the temperature/presssure/humidity sensor.
 *
 * @return true on success, false on failure
 */
bool initTempSensor();

/**
 * Initializes the ALS and proximity sensor and calibrates for interrupts to be
 * triggered on proximity count > current + margin.
 *
 * @return current proximity count or 0 on error.
 */
uint16_t initProxSensor();

#ifdef TEST_PROX_MODE
uint16_t getProxCount();
#endif

uint8_t clearProxInterrupt();

/**
 *
 * @param env_data [out]
 * @param secsActive seconds elapsed since device woke
 *
 * @return
 */
bool getTemp(EnvData& env_data, uint32_t secsActive);

/**
 * @return ambient light level in millilux (0 -> 1,573,000)
 */
uint32_t getLightLvl();


#endif /* SRC_DEVICES_SENSORS_H_ */
