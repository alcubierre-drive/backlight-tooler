# backlight-tooler

## Synopsis

    backlight-tooler [-V] [-v value|auto]
    backlight-tooler -D
    backlight-tooler -R
    backlight-tooler -g
    
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
| `ScreenBrightness` | property `d` | screen brightness in [0,1] |
| `KbdBrightness` | property `d` | keyboard brightness in [0,1] |
| `LastUpdate` | property `t` | time of last change, µs since the epoch |
| `Interval` | property `t` | automatic update interval in µs, 0 = off |
| `ConfigFile` | property `s` | config file in use |
| `Update()` | method | measure and set brightness now |
| `Adjust(d delta)` | method | change screen brightness by delta in [-1,1] |
| `ReloadConfig()` | method | re-read the config file; fails and keeps the old config if the file is unreadable or malformed |

All properties emit `PropertiesChanged`. `backlight-tooler -g` prints them,
`backlight-tooler -R` (also `systemctl --user reload backlight-tooler`, or
SIGHUP to the daemon) reloads the config. For example:

    busctl --user get-property org.BacklightTooler /org/BacklightTooler org.BacklightTooler ScreenBrightness

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

## Configuration

All config is done in `/etc/backlight-tooler.conf`. The default values might work
on an intel-igpu system.
