#ifndef LIBRECUDA_VALIDATE_H
#define LIBRECUDA_VALIDATE_H

#include <iostream>

typedef int libreCudaStatus_t;

#define LIBRECUDA_DEBUG(msg) std::cerr << "[LibreCuda Debug]: " << msg << std::endl
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define LIBRECUDA_VALIDATE(condition, err) { \
    if (!(condition)) {                      \
        LIBRECUDA_DEBUG(__FILE__ ":" TOSTRING(__LINE__) ": " #condition);         \
        return err;                          \
    }                                        \
}

// declare all cuda error codes
#define LIBRECUDA_DECLARE_STATUS(status, code) extern "C" libreCudaStatus_t status;
#include "librecuda_all_statuess.h"
#undef LIBRECUDA_DECLARE_STATUS

#define LIBRECUDA_SUCCEED() { return LIBRECUDA_SUCCESS; }
#define LIBRECUDA_FAIL(status) { return status; }

#define LIBRECUDA_ERR_PROPAGATE(status) { libreCudaStatus_t status_val = status; if (status_val != LIBRECUDA_SUCCESS) { LIBRECUDA_FAIL(status_val); } }

#endif //LIBRECUDA_VALIDATE_H
