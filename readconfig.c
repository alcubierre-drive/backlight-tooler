#include "readconfig.h"

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

typedef struct { char *key; int val; } t_symstruct;
static t_symstruct lookuptable[] = {
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

int keyfromstring(char *key) {
    for (int i=0; i < NKEYS; ++i) {
        if (strcmp(lookuptable[i].key, key) == 0)
            return lookuptable[i].val;
    }
    return BADKEY;
}

char* RemoveString(char* str, const char* sub) {
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

void ModifyConfig( config* cfg, char* line ) {
    if (line[strspn(line, " \t\v\r\n")] == '\0') {
        // ignore empty lines
        return;
    }
    if (line[0] == '#' || (line[0] == '/' && line[1] == '/')) {
        // ignore comments
        return;
    }
    char delimiter[16] = "=\n";
    char keyval[512];
    strcpy(keyval,line);
    char* key;
    char* val;
    key = strtok(keyval,delimiter);
    val = strtok(NULL,delimiter);
    switch(keyfromstring(key)) {
        case W_DEV:
            strcpy(cfg->WebcamDevice, val);
            cfg->has_WebcamDevice = true;
            break;
        case B_DEV:
            strcpy(cfg->BacklightDevice, val);
            cfg->has_BacklightDevice = true;
            break;
        case K_DEV:
            strcpy(cfg->KeyboardDevice, val);
            cfg->has_KeyboardDevice = true;
            break;
        case U_KBD:
            if (strcmp(val,"true") == 0) {
                cfg->UseKeyboard = true;
            } else {
                cfg->UseKeyboard = false;
            }
            cfg->has_UseKeyboard = true;
            break;
        case MAX_B:
            cfg->MaxBrightness = atoi(val);
            cfg->has_MaxBrightness = true;
            break;
        case MIN_B:
            cfg->MinBrightness = atoi(val);
            cfg->has_MinBrightness = true;
            break;
        case DEF_S:
            cfg->DefaultSpeed = atoi(val);
            cfg->has_DefaultSpeed = true;
            break;
        case DEF_A:
            cfg->DefaultAmount = atoi(val);
            cfg->has_DefaultAmount = true;
            break;
        case WIDTH:
            cfg->WebcamWidth = atoi(val);
            cfg->has_WebcamWidth = true;
            break;
        case HEIGHT:
            cfg->WebcamHeight = atoi(val);
            cfg->has_WebcamHeight = false;
            break;
        case W_LOW:
            cfg->WebcamLightValueLow = atoi(val);
            cfg->has_WebcamLightValueLow = true;
            break;
        case W_HIGH:
            cfg->WebcamLightValueHigh = atoi(val);
            cfg->has_WebcamLightValueHigh = true;
            break;
        case AUTO_VALUE:
            cfg->AutoAmount = atoi(val);
            cfg->has_AutoAmount = true;
            break;
        case PULSE_VALUE:
            cfg->PulseAmount = atoi(val);
            cfg->has_PulseAmount = true;
            break;
        case MAX_PULSE:
            cfg->PulseMax = atoi(val);
            cfg->has_PulseMax = true;
            break;
        case USE_FUNCTION:
            strcpy(cfg->UseFunction,val);
            cfg->has_UseFunction = true;
            break;
        case FUNC_PARAM:
            cfg->FunctionParam = atof(val);
            cfg->has_FunctionParam = true;
            break;
        case KBD_VAL:
            cfg->KeyboardValue = atof(val);
            cfg->has_KeyboardValue = true;
            break;
        case KBD_MAX:
            cfg->KeyboardMaxBrightness = atoi(val);
            cfg->has_KeyboardMaxBrightness = true;
            break;
        case BADKEY:
            fprintf(stderr, "[config] unknown key '%s'.\n", key);
            break;
    }
}

void ReadConfig( config* cfg, char* path ) {
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "[config] could not read '%s'.\n", path);
        return;
    }
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&line, &len, fp)) != -1) {
        ModifyConfig( cfg, line );
    }
    fclose(fp);
}

void ReadMaxBrightness( config* cfg, bool kbd ) {
    char maxbrightnessfile[512];
    if (kbd) {
        strcpy(maxbrightnessfile,cfg->KeyboardDevice);
        sprintf(maxbrightnessfile, "%smax_brightness",
                RemoveString(maxbrightnessfile,"brightness"));
    } else {
        strcpy(maxbrightnessfile,cfg->BacklightDevice);
        sprintf(maxbrightnessfile, "%smax_brightness",
                RemoveString(maxbrightnessfile,"brightness"));
    }
    FILE* mbf = fopen(maxbrightnessfile, "r");
    if (mbf == NULL) {
        if (kbd) {
            cfg->KeyboardMaxBrightness = 2;
        } else {
            cfg->MaxBrightness = 1060;
        }
        fprintf(stderr,"[config] Impossible to read '%s'.\n",maxbrightnessfile);
        return;
    }
    char maxbrval[16];
    fread(maxbrval, 8, 1, mbf);
    fclose(mbf);
    if (kbd) {
        cfg->KeyboardMaxBrightness = atoi(maxbrval);
    } else {
        cfg->MaxBrightness = atoi(maxbrval);
    }
}

