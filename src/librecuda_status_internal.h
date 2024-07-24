#ifndef LIBRECUDA_LIBRECUDA_STATUS_INTERNAL_H
#define LIBRECUDA_LIBRECUDA_STATUS_INTERNAL_H

void internalLibreCuInitStatusNames();

const char *internalLibreCuGetStatusName(int code);

bool internalLibreCuInitStatusNamesInitialized();

#endif //LIBRECUDA_LIBRECUDA_STATUS_INTERNAL_H
