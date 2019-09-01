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
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "readconfig.h"
#include "webcam.h"
#include "functions.h"

#define CONFIG_FILE "/etc/backlight-tooler.conf"

int getBrightness( void** );
void setBrightness( void**, int target);

static void usleep( size_t usec ) {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = usec * 1000L;
    nanosleep(&req, NULL);
}

void inc(void** cfg, int amount) {
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);

    int brightness = getBrightness(cfg) + amount;
    brightness = brightness > maxbr ? maxbr : brightness;
    setBrightness(cfg, brightness);
}

void dec(void** cfg, int amount) {
    int minbr;
    read_config(cfg,MIN_B,&minbr);

    int brightness = getBrightness(cfg) - amount;
    brightness = brightness < minbr ? minbr : brightness;
    setBrightness(cfg, brightness);
}

void inc_percent(void** cfg, float percent) {
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);

    int brightness = getBrightness(cfg) + (int)((float)maxbr*percent);
    brightness = brightness > maxbr ? maxbr : brightness;
    setBrightness(cfg, brightness);
}

void dec_percent(void** cfg, float percent) {
    int minbr;
    read_config(cfg,MIN_B,&minbr);
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);

    int brightness = getBrightness(cfg) - (int)((float)maxbr*percent);
    brightness = brightness < minbr ? minbr : brightness;
    setBrightness(cfg, brightness);
}

void writeBrightness(void** cfg, int target) {
    int minbr, maxbr;
    read_config(cfg,MIN_B,&minbr);
    read_config(cfg,MAX_B,&maxbr);
    char name[512];
    read_config(cfg,B_DEV,name);

    if (target < minbr) target = minbr;
    if (target > maxbr) target = minbr;
    char buffer[256];
    sprintf(buffer, "%i", target);
    FILE* file = fopen(name, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",name);
        exit(1);
    }
    fwrite(buffer, 4, 1, file);
    fclose(file);
}

void writeBrightnessUnbound(void** cfg, int target) {
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);
    char name[512];
    read_config(cfg,B_DEV,name);

    if (target < 0) target = 0;
    if (target > maxbr) target = maxbr;
    char buffer[256];
    sprintf(buffer, "%i", target);
    FILE* file = fopen(name, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.", name);
        exit(1);
    }
    fwrite(buffer, 4, 1, file);
    fclose(file);
}

void setBrightness(void** cfg, int target) {
    int minbr, maxbr;
    read_config(cfg,MIN_B,&minbr);
    read_config(cfg,MAX_B,&maxbr);
    int defsp;
    read_config(cfg,DEF_S,&defsp);

    if (target < minbr) target = minbr;
    if (target > maxbr) target = maxbr;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightness(cfg, current);
        usleep(20000/defsp);
    }
}

void setBrightness_percent(void** cfg, float percent) {
    int minbr, maxbr;
    read_config(cfg,MIN_B,&minbr);
    read_config(cfg,MAX_B,&maxbr);
    int defsp;
    read_config(cfg,DEF_S,&defsp);

    int target = (int)((float)maxbr * percent);
    if (target < minbr) target = minbr;
    if (target > maxbr) target = maxbr;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightness(cfg, current);
        usleep(20000/defsp);
    }
}

void setBrightnessUnbound(void** cfg, int target) {
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);
    int defsp;
    read_config(cfg,DEF_S,&defsp);

    if (target < 0) target = 0;
    if (target > maxbr) target = maxbr;
    int current = getBrightness(cfg);
    int inc = current > target ? -1 : 1;
    int steps = (target-current)*inc;
    while (steps --> 0) {
        current += inc;
        writeBrightnessUnbound(cfg, current);
        usleep(20000/defsp);
    }
}

void setBrightnessKeyboard(void** cfg, char* val) {
    char name[512];
    read_config(cfg,K_DEV,name);

    FILE* file = fopen(name, "w");
    if (file == NULL) {
        fprintf(stderr, "[main] could not write to '%s'\n.",name);
        exit(1);
    }
    fprintf(file, val);
    fclose(file);
}

