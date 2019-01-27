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

#define CHAR_TYPE 1
#define INT_TYPE 2
#define FLOAT_TYPE 3
#define BOOL_TYPE 4

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

typedef struct {
    char *key;
    int val;
    int type;
} t_symstruct;
static const t_symstruct lookuptable[] = {
    { "WebcamDevice", W_DEV, CHAR_TYPE },
    { "BacklightDevice", B_DEV, CHAR_TYPE },
    { "KeyboardDevice", K_DEV, CHAR_TYPE },
    { "UseKeyboard", U_KBD, BOOL_TYPE },
    { "MaxBrightness", MAX_B, INT_TYPE },
    { "MinBrightness", MIN_B, INT_TYPE },
    { "DefaultSpeed", DEF_S, INT_TYPE },
    { "DefaultAmount", DEF_A, INT_TYPE },
    { "AutoAmount", AUTO_VALUE, INT_TYPE },
    { "WebcamWidth", WIDTH, INT_TYPE },
    { "WebcamHeight", HEIGHT, INT_TYPE },
    { "WebcamLightValueLow", W_LOW, INT_TYPE },
    { "WebcamLightValueHigh", W_HIGH, INT_TYPE },
    { "PulseAmount", PULSE_VALUE, INT_TYPE },
    { "PulseMax", MAX_PULSE, INT_TYPE },
    { "Function", USE_FUNCTION, CHAR_TYPE },
    { "FuncParam", FUNC_PARAM, FLOAT_TYPE },
    { "KeyboardValue", KBD_VAL, FLOAT_TYPE },
    { "KeyboardMaxBrightness", KBD_MAX, FLOAT_TYPE }
};

#define NKEYS sizeof(lookuptable)/sizeof(t_symstruct)

config InitConfig();
void ReadConfig( config*, char* path );
void DefaultConfig( config* );
void dbg_print_config( config* );

#endif // READCONFIG_H
