#define _XOPEN_SOURCE 700

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define USE_LOOKUP_TABLES
#include "readconfig.h"

int keyfromstring(char* key, int* type) {
    for (unsigned i=0; i < NKEYS; ++i) {
        if (strcmp(lookuptable[i].key, key) == 0) {
            *type = lookuptable[i].type;
            return lookuptable[i].val;
        }
    }
    return BADKEY;
}

char* remove_string(char* str, const char* sub) {
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

void cf_char_set( cf_char* x, char* y ) {
    strcpy(x->val,y);
    x->has = true;
}
void cf_int_set( cf_int* x, char* y ) {
    x->val = atoi(y);
    x->has = true;
}
void cf_float_set( cf_float* x, char* y ) {
    x->val = atof(y);
    x->has = true;
}
void cf_bool_set( cf_bool* x, char* y ) {
    if (strcmp(y,"true") == 0) {
        x->val = true;
    } else {
        x->val = false;
    }
    x->has = true;
}

void get_name(int which, char* out) {
    for (unsigned i=0; i<NKEYS; ++i) {
        if (lookuptable[i].val == which) {
            strcpy(out,lookuptable[i].key);
            break;
        }
    }
}


void cf_set( void** cfg, int which, char* val, int type ) {
    if (val==NULL) {
        char name[512];
        get_name(which,name);
        fprintf(stderr,"[config] Empty config field '%s'\n",name);
        return;
    }
    switch(type) {
        case CHAR_TYPE:
            cf_char_set( ((cf_char*)cfg[which]), val );
            break;
        case INT_TYPE:
            cf_int_set( ((cf_int*)cfg[which]), val );
            break;
        case FLOAT_TYPE:
            cf_float_set( ((cf_float*)cfg[which]), val );
            break;
        case BOOL_TYPE:
            cf_bool_set( ((cf_bool*)cfg[which]), val );
            break;
    }
}

void cf_set_default( void** cfg ) {
    for (unsigned i=0; i<NKEYS; ++i) {
        int idx = lookuptable[i].val;
        defaults def = lookuptable[i].def;
        int type = lookuptable[i].type;
        switch(type) {
            case CHAR_TYPE:
                if ( ((cf_char*)cfg[idx])->has ) {
                    break;
                }
                strcpy(((cf_char*)cfg[idx])->val,def.c);
                break;
            case INT_TYPE:
                if ( ((cf_int*)cfg[idx])->has ) {
                    break;
                }
                ((cf_int*)cfg[idx])->val = def.i;
                break;
            case BOOL_TYPE:
                if ( ((cf_bool*)cfg[idx])->has ) {
                    break;
                }
                ((cf_bool*)cfg[idx])->val = def.b;
                break;
            case FLOAT_TYPE:
                if ( ((cf_float*)cfg[idx])->has ) {
                    break;
                }
                ((cf_float*)cfg[idx])->val = def.f;
                break;
        }
    }
}


void modify_config( void** cfg, char* line ) {
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
    int key_num = keyfromstring(key, &type);
    if (key_num != BADKEY) {
        cf_set( cfg, key_num, val, type );
    } else {
        fprintf(stderr, "[config] unknown key '%s'.\n", key);
    }
}

void read_configuration_file( void** cfg, char* path ) {
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "[config] could not read '%s'.\n", path);
        return;
    }
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&line, &len, fp)) != -1) {
        modify_config( cfg, line );
    }
    fclose(fp);
}

void read_max_brightness( void** cfg, bool kbd ) {
    char maxbrightnessfile[512];
    if (kbd) {
        strcpy(maxbrightnessfile,((cf_char*)cfg[K_DEV])->val);
        sprintf(maxbrightnessfile, "%smax_brightness",
                remove_string(maxbrightnessfile,"brightness"));
    } else {
        strcpy(maxbrightnessfile,((cf_char*)cfg[B_DEV])->val);
        sprintf(maxbrightnessfile, "%smax_brightness",
                remove_string(maxbrightnessfile,"brightness"));
    }
    FILE* mbf = fopen(maxbrightnessfile, "r");
    if (mbf == NULL) {
        if (kbd) {
            ((cf_int*)cfg[KBD_MAX])->val = 2;
        } else {
            ((cf_int*)cfg[MAX_B])->val = 1060;
        }
        fprintf(stderr,"[config] Impossible to read '%s'.\n",maxbrightnessfile);
        return;
    }
    char maxbrval[16];
    fread(maxbrval, 8, 1, mbf);
    fclose(mbf);
    if (kbd) {
        ((cf_int*)cfg[KBD_MAX])->val = atoi(maxbrval);
    } else {
        ((cf_int*)cfg[MAX_B])->val = atoi(maxbrval);
    }
}

