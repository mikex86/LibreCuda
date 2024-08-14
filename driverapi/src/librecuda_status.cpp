#include "librecuda_status.h"
#include "librecuda_status_internal.h"

#include <unordered_map>
#include <string>

#define LIBRECUDA_DECLARE_STATUS(status, code) libreCudaStatus_t status = code;

#include "librecuda_all_statuses.h"

#undef LIBRECUDA_DECLARE_STATUS


std::unordered_map<int, std::string> status_to_name = {};
static bool initialized = false;

void internalLibreCuInitStatusNames() {

#define LIBRECUDA_DECLARE_STATUS(status, code) status_to_name[code] = #status;

#include "librecuda_all_statuses.h"

#undef LIBRECUDA_DECLARE_STATUS

    initialized = true;
}

const char *internalLibreCuGetStatusName(int code) {
    auto it = status_to_name.find(code);
    if (it == status_to_name.end()) {
        return nullptr;
    }
    return it->second.c_str();
}

bool internalLibreCuInitStatusNamesInitialized() {
    return initialized;
}