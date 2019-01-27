#ifndef READCONFIG_H
#define READCONFIG_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    char val[512];
    bool has;
} cf_char;
typedef struct {
    int val;
    bool has;
} cf_int;
typedef struct {
    float val;
    bool has;
} cf_float;
typedef struct {
    bool val;
    bool has;
} cf_bool;

typedef struct {
    char c[512];
    int i;
    bool b;
    float f;
} defaults;

typedef struct {
    char *key;
    int val;
    int type;
    defaults def;
} t_symstruct;

#define CHAR_TYPE 1
#define INT_TYPE 2
#define FLOAT_TYPE 3
#define BOOL_TYPE 4

#define W_DEV 0
#define B_DEV 1
#define K_DEV 2
#define U_KBD 3
#define MAX_B 4
#define MIN_B 5
#define DEF_S 6
#define DEF_A 7
#define WIDTH 8
#define HEIGHT 9
#define W_LOW 10
#define W_HIGH 11
#define AUTO_VALUE 12
#define PULSE_VALUE 13
#define MAX_PULSE 14
#define USE_FUNCTION 15
#define FUNC_PARAM 16
#define KBD_VAL 17
#define KBD_MAX 18
#define BADKEY -1

#ifdef USE_LOOKUP_TABLES

const defaults defaultstable[] = {
    { "/dev/video0", 0, 0, 0 },
    { "/sys/class/backlight/intel_backlight/brightness", 0, 0, 0 },
    { "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::kbd_backlight/brightness", 0, 0, 0},
    { "", 0, false, 0 },
    { "", 1000, 0, 0 },
    { "", 10, 0, 0 },
    { "", 100, 0, 0 },
    { "", 100, 0, 0 },
    { "", 1, 0, 0 },
    { "", 1280, 0, 0 },
    { "", 720, 0, 0 },
    { "", 77, 0, 0 },
    { "", 200, 0, 0 },
    { "", 30, 0, 0 },
    { "", 1060, 0, 0 },
    { "linear", 0, 0, 0 },
    { "", 0, 0, 1.0 },
    { "", 0, 0, 0.25 },
    { "", 2, 0, 0 }
};
const t_symstruct lookuptable[] = {
    { "WebcamDevice", W_DEV, CHAR_TYPE, defaultstable[0] },
    { "BacklightDevice", B_DEV, CHAR_TYPE, defaultstable[1] },
    { "KeyboardDevice", K_DEV, CHAR_TYPE, defaultstable[2] },
    { "UseKeyboard", U_KBD, BOOL_TYPE, defaultstable[3] },
    { "MaxBrightness", MAX_B, INT_TYPE, defaultstable[4] },
    { "MinBrightness", MIN_B, INT_TYPE, defaultstable[5] },
    { "DefaultSpeed", DEF_S, INT_TYPE, defaultstable[6] },
    { "DefaultAmount", DEF_A, INT_TYPE, defaultstable[7] },
    { "AutoAmount", AUTO_VALUE, INT_TYPE, defaultstable[8] },
    { "WebcamWidth", WIDTH, INT_TYPE, defaultstable[9] },
    { "WebcamHeight", HEIGHT, INT_TYPE, defaultstable[10] },
    { "WebcamLightValueLow", W_LOW, INT_TYPE, defaultstable[11] },
    { "WebcamLightValueHigh", W_HIGH, INT_TYPE, defaultstable[12] },
    { "PulseAmount", PULSE_VALUE, INT_TYPE, defaultstable[13] },
    { "PulseMax", MAX_PULSE, INT_TYPE, defaultstable[14] },
    { "Function", USE_FUNCTION, CHAR_TYPE, defaultstable[15] },
    { "FuncParam", FUNC_PARAM, FLOAT_TYPE, defaultstable[16] },
    { "KeyboardValue", KBD_VAL, FLOAT_TYPE, defaultstable[17] },
    { "KeyboardMaxBrightness", KBD_MAX, INT_TYPE, defaultstable[18] }
};
#define NKEYS sizeof(lookuptable)/sizeof(t_symstruct)

#endif

void read_configuration_file( void**, char* path );
void default_config( void** );
void init_config(void**);
void dbg_cnf(void**);
void delete_config(void**);
int get_keynum();
void read_config(void** cfg, int which, void* out);

#endif // READCONFIG_H