void DefaultConfig( config* cfg ) {
    if (!cfg->has_WebcamDevice) {
        strcpy(cfg->WebcamDevice, "/dev/video0");
    }
    if (!cfg->has_BacklightDevice) {
        strcpy(cfg->BacklightDevice,
                "/sys/class/backlight/intel_backlight/brightness");
    }
    if (!cfg->has_KeyboardDevice) {
        strcpy(cfg->KeyboardDevice,
  "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::kbd_backlight/brightness"
        );
    }
    if (!cfg->has_UseKeyboard) {
        cfg->UseKeyboard = false;
    }
    if (!cfg->has_MaxBrightness) {
        ReadMaxBrightness( cfg, false );
    }
    if (!cfg->has_MinBrightness) {
        cfg->MinBrightness = 0.02*cfg->MaxBrightness;
    }
    if (!cfg->has_DefaultAmount) {
        cfg->DefaultAmount = 0.07*cfg->MaxBrightness;
    }
    if (!cfg->has_DefaultSpeed) {
        cfg->DefaultSpeed = 100;
    }
    if (!cfg->has_WebcamWidth) {
        cfg->WebcamWidth = 1280;
    }
    if (!cfg->has_WebcamHeight) {
        cfg->WebcamHeight = 720;
    }
    if (!cfg->has_WebcamLightValueLow) {
        cfg->WebcamLightValueLow = 77;
    }
    if (!cfg->has_WebcamLightValueHigh) {
        cfg->WebcamLightValueHigh = 200;
    }
    if (!cfg->has_AutoAmount) {
        cfg->AutoAmount = 1;
    }
    if (!cfg->has_PulseMax) {
        cfg->PulseMax = 1000;
    }
    if (!cfg->has_PulseAmount) {
        cfg->PulseAmount = 30;
    }
    if (!cfg->has_UseFunction) {
        strcpy(cfg->UseFunction,"linear");
    }
    if (!cfg->has_FunctionParam) {
        cfg->FunctionParam = 1.0;
    }
    if (!cfg->has_KeyboardValue) {
        cfg->KeyboardValue = 0.25;
    }
    if (!cfg->has_KeyboardMaxBrightness) {
        if (cfg->UseKeyboard) {
            ReadMaxBrightness( cfg, true );
        } else {
            cfg->KeyboardMaxBrightness = 0;
        }
    }
}

void dbg_print_config( config* cfg ) {
    fprintf(stderr,
        "%s\n%s\n%s\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%s\n%.2f\n%.2f\n%i\n",
        cfg->WebcamDevice,
        cfg->BacklightDevice,
        cfg->KeyboardDevice,
        cfg->UseKeyboard,
        cfg->MaxBrightness,
        cfg->MinBrightness,
        cfg->DefaultSpeed,
        cfg->DefaultAmount,
        cfg->WebcamWidth,
        cfg->WebcamHeight,
        cfg->WebcamLightValueLow,
        cfg->WebcamLightValueHigh,
        cfg->AutoAmount,
        cfg->PulseAmount,
        cfg->PulseMax,
        cfg->UseFunction,
        cfg->FunctionParam,
        cfg->KeyboardValue,
        cfg->KeyboardMaxBrightness);
}

config InitConfig() {
    config c;
    c.has_WebcamDevice = false;
    c.has_BacklightDevice = false;
    c.has_KeyboardDevice = false;
    c.has_UseKeyboard = false;
    c.has_MaxBrightness = false;
    c.has_MinBrightness = false;
    c.has_DefaultSpeed = false;
    c.has_DefaultAmount = false;
    c.has_WebcamWidth = false;
    c.has_WebcamHeight = false;
    c.has_WebcamLightValueLow = false;
    c.has_WebcamLightValueHigh = false;
    c.has_AutoAmount = false;
    c.has_PulseAmount = false;
    c.has_PulseMax = false;
    c.has_UseFunction = false;
    c.has_FunctionParam = false;
    c.has_KeyboardValue = false;
    c.has_KeyboardMaxBrightness = false;
    return c;
}
