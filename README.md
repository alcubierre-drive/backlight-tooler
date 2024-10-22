# backlight-tooler

## Synopsis

    backlight-tooler -V value|auto
    
    backlight-tooler-service-toggle

## Usage

For the tool to work, the systemd-service
`backlight-tooler-change-permissions` has to be enabled and started. The
systemd-user timer `backlight-tooler.timer` can be enabled to get
automatic handling every 15 minutes. For changes of the default `auto`
value, consider copying
`/usr/lib/systemd/user/backlight-tooler.service` to
`~/.config/systemd/user/` and edit the file from there.

## Example .xbindkeysrc

    "backlight-tooler inc"
    XF86MonBrightnessUp
    
    "backlight-tooler dec"
    XF86MonBrightnessDown
    
    "systemctl --user start backlight-tooler.service"
    XF86Launch1
    
    "systemctl --user stop backlight-tooler.timer; backlight-tooler
    toggle"
    XF86Launch2
    
    "backlight-tooler-service-toggle"
    XF86Tools

## Configuration

All config is done in `/etc/backlight-tooler.conf`. The default values might work
on an intel-igpu system.
