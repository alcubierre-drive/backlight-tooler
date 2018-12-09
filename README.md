# BacklightTooler

## Synopsis

    BacklightTooler inc|dec|auto amount pulse|toggle
    
    BacklightToolerServiceToggle

## Usage

For the tool to work, the systemd-service
`BacklightToolerChangePermissions` has to be enabled and started. The
systemd-user timer `BacklightTooler.timer` can be enabled to get
automatic handling every 15 minutes. For changes of the default `auto`
value, consider copying
`/usr/lib/systemd/user/BacklightTooler.service` to
`~/.config/systemd/user/` and edit the file from there.

## Example .xbindkeysrc

    "BacklightTooler inc"
    XF86MonBrightnessUp
    
    "BacklightTooler dec"
    XF86MonBrightnessDown
    
    "systemctl --user start BacklightTooler.service"
    XF86Launch1
    
    "systemctl --user stop BacklightTooler.timer; BacklightTooler
    toggle"
    XF86Launch2
    
    "BacklightToolerServiceToggle"
    XF86Tools

## Configuration

All config is done in `/etc/BacklightTooler.conf`. The default values might work
on an intel-igpu system.
