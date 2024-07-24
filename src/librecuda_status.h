#ifndef LIBRECUDA_VALIDATE_H
#define LIBRECUDA_VALIDATE_H

#include <iostream>

typedef int libreCudaStatus_t;

#define LIBRECUDA_VALIDATE(condition, err) { \
    if (!(condition)) {                      \
        return err;                          \
    }                                        \
}

// declare all cuda error codes
#define LIBRECUDA_DECLARE_STATUS(status, code) extern "C" libreCudaStatus_t status;
#include "librecuda_all_statuess.h"
#undef LIBRECUDA_DECLARE_STATUS


#define LIBRECUDA_SUCCEED() { return LIBRECUDA_SUCCESS; }
#define LIBRECUDA_FAIL(status) { return status; }

#endif //LIBRECUDA_VALIDATE_H
