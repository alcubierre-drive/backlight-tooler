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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int getBrightness(config* );
void setBrightness(config*, int target);

void inc(config* cfg, int amount) {
    int brightness = getBrightness(cfg) + amount;
    brightness = brightness >cfg->MaxBrightness?cfg->MaxBrightness : brightness;
    setBrightness(cfg, brightness);
}

void dec(config* cfg, int amount) {
    int brightness = getBrightness(cfg) - amount;
    brightness = brightness <cfg->MinBrightness?cfg->MinBrightness : brightness;
    setBrightness(cfg, brightness);
}

void writeBrightness(config* cfg, int target) {
    if (target < cfg->MinBrightness) target = cfg->MinBrightness;
    if (target > cfg->MaxBrightness) target = cfg->MaxBrightness;
    char buffer[256];
    sprintf(buffer, "%i", target);
    FILE* file = fopen(cfg->BacklightDevice, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",
                cfg->BacklightDevice);
        exit(1);
    }
    fwrite(buffer, 4, 1, file);
    fclose(file);
}

void writeBrightnessUnbound(config* cfg, int target) {
    if (target < 0) target = 0;
    if (target > cfg->MaxBrightness) target = cfg->MaxBrightness;
    char buffer[256];
    sprintf(buffer, "%i", target);
    FILE* file = fopen(cfg->BacklightDevice, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",
                cfg->BacklightDevice);
        exit(1);
    }
    fwrite(buffer, 4, 1, file);
    fclose(file);
}

void setBrightness(config* cfg, int target) {
    if (target < cfg->MinBrightness) target = cfg->MinBrightness;
    if (target > cfg->MaxBrightness) target = cfg->MaxBrightness;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightness(cfg, current);
        usleep(20000/cfg->DefaultSpeed);
    }
}

void setBrightnessUnbound(config* cfg, int target) {
    if (target < 0) target = 0;
    if (target > cfg->MaxBrightness) target = cfg->MaxBrightness;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightnessUnbound(cfg, current);
        usleep(20000/cfg->DefaultSpeed);
    }
}

void setBrightnessKeyboard(config* cfg, char* val) {
    FILE* file = fopen(cfg->KeyboardDevice, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",
                cfg->KeyboardDevice);
        exit(1);
    }
    fprintf(file, val);
    fclose(file);
}

void pulse(config* cfg, int amount) {
    int low = 1;
    int high = cfg->MaxBrightness;
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
    FILE* file = fopen(cfg->BacklightDevice, "r");
    if (file == NULL) {
        fprintf(stderr, "[main] could not read '%s'\n.",
                cfg->BacklightDevice);
        exit(1);
    }
    char buf[16];
    fread(buf, 8, 1, file);
    fclose(file);
    return atoi(buf);
}

void autoBrightness(config* cfg, int amount) {
    int target = getLightLevel(cfg)*amount;
    target -= cfg->WebcamLightValueLow;
    target *= cfg->MaxBrightness;
    target /= cfg->WebcamLightValueHigh;
    setBrightness(cfg, target);
    if (cfg->UseKeyboard) {
        if (target < 0.25*cfg->MaxBrightness) {
            if (target < 0.125*cfg->MaxBrightness) {
                setBrightnessKeyboard(cfg, "1");
            }
            else {
                setBrightnessKeyboard(cfg, "2");
            }
        }
        else {
            setBrightnessKeyboard(cfg, "0");
        }
    }
}

void help() {
    puts("Usage: BacklightTool OPTION [AMOUNT]\n"\
         "Options: set, inc, dec, pulse, auto, toggle");
}

void toggleBrightness(config* cfg) {
    int curr = getBrightness(cfg);
    if (curr != 0) {
        setBrightnessUnbound(cfg, 0);
        if (cfg->UseKeyboard) {
            setBrightnessKeyboard(cfg, "0");
        }
    } else {
        setBrightnessUnbound(cfg, cfg->MaxBrightness);
        if (cfg->UseKeyboard) {
            setBrightnessKeyboard(cfg, "0");
        }
    }
}

int main(int argc, char **argv) {
    config cfg;
    ReadConfig(&cfg, "/etc/BacklightTooler.conf");
    DefaultConfig(&cfg);

    if (argc < 2) {
        help();
        return 0;
    }

    int amount = cfg.DefaultAmount;
    if (argc > 2) amount = atoi(argv[2]);
    if (amount < 0 || amount > cfg.MaxBrightness) {
        puts("Invalid amount.\n");
        return 0;
    }

    if (!strcmp(argv[1], "inc")) inc(&cfg, amount);
    else if (!strcmp(argv[1], "dec")) dec(&cfg, amount);
    else if (!strcmp(argv[1], "pulse")) pulse(&cfg, amount);
    else if (!strcmp(argv[1], "auto")) autoBrightness(&cfg, amount);
    else if (!strcmp(argv[1], "set") && argc > 2) setBrightness(&cfg, amount);
    else if (!strcmp(argv[1], "toggle")) toggleBrightness(&cfg);
    else help();
}
