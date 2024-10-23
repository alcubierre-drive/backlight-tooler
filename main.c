#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <float.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "inih/ini.h"

typedef struct brightness_t brightness_t;
typedef struct {
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
} funcparam_t;
typedef float (*bfun_t)( float web, float min, funcparam_t p );
typedef struct {
    bfun_t fun;
    funcparam_t p;
} funcsel_t;
typedef struct config_t config_t;
struct config_t {
    char webcam_path[512];
    uint64_t webcam_br_min, webcam_br_max;
    uint64_t webcam_nx, webcam_ny;

    char screen_path[512];
    float screen_br_min;

    char kbd_path[512];
    float kbd_br_max;
    float kbd_val_min;

    char function_name[32];
    char function_params[512];

    float value_percent;

    char config_file[512];
    int config_file_touched;
};
static const config_t config_defaults = {
    .webcam_path = "/dev/video0",
    .webcam_br_min = 72,
    .webcam_br_max = 250,
    .webcam_nx = 1080,
    .webcam_ny = 720,

    .screen_path = "/sys/class/backlight/intel_backlight/",
    .screen_br_min = 0.05,

    .kbd_path = "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::kbd_backlight/",
    .kbd_br_max = 0.2,
    .kbd_val_min = 0.4,

    .function_name = "exp",
    .function_params = "0.0,3.0",

    // auto mode, every other value is used for non-auto (inc/dec) mode
    .value_percent = FLT_MAX,

    .config_file = "/etc/backlight-tooler.conf",
    .config_file_touched = 0,
};
static int verbose = 0;

static float webcam_read( const char* name, float min, float max, unsigned width, unsigned height );
static brightness_t* brightness_init( const char* path );
static void brightness_set( brightness_t* b, float val );
static float brightness_get( brightness_t* b );
static void brightness_free( brightness_t* b );
static funcsel_t selectfun( const char* name, const char* params );
static config_t parse_args( int* pargc, char*** pargv );

int main(int argc, char** argv) {

    config_t cfg = parse_args(&argc, &argv);

    brightness_t* b = brightness_init( cfg.screen_path );
    if (!b) return EXIT_FAILURE;

    int auto_mode = (cfg.value_percent == FLT_MAX);
    if (auto_mode) {
        float wval = webcam_read( cfg.webcam_path, cfg.webcam_br_min, cfg.webcam_br_max, cfg.webcam_nx, cfg.webcam_ny );
        if (wval < 0.0) wval = 0.0;
        if (wval > 1.0) wval = 1.0;
        if (verbose) fprintf( stderr, "read brightness: %3.0f%%\n", wval*100.0 );
        funcsel_t f = selectfun( cfg.function_name, cfg.function_params );
        cfg.value_percent = (*f.fun)( wval, cfg.screen_br_min, f.p );
    } else {
        float xval = brightness_get(b) + cfg.value_percent/100.0;
        if (xval < cfg.screen_br_min) xval = cfg.screen_br_min;
        if (xval > 1.0) xval = 1.0;
        cfg.value_percent = xval;
    }


    if (verbose) fprintf( stderr, "anticipated brightness %3.0f%%\n", cfg.value_percent*100.0 );
    brightness_set(b, cfg.value_percent);
    brightness_free(b);

    brightness_t* k = brightness_init( cfg.kbd_path );
    if (!k) return EXIT_FAILURE;

    float kval = cfg.value_percent;
    if (kval >= cfg.kbd_br_max) {
        kval = 0.0;
    } else {
        kval = kval/cfg.kbd_br_max;
        if (kval > 1.0) kval = 1.0;
        if (kval <= cfg.kbd_val_min) kval = cfg.kbd_val_min;
    }
    if (verbose)
        fprintf( stderr, "kbd value after bounds (%.2f,%.2f): %.2f\n", cfg.kbd_br_max, cfg.kbd_val_min, kval );
    brightness_set(k, kval);
    brightness_free(k);
}

static void strcpy_last( char* out, const char* in ) {
    char* x = alloca(strlen(in)+1);
    strcpy(x, in);
    char* last_slash = strrchr(x, '/');
    *last_slash = 0;
    strcpy(out, x);
}

