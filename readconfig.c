#include "readconfig.h"

int keyfromstring(char* key, int* type) {
    for (int i=0; i < NKEYS; ++i) {
        if (strcmp(lookuptable[i].key, key) == 0)
            *type = lookuptable[i].type;
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

void cf_char_set( cf_char x, char* y ) {
    strcpy(x.val,y);
    x.has = true;
}
void cf_int_set( cf_int x, char* y ) {
    x.val = atoi(y);
    x.has = true;
}
void cf_float_set( cf_float x, char* y ) {
    x.val = atof(y);
    x.has = true;
}
void cf_bool_set( cf_bool x, char* y ) {
    if (strcmp(y,"true") == 0) {
        x.val = true;
    } else {
        x.val = false;
    }
    x.has = true;
}
void cf_set( void* structure, char* val, int type ) {
    switch(type) {
        case CHAR_TYPE:
            cf_char_set( *((cf_char*)structure), val );
            break;
        case INT_TYPE:
            cf_int_set( *((cf_int*)structure), val );
            break;
        case FLOAT_TYPE:
            cf_float_set( *((cf_float*)structure), val );
            break;
        case BOOL_TYPE:
            cf_bool_set( *((cf_bool*)structure), val );
            break;
    }
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
    int type = BADKEY;
    switch( keyfromstring(key, &type) ) {
        case W_DEV:        cf_set(&cfg->WebcamDevice,          val, type); break;
        case B_DEV:        cf_set(&cfg->BacklightDevice,       val, type); break;
        case K_DEV:        cf_set(&cfg->KeyboardDevice,        val, type); break;
        case U_KBD:        cf_set(&cfg->UseKeyboard,           val, type); break;
        case MAX_B:        cf_set(&cfg->MaxBrightness,         val, type); break;
        case MIN_B:        cf_set(&cfg->MinBrightness,         val, type); break;
        case DEF_S:        cf_set(&cfg->DefaultSpeed,          val, type); break;
        case DEF_A:        cf_set(&cfg->DefaultAmount,         val, type); break;
        case WIDTH:        cf_set(&cfg->WebcamWidth,           val, type); break;
        case HEIGHT:       cf_set(&cfg->WebcamHeight,          val, type); break;
        case W_LOW:        cf_set(&cfg->WebcamLightValueLow,   val, type); break;
        case W_HIGH:       cf_set(&cfg->WebcamLightValueHigh,  val, type); break;
        case AUTO_VALUE:   cf_set(&cfg->AutoAmount,            val, type); break;
        case PULSE_VALUE:  cf_set(&cfg->PulseAmount,           val, type); break;
        case MAX_PULSE:    cf_set(&cfg->PulseMax,              val, type); break;
        case USE_FUNCTION: cf_set(&cfg->UseFunction,           val, type); break;
        case FUNC_PARAM:   cf_set(&cfg->FunctionParam,         val, type); break;
        case KBD_VAL:      cf_set(&cfg->KeyboardValue,         val, type); break;
        case KBD_MAX:      cf_set(&cfg->KeyboardMaxBrightness, val, type); break;
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
        strcpy(maxbrightnessfile,cfg->KeyboardDevice.val);
        sprintf(maxbrightnessfile, "%smax_brightness",
                RemoveString(maxbrightnessfile,"brightness"));
    } else {
        strcpy(maxbrightnessfile,cfg->BacklightDevice.val);
        sprintf(maxbrightnessfile, "%smax_brightness",
                RemoveString(maxbrightnessfile,"brightness"));
    }
    FILE* mbf = fopen(maxbrightnessfile, "r");
    if (mbf == NULL) {
        if (kbd) {
            cfg->KeyboardMaxBrightness.val = 2;
        } else {
            cfg->MaxBrightness.val = 1060;
        }
        fprintf(stderr,"[config] Impossible to read '%s'.\n",maxbrightnessfile);
        return;
    }
    char maxbrval[16];
    fread(maxbrval, 8, 1, mbf);
    fclose(mbf);
    if (kbd) {
        cfg->KeyboardMaxBrightness.val = atoi(maxbrval);
    } else {
        cfg->MaxBrightness.val = atoi(maxbrval);
    }
}

void DefaultConfig( config* cfg ) {
    if (!cfg->WebcamDevice.has) {
        strcpy(cfg->WebcamDevice.val, "/dev/video0");
    }
    if (!cfg->BacklightDevice.has) {
        strcpy(cfg->BacklightDevice.val,
                "/sys/class/backlight/intel_backlight/brightness");
    }
    if (!cfg->KeyboardDevice.has) {
        strcpy(cfg->KeyboardDevice.val,
  "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::kbd_backlight/brightness"
        );
    }
    if (!cfg->UseKeyboard.has) {
        cfg->UseKeyboard.val = false;
    }
    if (!cfg->MaxBrightness.has) {
        ReadMaxBrightness( cfg, false );
    }
    if (!cfg->MinBrightness.has) {
        cfg->MinBrightness.val = 0.02*cfg->MaxBrightness.val;
    }
    if (!cfg->DefaultAmount.has) {
        cfg->DefaultAmount.val = 0.07*cfg->MaxBrightness.val;
    }
    if (!cfg->DefaultSpeed.has) {
        cfg->DefaultSpeed.val = 100;
    }
    if (!cfg->WebcamWidth.has) {
        cfg->WebcamWidth.val = 1280;
    }
    if (!cfg->WebcamHeight.has) {
        cfg->WebcamHeight.val = 720;
    }
    if (!cfg->WebcamLightValueLow.has) {
        cfg->WebcamLightValueLow.val = 77;
    }
    if (!cfg->WebcamLightValueHigh.has) {
        cfg->WebcamLightValueHigh.val = 200;
    }
    if (!cfg->AutoAmount.has) {
        cfg->AutoAmount.val = 1;
    }
    if (!cfg->PulseMax.has) {
        cfg->PulseMax.val = 1000;
    }
    if (!cfg->PulseAmount.has) {
        cfg->PulseAmount.val = 30;
    }
    if (!cfg->UseFunction.has) {
        strcpy(cfg->UseFunction.val,"linear");
    }
    if (!cfg->FunctionParam.has) {
        cfg->FunctionParam.val = 1.0;
    }
    if (!cfg->KeyboardValue.has) {
        cfg->KeyboardValue.val = 0.25;
    }
    if (!cfg->KeyboardMaxBrightness.has) {
        if (cfg->UseKeyboard.val) {
            ReadMaxBrightness( cfg, true );
        } else {
            cfg->KeyboardMaxBrightness.val = 0;
        }
    }
}

void dbg_print_config( config* cfg ) {
    fprintf(stderr,
        "%s\n%s\n%s\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%i\n%s\n%.2f\n%.2f\n%i\n",
        cfg->WebcamDevice.val,
        cfg->BacklightDevice.val,
        cfg->KeyboardDevice.val,
        cfg->UseKeyboard.val,
        cfg->MaxBrightness.val,
        cfg->MinBrightness.val,
        cfg->DefaultSpeed.val,
        cfg->DefaultAmount.val,
        cfg->WebcamWidth.val,
        cfg->WebcamHeight.val,
        cfg->WebcamLightValueLow.val,
        cfg->WebcamLightValueHigh.val,
        cfg->AutoAmount.val,
        cfg->PulseAmount.val,
        cfg->PulseMax.val,
        cfg->UseFunction.val,
        cfg->FunctionParam.val,
        cfg->KeyboardValue.val,
        cfg->KeyboardMaxBrightness.val);
}

config InitConfig() {
    config c;
    c.WebcamDevice.has = false;
    c.BacklightDevice.has = false;
    c.KeyboardDevice.has = false;
    c.UseKeyboard.has = false;
    c.MaxBrightness.has = false;
    c.MinBrightness.has = false;
    c.DefaultSpeed.has = false;
    c.DefaultAmount.has = false;
    c.WebcamWidth.has = false;
    c.WebcamHeight.has = false;
    c.WebcamLightValueLow.has = false;
    c.WebcamLightValueHigh.has = false;
    c.AutoAmount.has = false;
    c.PulseAmount.has = false;
    c.PulseMax.has = false;
    c.UseFunction.has = false;
    c.FunctionParam.has = false;
    c.KeyboardValue.has = false;
    c.KeyboardMaxBrightness.has = false;
    return c;
}
