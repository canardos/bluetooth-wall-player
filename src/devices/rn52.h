/*
 */
#ifndef SRC_DEVICES_RN52_H_
#define SRC_DEVICES_RN52_H_

#include <drivers/wireless/rn_52.h>

/**
 * Start RN52 UART and power up module.
 */
void initRn52();


Libp::Rn52& getRn52();


#endif /* SRC_DEVICES_RN52_H_ */
