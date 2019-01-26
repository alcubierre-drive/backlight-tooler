#ifndef READCONFIG_H
#define READCONFIG_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

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

typedef struct cf_char {
    char val[512];
    bool has;
} cf_char;
typedef struct cf_int {
    int val;
    bool has;
} cf_int;
typedef struct cf_float {
    float val;
    bool has;
} cf_float;
typedef struct cf_bool {
    bool val;
    bool has;
} cf_bool;

struct config {
    cf_char WebcamDevice;
    cf_char BacklightDevice;
    cf_char KeyboardDevice;
    cf_bool UseKeyboard;
    cf_int MaxBrightness;
    cf_int MinBrightness;
    cf_int DefaultSpeed;
    cf_int DefaultAmount;
    cf_int WebcamWidth;
    cf_int WebcamHeight;
    cf_int WebcamLightValueLow;
    cf_int WebcamLightValueHigh;
    cf_int AutoAmount;
    cf_int PulseAmount;
    cf_int PulseMax;
    cf_char UseFunction;
    cf_float FunctionParam;
    cf_float KeyboardValue;
    cf_float KeyboardMaxBrightness;
};
typedef struct config config;

typedef struct { char *key; int val; } t_symstruct;
static const t_symstruct lookuptable[] = {
    { "WebcamDevice", W_DEV },
    { "BacklightDevice", B_DEV },
    { "KeyboardDevice", K_DEV },
    { "UseKeyboard", U_KBD },
    { "MaxBrightness", MAX_B },
    { "MinBrightness", MIN_B },
    { "DefaultSpeed", DEF_S },
    { "DefaultAmount", DEF_A },
    { "AutoAmount", AUTO_VALUE },
    { "WebcamWidth", WIDTH },
    { "WebcamHeight", HEIGHT },
    { "WebcamLightValueLow", W_LOW },
    { "WebcamLightValueHigh", W_HIGH },
    { "PulseAmount", PULSE_VALUE },
    { "PulseMax", MAX_PULSE },
    { "Function", USE_FUNCTION },
    { "FuncParam", FUNC_PARAM },
    { "KeyboardValue", KBD_VAL },
    { "KeyboardMaxBrightness", KBD_MAX }
};

#define NKEYS sizeof(lookuptable)/sizeof(t_symstruct)

config InitConfig();
void ReadConfig( config*, char* path );
void DefaultConfig( config* );
void dbg_print_config( config* );

#endif // READCONFIG_H
