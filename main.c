#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <float.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "inih/ini.h"

#define BUS_NAME  "org.BacklightTooler"
#define BUS_PATH  "/org/BacklightTooler"
#define BUS_IFACE "org.BacklightTooler"
#define USEC_PER_SEC 1000000ULL
// bounds for an Interval set over the bus (0 is always accepted)
#define INTERVAL_MIN_USEC (1ULL * USEC_PER_SEC)
#define INTERVAL_MAX_USEC (366ULL * 24 * 3600 * USEC_PER_SEC)

enum { ACTION_SET, ACTION_RELOAD, ACTION_STATUS, ACTION_TIMER };
enum { TIMER_ON, TIMER_OFF, TIMER_TOGGLE };

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

    // seconds between automatic updates in daemon mode, 0 disables them
    uint64_t daemon_interval;

    float value_percent;

    char config_file[512];
    int config_file_touched;
    // return value of ini_parse(): 0 ok, <0 unreadable, >0 first bad line
    int config_file_status;

    int daemon_mode;
    int bus_action;
    int timer_action; // with ACTION_TIMER
    // set by options that only make sense when we do the work ourselves
    int local_only;
};
typedef struct {
    double webcam; // last measured webcam value in [0,1], -1 if none yet
    double screen;
    double kbd;
} levels_t;
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

    .daemon_interval = 900,

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
static int run_once( const config_t* cfg, float value_percent, levels_t* lvl );
static int daemon_run( int argc, char** argv, const config_t* cfg );
static int client_run( const config_t* cfg );