static int config_parser( void* c_, const char* section, const char* name, const char* value ) {
    config_t* c = c_;
    #define MATCH( s, n, ... ) \
    if ((strcmp(section, (s)) == 0) && (strcmp(name, (n)) == 0)) { \
        nparse++; \
        if (verbose) fprintf( stderr, "[%s] %s=%s\n", (s), (n), value ); \
        __VA_ARGS__; \
    }
    int nparse = 0;
    MATCH("webcam", "path", strcpy(c->webcam_path, value));
    MATCH("webcam", "br_min", c->webcam_br_min = atoi(value));
    MATCH("webcam", "br_max", c->webcam_br_max = atoi(value));
    MATCH("webcam", "nx", c->webcam_nx = atoi(value));
    MATCH("webcam", "ny", c->webcam_ny = atoi(value));
    MATCH("screen", "path", strcpy(c->screen_path, value));
    MATCH("screen", "br_min", c->screen_br_min = atof(value));
    MATCH("kbd", "path", strcpy(c->kbd_path, value));
    MATCH("kbd", "br_max", c->kbd_br_max = atof(value));
    MATCH("kbd", "val_min", c->kbd_val_min = atof(value));
    MATCH("function", "name", strcpy(c->function_name, value));
    MATCH("function", "params", strcpy(c->function_params, value));

    #define AMATCH( n, ... ) \
    if (strcmp(name, (n)) == 0) { \
        nparse++; \
        if (verbose) fprintf( stderr, "DEPRECATED CONFIG KEY: '%s'\n", (n) ); \
        __VA_ARGS__; \
    }
    AMATCH( "backlightDevice", strcpy_last(c->screen_path, value) );
    AMATCH( "maxBrightness" );
    AMATCH( "minBrightness", c->screen_br_min = atof(value)/400. );
    AMATCH( "readMinMaxBrightness" );
    AMATCH( "minAfterRead" );
    AMATCH( "keyboardDevice", strcpy_last(c->kbd_path, value) );
    AMATCH( "useKeyboard" );
    AMATCH( "keyboardMaxBrightness", c->kbd_br_max = atof(value) );
    AMATCH( "keyboardMinBrightness", c->kbd_val_min = atof(value) );
    AMATCH( "webcamDevice", strcpy(c->webcam_path, value) );
    AMATCH( "webcamWidth", c->webcam_nx = atol(value) );
    AMATCH( "webcamHeight", c->webcam_ny = atol(value) );
    AMATCH( "webcamLightValLow", c->webcam_br_min = atol(value) );
    AMATCH( "webcamLightValHigh", c->webcam_br_max = atol(value) );
    AMATCH( "functionChoice", strcpy( c->function_name, value ) );
    AMATCH( "functionParams", strcpy( c->function_params, value ) );

    return nparse;
}

static int atofN( const char* str_, float* ary, int ary_max );
static int atou64N( const char* str_, uint64_t* ary, int ary_max );

static void parse_config_file( config_t* cfg ) {
    int ii = ini_parse(cfg->config_file, config_parser, cfg);
    if (ii < 0 && cfg->config_file_touched) fprintf(stderr, "can't load '%s'\n", cfg->config_file);
    if (ii > 0 && verbose) fprintf( stderr, "config file parsing error in l.%i\n", ii );
}

static config_t parse_args( int* pargc, char*** pargv ) {
    config_t cfg = config_defaults;
    parse_config_file(&cfg);

    char pad[512] = "";
    sprintf( pad, "%*s", (int)strlen(*pargv[0]), " " );
    int opt;

    // we do option parsing twice in order to first find a config file, and then
    // override options found in the config file
    while ((opt = getopt(*pargc, *pargv, ":c:v:V W:w:d: S:s: K:k:m: F:f: h")) != -1) {
        switch (opt) {
            case 'c': strcpy(cfg.config_file, optarg); cfg.config_file_touched=1; break;
            case 'v': if (strcmp(optarg, "auto") != 0) cfg.value_percent = atof(optarg); break;
            case 'V': verbose = 1; break;
            default: break;
        }
    }
    parse_config_file(&cfg);
    optind = 1;
    while ((opt = getopt(*pargc, *pargv, "c:v:V W:w:d: S:s: K:k:m: F:f: h")) != -1) {
        switch (opt) {
            // handled above
            case 'c':
            case 'v':
            case 'V': break;

            case 'W': strcpy(cfg.webcam_path, optarg); break;
            case 'w': atou64N(optarg, &cfg.webcam_br_min, 2); break;
            case 'd': atou64N(optarg, &cfg.webcam_nx, 2); break;

            case 'S': strcpy(cfg.screen_path, optarg); break;
            case 's': cfg.screen_br_min = atof(optarg); break;

            case 'K': strcpy(cfg.kbd_path, optarg); break;
            case 'k': cfg.kbd_br_max = atof(optarg); break;
            case 'm': cfg.kbd_val_min = atof(optarg); break;

            case 'F': strcpy(cfg.function_name, optarg); break;
            case 'f': strcpy(cfg.function_params, optarg); break;

            case 'h': fprintf( stderr,
"Usage: %s [-c /path/to/config/file] [-v percent|auto] [-V:verbose]\n"
"       %s [-W /path/to/video] [-w webcam absoulte limits (min,max)]\n"
"       %s [-d webcam dim (x,y)] [-S /path/to/brightness/]\n"
"       %s [-s screen min brightness (rel value)] [-K /path/to/keyboard/]\n"
"       %s [-k kbd upper bdy (rel val)] [-m kbd lower bdy replace (rel val)]\n"
"       %s [-F function name] [-f function params (a,b,c,…)] [-h:this help]\n"
                                   , *pargv[0], pad, pad, pad, pad, pad );
                      exit(EXIT_SUCCESS); break;
            case ':':
            case '?':
            default:
                      fprintf( stderr, "%s -h for help\n", *pargv[0] );
                      exit(EXIT_FAILURE); break;
        }
    }
    return cfg;
}


