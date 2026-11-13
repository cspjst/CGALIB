#include "cga_version.h"
#include "../include/BIOS/bios_version.h"

#if BIOS_VERSION < CGA_MIN_BIOS_VERSION
#error vendored BIOSLIB headers incorrect version
#pragma message("found BIOSLIB " BIOS_VERSION_STRING)
#endif

unsigned int cga_version() {
    return CGA_VERSION;
}

const char* cga_version_string() {
    return CGA_VERSION_STRING;
}