void pulse(void** cfg, int amount) {
    int high;
    read_config(cfg,MAX_B,&high);

    int low = 1;
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

int getBrightness(void** cfg) {
    char name[512];
    read_config(cfg,B_DEV,name);

    FILE* file = fopen(name, "r");
    if (file == NULL) {
        fprintf(stderr, "[main] could not read '%s'\n.",name);
        exit(1);
    }
    char buf[16];
    fread(buf, 8, 1, file);
    fclose(file);
    return atoi(buf);
}

void autoBrightness(void** cfg, int amount) {
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);
    bool usekbd;
    read_config(cfg,U_KBD,&usekbd);
    float kbdval;
    read_config(cfg,KBD_VAL,&kbdval);
    int kbdmaxbr;
    read_config(cfg,KBD_MAX,&kbdmaxbr);


    int temp_amount = getLightLevel(cfg)*amount;
    int target = choose_function_and_params(cfg, temp_amount);

    if (target < 0) {
        target = 0;
    } else if (target > maxbr) {
        target = maxbr;
    }

    setBrightness(cfg, target);
    if (usekbd) {
        if (target <= kbdval*maxbr) {
            float tmp = target;
            tmp -= kbdval*maxbr;
            tmp /= (float)(kbdval*maxbr);
            int kbdbr = (int)((1.0+tmp)*kbdmaxbr+1.0);
            if (kbdbr > kbdmaxbr) {
                kbdbr = kbdmaxbr;
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
    printf("Usage: %s <option> [amount]\n"
           "\n"
           "options with [amount]:    set, inc, dec, pulse, auto\n"
           "options without [amount]: toggle, info\n",
         argv[0]);
}

void toggleBrightness(void** cfg) {
    bool usekbd;
    read_config(cfg,U_KBD,&usekbd);
    int maxbr;
    read_config(cfg,MAX_B,&maxbr);

    int curr = getBrightness(cfg);
    if (curr != 0) {
        setBrightnessUnbound(cfg, 0);
        if (usekbd) {
            setBrightnessKeyboard(cfg, "0");
        }
    } else {
        setBrightnessUnbound(cfg, maxbr);
        if (usekbd) {
            setBrightnessKeyboard(cfg, "0");
        }
    }
}

int chka(void** cfg, int amount) {
    int maxbr, defam;
    read_config(cfg,MAX_B,&maxbr);
    read_config(cfg,DEF_A,&defam);

    if (amount > maxbr) {
        fprintf(stderr, "[main] Value '%i' too high. Must be within [0,%i].\n",
                amount, maxbr);
        fprintf(stderr, "[main] Using maximum '%i'.\n", maxbr);
        return maxbr;
    } else if (amount < 0) {
        fprintf(stderr, "[main] Invalid amount '%i'. Must be within [0,%i].\n",
                amount, maxbr);
        fprintf(stderr, "[main] Using default '%i'.\n", defam);
        return defam;
    } else {
        return amount;
    }
}

float chka_percent(float amount) {
    amount = amount > 1.0 ? 1.0 : amount;
    amount = amount < 0.0 ? 0.0 : amount;
    return amount;
}

int chkp(void** cfg, int amount) {
    int pulsemax, pulseam;
    read_config(cfg,MAX_PULSE,&pulsemax);
    read_config(cfg,PULSE_VALUE,&pulseam);

    if (amount < 0 || amount > pulsemax) {
        fprintf(stderr, "[main] Invalid amount '%i'. Must be within [0,%i].\n",
                amount, pulsemax);
        fprintf(stderr, "[main] Using default '%i'.\n", pulseam);
        return pulseam;
    } else {
        return amount;
    }
}

bool use_percent(int argc, char** argv) {
    if (argc<=2) { return false; }
    int len = strlen(argv[2]);
    if (argv[2][len-1] == '%') {
        argv[2][len-1] = '\0';
        return true;
    } else {
        return false;
    }
}

#define percch( fun, cnf, amount, usep ) \
    if (usep) { \
        fun##_percent( cnf, chka_percent( (float) amount / 100.0 ) ) ; \
    } else { \
        fun ( cnf, chka( cnf, amount ) ) ; \
    }


void print_info( void** cnf ) {
    int level = getLightLevel(cnf);
    fprintf(stderr,"[info] current webcam light level, brightness: %i, %i\n",
            level, getBrightness(cnf));
    int maxbr, minbr;
    read_config(cnf, MAX_B, &maxbr); read_config(cnf, MIN_B, &minbr);
    fprintf(stderr,"[info] min, max brightness: %i, %i\n",minbr,maxbr);
}

int main(int argc, char **argv) {
    void** cnf = malloc(sizeof(void*)*get_keynum());
    init_config(cnf);
    read_configuration_file(cnf, CONFIG_FILE);
    default_config(cnf);
    if (argc < 2) {
        help( argv );
        return 0;
    }

    int set_amount, auto_amount, pulse_amount;
    read_config(cnf,DEF_A,&set_amount);
    read_config(cnf,AUTO_VALUE,&auto_amount);
    read_config(cnf,PULSE_VALUE,&pulse_amount);
    bool percent = use_percent(argc,argv);
    if (argc > 2) {
        set_amount = atoi(argv[2]);
        auto_amount = atoi(argv[2]);
        pulse_amount = atoi(argv[2]);
    }

    if (!strcmp(argv[1], "inc")) {
        percch( inc, cnf, set_amount, percent )
    } else if (!strcmp(argv[1], "dec")) {
        percch( dec, cnf, set_amount, percent )
    } else if (!strcmp(argv[1], "pulse")) {
        pulse(cnf, chkp(cnf, pulse_amount));
    } else if (!strcmp(argv[1], "auto")) {
        autoBrightness(cnf, auto_amount);
    } else if (!strcmp(argv[1], "set") && argc > 2) {
        percch( setBrightness, cnf, set_amount, percent )
    } else if (!strcmp(argv[1], "toggle")) {
        toggleBrightness(cnf);
    } else if (!strcmp(argv[1], "info")) {
        print_info(cnf);
    } else if (!strcmp(argv[1], "dbg")) {
        dbg_cnf(cnf);
    } else {
        help( argv );
    }
    delete_config(cnf);
    free(cnf);
}