int main(int argc, char** argv) {

    config_t cfg = parse_args(&argc, &argv);

    if (cfg.daemon_mode)
        return daemon_run(argc, argv, &cfg) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;

    // hand the request to a running daemon if there is one, otherwise do the
    // work ourselves
    if (!cfg.local_only || cfg.bus_action != ACTION_SET) {
        int r = client_run(&cfg);
        if (r > 0) return EXIT_SUCCESS;
        if (r < 0) return EXIT_FAILURE;
    }
    if (cfg.bus_action != ACTION_SET) {
        fprintf( stderr, "backlight-tooler daemon is not running\n" );
        return EXIT_FAILURE;
    }

    levels_t lvl = { -1, -1, -1 };
    return run_once(&cfg, cfg.value_percent, &lvl) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

// value_percent == FLT_MAX means auto mode, every other value is a relative
// change in percent
static int run_once( const config_t* cfg, float value_percent, levels_t* lvl ) {

    brightness_t* b = brightness_init( cfg->screen_path );
    if (!b) return -1;

    float val;
    if (value_percent == FLT_MAX) {
        float wval = webcam_read( cfg->webcam_path, cfg->webcam_br_min, cfg->webcam_br_max, cfg->webcam_nx, cfg->webcam_ny );
        if (wval < 0.0) wval = 0.0;
        if (wval > 1.0) wval = 1.0;
        if (verbose) fprintf( stderr, "read brightness: %3.0f%%\n", wval*100.0 );
        lvl->webcam = wval;
        funcsel_t f = selectfun( cfg->function_name, cfg->function_params );
        val = (*f.fun)( wval, cfg->screen_br_min, f.p );
    } else {
        val = brightness_get(b) + value_percent/100.0;
        if (val < cfg->screen_br_min) val = cfg->screen_br_min;
        if (val > 1.0) val = 1.0;
    }

    if (verbose) fprintf( stderr, "anticipated brightness %3.0f%%\n", val*100.0 );
    brightness_set(b, val);
    brightness_free(b);
    lvl->screen = val;

    brightness_t* k = brightness_init( cfg->kbd_path );
    if (!k) return -1;

    float kval = val;
    if (kval >= cfg->kbd_br_max) {
        kval = 0.0;
    } else {
        kval = kval/cfg->kbd_br_max;
        if (kval > 1.0) kval = 1.0;
        if (kval <= cfg->kbd_val_min) kval = cfg->kbd_val_min;
    }
    if (verbose)
        fprintf( stderr, "kbd value after bounds (%.2f,%.2f): %.2f\n", cfg->kbd_br_max, cfg->kbd_val_min, kval );
    brightness_set(k, kval);
    brightness_free(k);
    lvl->kbd = kval;
    return 0;
}

static uint64_t now_usec( clockid_t clock ) {
    struct timespec ts;
    clock_gettime(clock, &ts);
    return (uint64_t)ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / 1000;
}

typedef struct {
    // kept to re-run the option parsing on config reloads, such that command
    // line options keep overriding the config file
    int argc;
    char** argv;
    config_t cfg;

    levels_t lvl;
    uint64_t last_update; // CLOCK_REALTIME, usec
    // usec; starts out as the config's interval, can be changed over the bus.
    // a config reload only overrides it if the config's interval changed.
    uint64_t interval;

    sd_bus* bus;
    sd_event_source* timer;
} daemon_t;

static int daemon_update( daemon_t* d, float value_percent ) {
    int r = run_once(&d->cfg, value_percent, &d->lvl);
    if (r < 0) return r;
    d->last_update = now_usec(CLOCK_REALTIME);
    sd_bus_emit_properties_changed(d->bus, BUS_PATH, BUS_IFACE,
            "WebcamValue", "ScreenBrightness", "KbdBrightness", "LastUpdate", NULL);
    return 0;
}

static void daemon_schedule( daemon_t* d ) {
    if (d->interval == 0) {
        sd_event_source_set_enabled(d->timer, SD_EVENT_OFF);
        return;
    }
    sd_event_source_set_time_relative(d->timer, d->interval);
    sd_event_source_set_enabled(d->timer, SD_EVENT_ONESHOT);
}

static int daemon_reload( daemon_t* d, sd_bus_error* err ) {
    config_t cfg = parse_args(&d->argc, &d->argv);
    if (cfg.config_file_status < 0)
        return sd_bus_error_setf(err, SD_BUS_ERROR_FILE_NOT_FOUND,
                "cannot read '%s'", cfg.config_file);
    if (cfg.config_file_status > 0)
        return sd_bus_error_setf(err, SD_BUS_ERROR_INVALID_FILE_CONTENT,
                "'%s': parse error in l.%i", cfg.config_file, cfg.config_file_status);

    // a stopped/changed timer survives the reload unless the config's interval
    // itself changed
    int interval_changed = cfg.daemon_interval != d->cfg.daemon_interval;
    d->cfg = cfg;
    uint64_t interval = cfg.daemon_interval * USEC_PER_SEC;
    if (interval_changed && interval != d->interval) {
        d->interval = interval;
        daemon_schedule(d);
    }
    fprintf( stderr, "reloaded config from '%s'\n", cfg.config_file );
    sd_bus_emit_properties_changed(d->bus, BUS_PATH, BUS_IFACE,
            "Interval", "ConfigInterval", "ConfigFile", NULL);
    return 0;
}

static int on_timer( sd_event_source* s, uint64_t usec, void* userdata ) {
    (void)s; (void)usec;
    daemon_t* d = userdata;
    if (daemon_update(d, FLT_MAX) < 0)
        fprintf( stderr, "automatic update failed\n" );
    daemon_schedule(d);
    return 0;
}

static int on_sighup( sd_event_source* s, const struct signalfd_siginfo* si, void* userdata ) {
    (void)s; (void)si;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    if (daemon_reload(userdata, &err) < 0)
        fprintf( stderr, "config reload failed: %s\n", err.message );
    sd_bus_error_free(&err);
    return 0;
}

static int method_update( sd_bus_message* m, void* userdata, sd_bus_error* err ) {
    if (daemon_update(userdata, FLT_MAX) < 0)
        return sd_bus_error_set(err, SD_BUS_ERROR_FAILED, "could not set brightness");
    return sd_bus_reply_method_return(m, NULL);
}

static int method_adjust( sd_bus_message* m, void* userdata, sd_bus_error* err ) {
    double delta;
    int r = sd_bus_message_read(m, "d", &delta);
    if (r < 0) return r;
    if (!isfinite(delta) || fabs(delta) > 1.0)
        return sd_bus_error_set(err, SD_BUS_ERROR_INVALID_ARGS, "delta must be in [-1,1]");
    if (daemon_update(userdata, delta*100.0) < 0)
        return sd_bus_error_set(err, SD_BUS_ERROR_FAILED, "could not set brightness");
    return sd_bus_reply_method_return(m, NULL);
}

static int method_reload( sd_bus_message* m, void* userdata, sd_bus_error* err ) {
    int r = daemon_reload(userdata, err);
    if (r < 0) return r;
    return sd_bus_reply_method_return(m, NULL);
}

static int get_config_file( sd_bus* bus, const char* path, const char* iface, const char* prop,
                            sd_bus_message* reply, void* userdata, sd_bus_error* err ) {
    (void)bus; (void)path; (void)iface; (void)prop; (void)err;
    daemon_t* d = userdata;
    return sd_bus_message_append(reply, "s", d->cfg.config_file);
}

static int get_interval( sd_bus* bus, const char* path, const char* iface, const char* prop,
                         sd_bus_message* reply, void* userdata, sd_bus_error* err ) {
    (void)bus; (void)path; (void)iface; (void)prop; (void)err;
    daemon_t* d = userdata;
    return sd_bus_message_append(reply, "t", d->interval);
}

static int get_config_interval( sd_bus* bus, const char* path, const char* iface, const char* prop,
                                sd_bus_message* reply, void* userdata, sd_bus_error* err ) {
    (void)bus; (void)path; (void)iface; (void)prop; (void)err;
    daemon_t* d = userdata;
    return sd_bus_message_append(reply, "t", d->cfg.daemon_interval * USEC_PER_SEC);
}

// 0 stops the timer, every other value (re)starts it with that period from now
static int set_interval( sd_bus* bus, const char* path, const char* iface, const char* prop,
                         sd_bus_message* value, void* userdata, sd_bus_error* err ) {
    (void)path; (void)iface; (void)prop;
    daemon_t* d = userdata;
    uint64_t interval;
    int r = sd_bus_message_read(value, "t", &interval);
    if (r < 0) return r;
    // the config's value is always fine, whatever it is
    if (interval && interval != d->cfg.daemon_interval * USEC_PER_SEC &&
        (interval < INTERVAL_MIN_USEC || interval > INTERVAL_MAX_USEC))
        return sd_bus_error_setf(err, SD_BUS_ERROR_INVALID_ARGS,
                "interval must be 0 or in [%llu,%llu] usec",
                INTERVAL_MIN_USEC, INTERVAL_MAX_USEC);
    d->interval = interval;
    daemon_schedule(d);
    if (interval) fprintf( stderr, "automatic updates every %.0fs\n", (double)interval / USEC_PER_SEC );
    else fprintf( stderr, "automatic updates off\n" );
    sd_bus_emit_properties_changed(bus, BUS_PATH, BUS_IFACE, "Interval", NULL);
    return 0;
}

static const sd_bus_vtable daemon_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_PROPERTY("WebcamValue", "d", NULL, offsetof(daemon_t, lvl.webcam), SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("ScreenBrightness", "d", NULL, offsetof(daemon_t, lvl.screen), SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("KbdBrightness", "d", NULL, offsetof(daemon_t, lvl.kbd), SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("LastUpdate", "t", NULL, offsetof(daemon_t, last_update), SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_WRITABLE_PROPERTY("Interval", "t", get_interval, set_interval, 0,
                             SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE | SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_PROPERTY("ConfigInterval", "t", get_config_interval, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("ConfigFile", "s", get_config_file, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_METHOD("Update", "", "", method_update, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Adjust", "d", "", method_adjust, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("ReloadConfig", "", "", method_reload, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

static int daemon_run( int argc, char** argv, const config_t* cfg ) {
    daemon_t d = {
        .argc = argc,
        .argv = argv,
        .cfg = *cfg,
        .lvl = { -1, -1, -1 },
        .interval = cfg->daemon_interval * USEC_PER_SEC,
    };
    sd_event* event = NULL;
    const char* what;
    int r;

    what = "sd_event_default";
    if ((r = sd_event_default(&event)) < 0) goto fail;
    what = "sd_event_set_signal_exit";
    if ((r = sd_event_set_signal_exit(event, 1)) < 0) goto fail;
    what = "sd_event_add_signal";
    if ((r = sd_event_add_signal(event, NULL, SIGHUP | SD_EVENT_SIGNAL_PROCMASK, on_sighup, &d)) < 0) goto fail;
    // fires right away for the initial update; BOOTTIME so that we catch up
    // after a suspend
    what = "sd_event_add_time_relative";
    if ((r = sd_event_add_time_relative(event, &d.timer, CLOCK_BOOTTIME, 0, 0, on_timer, &d)) < 0) goto fail;

    what = "sd_bus_open_user";
    if ((r = sd_bus_open_user(&d.bus)) < 0) goto fail;
    what = "sd_bus_add_object_vtable";
    if ((r = sd_bus_add_object_vtable(d.bus, NULL, BUS_PATH, BUS_IFACE, daemon_vtable, &d)) < 0) goto fail;
    what = "sd_bus_request_name";
    if ((r = sd_bus_request_name(d.bus, BUS_NAME, 0)) < 0) goto fail;
    what = "sd_bus_attach_event";
    if ((r = sd_bus_attach_event(d.bus, event, SD_EVENT_PRIORITY_NORMAL)) < 0) goto fail;

    what = "sd_event_loop";
    r = sd_event_loop(event);

fail:
    if (r < 0) fprintf( stderr, "%s: %s\n", what, strerror(-r) );
    sd_event_source_unref(d.timer);
    sd_bus_flush_close_unref(d.bus);
    sd_event_unref(event);
    return r;
}

static int client_status( sd_bus* bus, sd_bus_error* err ) {
    static const char* names[] = { "WebcamValue", "ScreenBrightness", "KbdBrightness" };
    double val[3];
    uint64_t last, interval;
    char* config_file = NULL;
    int r;
    for (int i=0; i<3; ++i)
        if ((r = sd_bus_get_property_trivial(bus, BUS_NAME, BUS_PATH, BUS_IFACE, names[i], err, 'd', &val[i])) < 0)
            return r;
    if ((r = sd_bus_get_property_trivial(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "LastUpdate", err, 't', &last)) < 0)
        return r;
    if ((r = sd_bus_get_property_trivial(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "Interval", err, 't', &interval)) < 0)
        return r;
    if ((r = sd_bus_get_property_string(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "ConfigFile", err, &config_file)) < 0)
        return r;

    char tbuf[64] = "never";
    if (last) {
        time_t t = last / USEC_PER_SEC;
        strftime(tbuf, sizeof tbuf, "%F %T", localtime(&t));
    }
    if (val[0] < 0) printf( "webcam:      n/a\n" );
    else printf( "webcam:      %3.0f%%\n", val[0]*100.0 );
    printf( "screen:      %3.0f%%\n", val[1]*100.0 );
    printf( "keyboard:    %3.0f%%\n", val[2]*100.0 );
    printf( "last update: %s\n", tbuf );
    if (interval) printf( "interval:    %lus\n", (unsigned long)(interval / USEC_PER_SEC) );
    else printf( "interval:    off\n" );
    printf( "config file: %s\n", config_file );
    free(config_file);
    return 0;
}

static int client_timer( sd_bus* bus, int action, sd_bus_error* err ) {
    uint64_t interval = 0, config_interval;
    int r;
    if ((r = sd_bus_get_property_trivial(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "ConfigInterval", err, 't', &config_interval)) < 0)
        return r;
    if (action == TIMER_TOGGLE) {
        if ((r = sd_bus_get_property_trivial(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "Interval", err, 't', &interval)) < 0)
            return r;
        action = interval ? TIMER_OFF : TIMER_ON;
    }
    if (action == TIMER_ON) {
        if (!config_interval)
            return sd_bus_error_set(err, SD_BUS_ERROR_NOT_SUPPORTED,
                    "no interval configured ([daemon] interval=0)");
        interval = config_interval;
    } else {
        interval = 0;
    }
    return sd_bus_set_property(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "Interval", err, "t", interval);
}

// 1: the daemon handled the request, 0: no daemon reachable, <0: error
static int client_run( const config_t* cfg ) {
    sd_bus* bus = NULL;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    int r = sd_bus_open_user(&bus);
    if (r < 0) {
        if (verbose) fprintf( stderr, "cannot connect to user bus: %s\n", strerror(-r) );
        return 0;
    }

    if (cfg->bus_action == ACTION_STATUS)
        r = client_status(bus, &err);
    else if (cfg->bus_action == ACTION_RELOAD)
        r = sd_bus_call_method(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "ReloadConfig", &err, NULL, "");
    else if (cfg->bus_action == ACTION_TIMER)
        r = client_timer(bus, cfg->timer_action, &err);
    else if (cfg->value_percent == FLT_MAX)
        r = sd_bus_call_method(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "Update", &err, NULL, "");
    else
        r = sd_bus_call_method(bus, BUS_NAME, BUS_PATH, BUS_IFACE, "Adjust", &err, NULL, "d",
                               (double)cfg->value_percent/100.0);

    if (r >= 0) {
        if (verbose) fprintf( stderr, "request handled by daemon\n" );
        r = 1;
    } else if (sd_bus_error_has_names(&err, SD_BUS_ERROR_SERVICE_UNKNOWN, SD_BUS_ERROR_NAME_HAS_NO_OWNER)) {
        if (verbose) fprintf( stderr, "no daemon on the user bus\n" );
        r = 0;
    } else {
        fprintf( stderr, "daemon: %s\n", err.message ? err.message : strerror(-r) );
    }
    sd_bus_error_free(&err);
    sd_bus_flush_close_unref(bus);
    return r;
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
    MATCH("daemon", "interval", c->daemon_interval = labs(atol(value)));

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
    cfg->config_file_status = ii;
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
    optind = 1;
    while ((opt = getopt(*pargc, *pargv, ":c:v:V W:w:d: S:s: K:k:m: F:f: DRga: h")) != -1) {
        switch (opt) {
            case 'c': strcpy(cfg.config_file, optarg); cfg.config_file_touched=1; cfg.local_only=1; break;
            case 'v': if (strcmp(optarg, "auto") != 0) cfg.value_percent = atof(optarg); break;
            case 'V': verbose = 1; break;
            default: break;
        }
    }
    parse_config_file(&cfg);
    optind = 1;
    while ((opt = getopt(*pargc, *pargv, "c:v:V W:w:d: S:s: K:k:m: F:f: DRga: h")) != -1) {
        switch (opt) {
            // handled above
            case 'c':
            case 'v':
            case 'V': break;

            case 'D': cfg.daemon_mode = 1; break;
            case 'R': cfg.bus_action = ACTION_RELOAD; break;
            case 'g': cfg.bus_action = ACTION_STATUS; break;
            case 'a':
                      cfg.bus_action = ACTION_TIMER;
                      if (!strcmp(optarg, "on")) cfg.timer_action = TIMER_ON;
                      else if (!strcmp(optarg, "off")) cfg.timer_action = TIMER_OFF;
                      else if (!strcmp(optarg, "toggle")) cfg.timer_action = TIMER_TOGGLE;
                      else {
                          fprintf( stderr, "-a: expected on, off or toggle, got '%s'\n", optarg );
                          exit(EXIT_FAILURE);
                      }
                      break;

            case 'W': strcpy(cfg.webcam_path, optarg); cfg.local_only = 1; break;
            case 'w': atou64N(optarg, &cfg.webcam_br_min, 2); cfg.local_only = 1; break;
            case 'd': atou64N(optarg, &cfg.webcam_nx, 2); cfg.local_only = 1; break;

            case 'S': strcpy(cfg.screen_path, optarg); cfg.local_only = 1; break;
            case 's': cfg.screen_br_min = atof(optarg); cfg.local_only = 1; break;

            case 'K': strcpy(cfg.kbd_path, optarg); cfg.local_only = 1; break;
            case 'k': cfg.kbd_br_max = atof(optarg); cfg.local_only = 1; break;
            case 'm': cfg.kbd_val_min = atof(optarg); cfg.local_only = 1; break;

            case 'F': strcpy(cfg.function_name, optarg); cfg.local_only = 1; break;
            case 'f': strcpy(cfg.function_params, optarg); cfg.local_only = 1; break;

            case 'h': fprintf( stderr,
"Usage: %s [-c /path/to/config/file] [-v percent|auto] [-V:verbose]\n"
"       %s [-W /path/to/video] [-w webcam absoulte limits (min,max)]\n"
"       %s [-d webcam dim (x,y)] [-S /path/to/brightness/]\n"
"       %s [-s screen min brightness (rel value)] [-K /path/to/keyboard/]\n"
"       %s [-k kbd upper bdy (rel val)] [-m kbd lower bdy replace (rel val)]\n"
"       %s [-F function name] [-f function params (a,b,c,…)] [-h:this help]\n"
"       %s [-D:run as daemon] [-R:reload daemon config] [-g:print daemon status]\n"
"       %s [-a on|off|toggle: daemon's automatic updates]\n"
                                   , *pargv[0], pad, pad, pad, pad, pad, pad, pad );
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

