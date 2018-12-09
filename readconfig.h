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
    char BacklightDevice[512];
    char KeyboardDevice[512];
    bool UseKeyboard;
    int MaxBrightness;
    int MinBrightness;
    int DefaultSpeed;
    int DefaultAmount;
};
typedef struct config config;

config DefaultConfig();
void ReadConfig( config* cfg, char* path );

#endif // READCONFIG_H
