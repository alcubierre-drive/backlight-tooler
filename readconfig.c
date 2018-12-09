#include "readconfig.h"

#define W_DEV 0
#define B_DEV 1
#define K_DEV 2
#define U_KBD 3
#define MAX_B 4
#define MIN_B 5
#define DEF_S 6
#define DEF_A 7
#define BADKEY -1
#define NKEYS 8

typedef struct { char *key; int val; } t_symstruct;
static t_symstruct lookuptable[NKEYS] = {
    { "WebcamDevice", W_DEV },
    { "BacklightDevice", B_DEV },
    { "KeyboardDevice", K_DEV },
    { "UseKeyboard", U_KBD },
    { "MaxBrightness", MAX_B },
    { "MinBrightness", MIN_B },
    { "DefaultSpeed", DEF_S },
    { "DefaultAmount", DEF_A }
};

int keyfromstring(char *key) {
    for (int i=0; i < NKEYS; ++i) {
        if (strcmp(lookuptable[i].key, key) == 0)
            return lookuptable[i].val;
    }
    return BADKEY;
}

config DefaultConfig() {
    config cfg;
    strcpy(cfg.WebcamDevice, "/dev/video0");
    strcpy(cfg.BacklightDevice, "/sys/class/backlight/intel_backlight/brightness");
    strcpy(cfg.KeyboardDevice, "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::kbd_backlight/brightness");
    cfg.UseKeyboard = false;
    cfg.MaxBrightness = 1060;
    cfg.MinBrightness = 50;
    cfg.DefaultSpeed = 100;
    cfg.DefaultAmount = 106;
    return cfg;
}

void ModifyConfig( config* cfg, char* line ) {
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
        case B_DEV:
            strcpy(cfg->BacklightDevice, val);
        case K_DEV:
            strcpy(cfg->KeyboardDevice, val);
        case U_KBD:
            if (strcmp(val,"true") == 0) {
                cfg->UseKeyboard = true;
            } else {
                cfg->UseKeyboard = false;
            }
        case MAX_B:
            cfg->MaxBrightness = atoi(val);
        case MIN_B:
            cfg->MinBrightness = atoi(val);
        case DEF_S:
            cfg->DefaultSpeed = atoi(val);
        case DEF_A:
            cfg->DefaultAmount = atoi(val);
        case BADKEY:
            break;
    }
}

void ReadConfig( config* cfg, char* path ) {
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
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
