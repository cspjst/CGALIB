/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "test_cga.h"

#include <stdio.h>
#include <assert.h>

#include "../include/BIOS/bios_version.h"
#include "../include/BIOS/bios_keyboard_services.h"

#include "../include/ENV/env_version.h"
#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_types.h"
#include "../CGA/MED/cga_med_plot.h"



// vendored headers and .LIBs must come from the same build
void test_cga_versions() {
    printf("BIOSLIB headers %s, BIOS.LIB %s\n", BIOS_VERSION_STRING, bios_version_string());
    printf("ENVLIB  headers %s, ENV.LIB  %s\n", ENV_VERSION_STRING,  env_version_string());
    assert(bios_version() == BIOS_VERSION);
    assert(env_version()  == ENV_VERSION);
    printf("Vendored libraries match their headers: PASS\n\n");
}



void test_cga() {
    printf("Test CGALIB...\n");
    //test_cga_versions();
    // test_cga_med_plot();
    // test_cga_hi_plot();
    // test_cga_med_cls();
    // test_cga_hi_cls();
}