static float webcam_read( const char* name, float min, float max, unsigned width, unsigned height ) {
    int fd = open(name, O_RDWR);
    if (fd == -1) {
        fprintf( stderr, "cannot open file '%s'\n", name );
        return 0.5; // safe value in case we cannot read webcam
    }
    struct v4l2_capability cap;
    struct v4l2_format format;
    ioctl(fd, VIDIOC_QUERYCAP, &cap);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;

    ioctl(fd, VIDIOC_S_FMT, &format);
    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 1;

    ioctl(fd, VIDIOC_REQBUFS, &bufrequest);
    struct v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo);

    void* buffer_start = mmap(
        NULL,
        bufferinfo.length,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        bufferinfo.m.offset
    );

    memset(buffer_start, 0, bufferinfo.length);
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    int type = bufferinfo.type;
    ioctl(fd, VIDIOC_STREAMON, &type);
    ioctl(fd, VIDIOC_QBUF, &bufferinfo);
    ioctl(fd, VIDIOC_DQBUF, &bufferinfo);
    ioctl(fd, VIDIOC_STREAMOFF, &type);

    long total = 0;
    unsigned i = 0;
    for (i = 0; i<bufferinfo.length; ++i)
        total += ((unsigned char*)buffer_start)[i];
    munmap(buffer_start, bufferinfo.length);
    close(fd);
    float val = (float)total/(float)i;
    if (verbose)
        fprintf( stderr, "read value %.2f from webcam, bounds (%.2f,%.2f)\n", val, min, max );
    float res = (val - min)/(max - min);
    if (res < 0.0) return 0.0;
    if (res > 1.0) return 1.0;
    if (verbose)
        fprintf( stderr, "webcam effective value is %.2f\n", res );
    return res;
}

struct brightness_t {
    FILE* br;
    float max_br;
};

static void brightness_free( brightness_t* b ) {
    if (b) {
        if (b->br) fclose(b->br);
        free(b);
    }
}

#define error(...) { \
    fprintf( stderr, __VA_ARGS__ ); \
    goto errors; \
}

static brightness_t* brightness_init( const char* path_ ) {
    brightness_t* b = calloc(1, sizeof*b);
    FILE* f = NULL;
    if (!b) error("calloc\n");

    char path[512];
    path[0] = 0;
    strcat( path, path_ );
    strcat( path, "/brightness" );
    b->br = fopen( path, "r+" );

    if (!b->br) error("cannot open file %s\n", path);

    path[0] = 0;
    strcat( path, path_ );
    strcat( path, "/max_brightness" );
    f = fopen( path, "r" );

    if (!f) error("cannot open file %s\n", path);

    unsigned long max_br = -1;
    int nread = fscanf( f, "%lu", &max_br );
    if (nread != 1 || max_br == (unsigned long)-1) error("could not read max_brightness\n");

    fclose(f);
    b->max_br = max_br;
    return b;

errors:
    brightness_free(b);
    if (f) fclose(f);
    return NULL;
}

static float brightness_get( brightness_t* b ) {
    rewind(b->br);
    unsigned long br = -1;
    fscanf(b->br, "%lu", &br);
    return (float)br / b->max_br;
}