void default_config( void** cfg ) {
    cf_set_default( cfg );
    if ( ! ((cf_int*)cfg[MAX_B])->has ) {
        read_max_brightness( cfg, false );
    }
    if ( ! ((cf_int*)cfg[MIN_B])->has ) {
        ((cf_int*)cfg[MIN_B])->val = 0.02*((cf_int*)cfg[MAX_B])->val;
    }
    if ( ! ((cf_int*)cfg[DEF_A])->has ) {
        ((cf_int*)cfg[DEF_A])->val = 0.07*((cf_int*)cfg[DEF_A])->val;
    }
    if ( ! ((cf_int*)cfg[KBD_MAX])->has ) {
        if ( ((cf_bool*)cfg[U_KBD])->val ) {
            read_max_brightness( cfg, true );
        }
    }
}

int get_type(int which) {
    int type = -1;
    for (unsigned i=0; i<NKEYS; ++i) {
        if (lookuptable[i].val == which) {
            type = lookuptable[i].type;
        }
    }
    return type;
}

void dbg_cnf( void** cfg ) {
    for (unsigned i=0; i<NKEYS; ++i) {
        int type = get_type(i);
        char name[512];
        get_name(i,name);
        fprintf(stderr,"%s=",name);
        switch(type) {
            case BOOL_TYPE:
                fprintf(stderr,"%i\n",((cf_bool*)cfg[i])->val);
                break;
            case INT_TYPE:
                fprintf(stderr,"%i\n",((cf_int*)cfg[i])->val);
                break;
            case CHAR_TYPE:
                fprintf(stderr,"%s\n",((cf_char*)cfg[i])->val);
                break;
            case FLOAT_TYPE:
                fprintf(stderr,"%.3f\n",((cf_float*)cfg[i])->val);
                break;
        }
    }
}

void init_config(void** cfg) {
    for (unsigned i=0; i<NKEYS; ++i) {
        int idx = lookuptable[i].val;
        int type = lookuptable[i].type;
        int size = 0;
        switch(type) {
            case CHAR_TYPE:
                size = sizeof(cf_char);
                cfg[idx] = malloc(size);
                ((cf_char*)cfg[idx])->has = false;
                break;
            case INT_TYPE:
                size = sizeof(cf_int);
                cfg[idx] = malloc(size);
                ((cf_int*)cfg[idx])->has = false;
                break;
            case BOOL_TYPE:
                size = sizeof(cf_bool);
                cfg[idx] = malloc(size);
                ((cf_bool*)cfg[idx])->has = false;
                break;
            case FLOAT_TYPE:
                size = sizeof(cf_float);
                cfg[idx] = malloc(size);
                ((cf_float*)cfg[idx])->has = false;
                break;
        }
    }
}

void delete_config(void** cfg) {
    for (unsigned i=0; i<NKEYS; ++i) {
        free(cfg[i]);
    }
}

void read_config(void** cfg, int which, void* out ) {
    int type = get_type(which);
    switch(type) {
        case CHAR_TYPE:
            strcpy(out,((cf_char*)(cfg[which]))->val);
            break;
        case INT_TYPE:
            *((int*)out) = ((cf_int*)(cfg[which]))->val;
            break;
        case BOOL_TYPE:
            *((bool*)out) = ((cf_bool*)(cfg[which]))->val;
            break;
        case FLOAT_TYPE:
            *((float*)out) = ((cf_float*)(cfg[which]))->val;
            break;
    }
}

int get_keynum() {
    return NKEYS;
}
