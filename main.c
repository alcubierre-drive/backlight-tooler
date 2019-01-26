/*
 * Copyright (c) 2018 alcubierre-drive, Cotix.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "readconfig.h"
#include "webcam.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CONFIG_FILE "/etc/backlight-tooler.conf"

int getBrightness(config* );
void setBrightness(config*, int target);

void inc(config* cfg, int amount) {
    int brightness = getBrightness(cfg) + amount;
    brightness = brightness >cfg->MaxBrightness.val?cfg->MaxBrightness.val : brightness;
    setBrightness(cfg, brightness);
}

void dec(config* cfg, int amount) {
    int brightness = getBrightness(cfg) - amount;
    brightness = brightness <cfg->MinBrightness.val?cfg->MinBrightness.val : brightness;
    setBrightness(cfg, brightness);
}

void writeBrightness(config* cfg, int target) {
    if (target < cfg->MinBrightness.val) target = cfg->MinBrightness.val;
    if (target > cfg->MaxBrightness.val) target = cfg->MaxBrightness.val;
    char buffer[256];
    sprintf(buffer, "%i", target);
    FILE* file = fopen(cfg->BacklightDevice.val, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",
                cfg->BacklightDevice.val);
        exit(1);
    }
    fwrite(buffer, 4, 1, file);
    fclose(file);
}

void writeBrightnessUnbound(config* cfg, int target) {
    if (target < 0) target = 0;
    if (target > cfg->MaxBrightness.val) target = cfg->MaxBrightness.val;
    char buffer[256];
    sprintf(buffer, "%i", target);
    FILE* file = fopen(cfg->BacklightDevice.val, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",
                cfg->BacklightDevice.val);
        exit(1);
    }
    fwrite(buffer, 4, 1, file);
    fclose(file);
}

void setBrightness(config* cfg, int target) {
    if (target < cfg->MinBrightness.val) target = cfg->MinBrightness.val;
    if (target > cfg->MaxBrightness.val) target = cfg->MaxBrightness.val;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightness(cfg, current);
        usleep(20000/cfg->DefaultSpeed.val);
    }
}

void setBrightnessUnbound(config* cfg, int target) {
    if (target < 0) target = 0;
    if (target > cfg->MaxBrightness.val) target = cfg->MaxBrightness.val;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightnessUnbound(cfg, current);
        usleep(20000/cfg->DefaultSpeed.val);
    }
}

void setBrightnessKeyboard(config* cfg, char* val) {
    FILE* file = fopen(cfg->KeyboardDevice.val, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",
                cfg->KeyboardDevice.val);
        exit(1);
    }
    fprintf(file, val);
    fclose(file);
}

void pulse(config* cfg, int amount) {
    int low = 1;
    int high = cfg->MaxBrightness.val;
    int t;
    while (1){
        for (t = low; t < high; ++t) {
            writeBrightness(cfg, t);
            usleep(20000/amount);
        }
        for (t = high; t > low; --t) {
            writeBrightness(cfg, t);
            usleep(20000/amount);
        }
    }
}

int getBrightness(config* cfg) {
    FILE* file = fopen(cfg->BacklightDevice.val, "r");
    if (file == NULL) {
        fprintf(stderr, "[main] could not read '%s'\n.",
                cfg->BacklightDevice.val);
        exit(1);
    }
    char buf[16];
    fread(buf, 8, 1, file);
    fclose(file);
    return atoi(buf);
}

void autoBrightness(config* cfg, int amount) {
    int temp_amount = getLightLevel(cfg)*amount;
    int target = choose_function_and_params(cfg, temp_amount);

    if (target < 0) {
        target = 0;
    } else if (target > cfg->MaxBrightness.val) {
        target = cfg->MaxBrightness.val;
    }

    setBrightness(cfg, target);
    if (cfg->UseKeyboard.val) {
        if (target <= cfg->KeyboardValue.val*cfg->MaxBrightness.val) {
            float tmp = target;
            tmp -= cfg->KeyboardValue.val*cfg->MaxBrightness.val;
            tmp /= (float)(cfg->KeyboardValue.val*cfg->MaxBrightness.val);
            int kbdbr = (int)((1.0+tmp)*cfg->KeyboardMaxBrightness.val+1.0);
            if (kbdbr > cfg->KeyboardMaxBrightness.val) {
                kbdbr = cfg->KeyboardMaxBrightness.val;
            }
            char buffer[256];
            sprintf( buffer, "%i", kbdbr );
            setBrightnessKeyboard(cfg, buffer);
        } else {
            setBrightnessKeyboard(cfg, "0");
        }
    }
}

void help( char** argv ) {
    printf("Usage: %s <option> [amount]\n"\
           "           options:  set [amount]\n"\
           "                     inc [amount]\n"\
           "                     dec [amount]\n"\
           "                     pulse [amount]\n"\
           "                     auto [amount]\n"\
           "                     toggle\n"\
           "                     info\n",
         argv[0]);
}

void toggleBrightness(config* cfg) {
    int curr = getBrightness(cfg);
    if (curr != 0) {
        setBrightnessUnbound(cfg, 0);
        if (cfg->UseKeyboard.val) {
            setBrightnessKeyboard(cfg, "0");
        }
    } else {
        setBrightnessUnbound(cfg, cfg->MaxBrightness.val);
        if (cfg->UseKeyboard.val) {
            setBrightnessKeyboard(cfg, "0");
        }
    }
}

int chka(config* cfg, int amount) {
    if (amount < 0 || amount > cfg->MaxBrightness.val) {
        fprintf(stderr, "[main] Invalid amount '%i'. Must be within [0,%i].\n",
                amount, cfg->MaxBrightness.val);
        fprintf(stderr, "[main] Using default '%i'.\n", cfg->DefaultAmount.val);
        return cfg->DefaultAmount.val;
    } else {
        return amount;
    }
}

int chkp(config* cfg, int amount) {
    if (amount < 0 || amount > cfg->PulseMax.val) {
        fprintf(stderr, "[main] Invalid amount '%i'. Must be within [0,%i].\n",
                amount, cfg->PulseMax.val);
        fprintf(stderr, "[main] Using default '%i'.\n", cfg->PulseAmount.val);
        return cfg->PulseAmount.val;
    } else {
        return amount;
    }
}

int main(int argc, char **argv) {
    config cfg = InitConfig();
    ReadConfig(&cfg, CONFIG_FILE);
    DefaultConfig(&cfg);

    if (argc < 2) {
        help( argv );
        return 0;
    }

    int set_amount = cfg.DefaultAmount.val;
    int auto_amount = cfg.AutoAmount.val;
    int pulse_amount = cfg.PulseAmount.val;
    if (argc > 2) {
        set_amount = atoi(argv[2]);
        auto_amount = atoi(argv[2]);
        pulse_amount = atoi(argv[2]);
    }

    if (!strcmp(argv[1], "inc")) {
        inc(&cfg, chka(&cfg, set_amount));
    } else if (!strcmp(argv[1], "dec")) {
        dec(&cfg, chka(&cfg, set_amount));
    } else if (!strcmp(argv[1], "pulse")) {
        pulse(&cfg, chkp(&cfg, pulse_amount));
    } else if (!strcmp(argv[1], "auto")) {
        autoBrightness(&cfg, auto_amount);
    } else if (!strcmp(argv[1], "set") && argc > 2) {
            setBrightness(&cfg, chka(&cfg, set_amount));
    } else if (!strcmp(argv[1], "toggle")) {
        toggleBrightness(&cfg);
    } else if (!strcmp(argv[1], "info")) {
        int level = getLightLevel(&cfg);
        fprintf(stderr,"[info] Current webcam light level: %i\n",level);
        dbg_print_config( &cfg );
    } else { help( argv ); }
}
