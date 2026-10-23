# backlight-tooler

## Synopsis

    backlight-tooler [-V] [-v value|auto]
    backlight-tooler -D
    backlight-tooler -R
    backlight-tooler -g
    backlight-tooler -a on|off|toggle
    
    backlight-tooler-service-toggle

## Usage

For the tool to work, the systemd-service
`backlight-tooler-change-permissions` has to be enabled and started. The
systemd user service `backlight-tooler.service` runs `backlight-tooler -D`,
a daemon that adjusts the backlight automatically (every 15 minutes by
default, see `[daemon] interval` in the config). While it runs, plain
`backlight-tooler` invocations are forwarded to it over the user bus; without
it, they set the backlight themselves. Options that override the config
(`-c`, `-S`, `-F`, ...) always act locally.

## D-Bus interface

The daemon owns `org.BacklightTooler` on the user bus, object
`/org/BacklightTooler`, interface `org.BacklightTooler`:

| Member | Type | |
|---|---|---|
| `WebcamValue` | property `d` | last webcam reading in [0,1], -1 if none yet |
| `ScreenBrightness` | property `d` | screen brightness in [0,1], -1 if none yet |
| `KbdBrightness` | property `d` | keyboard brightness in [0,1], -1 if none yet |
| `LastUpdate` | property `t` | time of last change, µs since the epoch |
| `Interval` | property `t`, writable | automatic update interval in µs, 0 = off; see below |
| `ConfigInterval` | property `t` | `[daemon] interval` of the loaded config, in µs |
| `ConfigFile` | property `s` | config file in use |
| `ScreenMin` | property `d` | `[screen] br_min` of the loaded config: `Adjust()` doesn't go below it |
| `Update()` | method | measure and set brightness now; fails and changes nothing if the webcam can't be read (missing, or busy with another program) |
| `Adjust(d delta)` | method | change screen brightness by delta in [-1,1], clamped to [`ScreenMin`,1] |
| `ReloadConfig()` | method | re-read the config file; fails and keeps the old config if the file is unreadable or malformed |

All properties emit `PropertiesChanged`. `backlight-tooler -g` prints them,
`backlight-tooler -R` (also `systemctl --user reload backlight-tooler`, or
SIGHUP to the daemon) reloads the config. For example:

    busctl --user get-property org.BacklightTooler /org/BacklightTooler org.BacklightTooler ScreenBrightness

### Stopping and restarting the automatic updates

`Interval` can be set by any client of the user bus. 0 stops the timer
without stopping the daemon (`Update()`, `Adjust()` and `-v` keep working);
any other value (re)starts it with that period, the first update following one
period from now. Values other than 0 must lie in [1 s, 366 d], unless they equal
`ConfigInterval`; others are rejected with
`org.freedesktop.DBus.Error.InvalidArgs`.

    busctl --user set-property org.BacklightTooler /org/BacklightTooler org.BacklightTooler Interval t 0
    busctl --user set-property org.BacklightTooler /org/BacklightTooler org.BacklightTooler Interval t 300000000

`backlight-tooler -a off` does the former, `-a on` sets `Interval` back to
`ConfigInterval` (an error if that is 0), and `-a toggle` switches between
the two. Like `-R` and `-g`, `-a` needs a running daemon.

A value set over the bus survives a config reload as long as the config's
`[daemon] interval` stays the same; if the reloaded config has a different
interval, that one takes effect. With the timer stopped, `-g` prints

    webcam:       50%
    screen:       22%
    keyboard:      0%
    last update: 2026-10-09 11:26:45
    interval:    off
    config file: /etc/backlight-tooler.conf

## Example .xbindkeysrc

    "backlight-tooler inc"
    XF86MonBrightnessUp
    
    "backlight-tooler dec"
    XF86MonBrightnessDown
    
    "backlight-tooler"
    XF86Launch1
    
    "systemctl --user stop backlight-tooler.service; backlight-tooler
    toggle"
    XF86Launch2
    
    "backlight-tooler-service-toggle"
    XF86Tools
    
    "backlight-tooler -a toggle"
    XF86Display

## Configuration

All config is done in `/etc/backlight-tooler.conf`. The default values might work
on an intel-igpu system.
