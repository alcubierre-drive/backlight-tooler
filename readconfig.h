#ifndef READCONFIG_H
#define READCONFIG_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

struct config {
    char WebcamDevice[512];
    bool has_WebcamDevice;
    char BacklightDevice[512];
    bool has_BacklightDevice;
    char KeyboardDevice[512];
    bool has_KeyboardDevice;
    bool UseKeyboard;
    bool has_UseKeyboard;
    int MaxBrightness;
    bool has_MaxBrightness;
    int MinBrightness;
    bool has_MinBrightness;
    int DefaultSpeed;
    bool has_DefaultSpeed;
    int DefaultAmount;
    bool has_DefaultAmount;
    int WebcamWidth;
    bool has_WebcamWidth;
    int WebcamHeight;
    bool has_WebcamHeight;
    int WebcamLightValueLow;
    bool has_WebcamLightValueLow;
    int WebcamLightValueHigh;
    bool has_WebcamLightValueHigh;
    int AutoAmount;
    bool has_AutoAmount;
    int PulseAmount;
    bool has_PulseAmount;
    int PulseMax;
    bool has_PulseMax;
    char UseFunction[512];
    bool has_UseFunction;
    float FunctionParam;
    bool has_FunctionParam;
    float KeyboardValue;
    bool has_KeyboardValue;
    float KeyboardMaxBrightness;
    bool has_KeyboardMaxBrightness;
};
typedef struct config config;

config InitConfig();
void ReadConfig( config*, char* path );
void DefaultConfig( config* );
void dbg_print_config( config* );

#endif // READCONFIG_H