static void brightness_set( brightness_t* b, float val ) {
    unsigned long ival = lroundl( val * b->max_br );
    rewind(b->br);
    ftruncate(fileno(b->br), 0);
    fprintf(b->br, "%lu\n", ival);
    fflush(b->br);
}

static float minscale( float val, float min ) {
    return val * (1.-min) + min;
}

static float bfun_linear( float web, float min, funcparam_t p ) {
    (void)p;
    return minscale(web, min);
}

static float bfun_power( float web, float min, funcparam_t p ) {
    return minscale(pow(web,p.a), min);
}

#define return_xinterpolate( fun ) \
    float fa = fun(p.a), fb = fun(p.b), \
          fw = fun(p.a + (p.b-p.a)*web); \
    return minscale((fw-fa)/(fb-fa), min);

static float bfun_log( float web, float min, funcparam_t p ) {
    return_xinterpolate(log);
}

static float bfun_exp( float web, float min, funcparam_t p ) {
    return_xinterpolate(exp);
}

static float bfun_gamma( float web, float min, funcparam_t p ) {
    return_xinterpolate(tgammaf);
}

static float bfun_sin( float web, float min, funcparam_t p ) {
    (void)p;
    return minscale(sin(web*M_PI*0.5), min);
}

static float bfun_minkowski( float web, float min, funcparam_t p_ ) {
    (void)p_;
    float arg = web;
    long p = arg;

    float result = 0;
    if ((float)p > arg) --p;
    long q = 1, r = p + 1, s = 1, m, n;
    float d = 1, y = p;
    if (arg < (float)p || (p < 0) ^ (r <= 0)) {
        result = arg; 
        goto ret_result;
    }
    for (;;) {
        d /= 2;
        if (y + d == y)
            break;
        m = p + r;
        if ((m < 0) ^ (p < 0))
            break;
        n = q + s;
        if (n < 0)
            break;

        if (arg < (float)m / n) {
            r = m;
            s = n;
        } else {
            y += d;
            p = m;
            q = n;
        }
    }
    result = y + d;
ret_result:
    return minscale(result, min);
}

// oh yeah, that's a very nice way to parse arrays :)
static const int ATOXN_max = 1024;
#define ATOXN( name, type ) \
static int name##N( const char* str_, type* ary, int ary_max ) { \
    char* str = alloca(strlen(str_)+1); \
    strcpy(str, str_); \
    char* delim; \
    int len = 0; \
    while ((delim = strchrnul(str, ','))) { \
        int brk = !*delim; \
        *delim = 0; \
        if (str != delim) \
            ary[len++] = name(str); \
        if (len >= ATOXN_max || len >= ary_max || brk) \
            break; \
        str = delim+1; \
    } \
    return len; \
}
ATOXN( atof, float )
static inline uint64_t atou64( const char* str ) { return labs(atol(str)); }
ATOXN( atou64, uint64_t )

static funcsel_t selectfun( const char* name, const char* params ) {
    funcsel_t f = {0};
    if (verbose)
        fprintf( stderr, "trying to find function with name '%s' and parameters '%s'\n", name, params );
    if (!strcmp(name, "power")) {
        f.fun = &bfun_power;
        f.p.a = atof(params);
    } else if (!strcmp(name, "log")) {
        f.fun = &bfun_log;
        atofN(params, &f.p.a, 2);
    } else if (!strcmp(name, "exp")) {
        f.fun = &bfun_exp;
        atofN(params, &f.p.a, 2);
    } else if (!strcmp(name, "gamma")) {
        f.fun = &bfun_gamma;
        atofN(params, &f.p.a, 2);
    } else if (!strcmp(name, "sin")) {
        f.fun = &bfun_sin;
    } else if (!strcmp(name, "?")) {
        f.fun = &bfun_minkowski;
    } else {
        if (!strcmp(name, "LIST"))
            fprintf( stderr, "available functions:\n"
                     "  power:  x->x^a\n"
                     "  log:    x->normalize(log( (x-a)/(b-a) ))\n"
                     "  exp:    x->normalize(exp( (x-a)/(b-a) ))\n"
                     "  gamma:  x->normalize(tgammaf( (x-a)/(b-a) ))\n"
                     "  sin:    x->sin(M_PI/2 * x)\n"
                     "  linear: x->x\n" );
        f.fun = &bfun_linear;
    }
    return f;
}

