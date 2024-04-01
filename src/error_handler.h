#ifndef SRC_ERROR_HANDLER_H_
#define SRC_ERROR_HANDLER_H_

#include <error.h>

namespace ErrorCode {
    static constexpr uint8_t module_init_fail = 254;
    static constexpr uint8_t metadata_failure = 255;
}


/**
 * Initialize the error USART output.
 *
 * UART clocks/GPIO must be already configured.
 */
void initErrHndlr();

/**
 * Return a reference to the global error handling object.
 *
 * @p initErrHndlr must be called prior to using the returned object.
 *
 * @return
 */
Libp::Error& getErrHndlr();


#endif /* SRC_ERROR_HANDLER_H_ */
