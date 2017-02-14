
The `container` binary needs the lxc-jogr template,
so put a symlink to the template in /usr/share/lxc/templates/.

The template has hard coded paths currently that needs
to be changed by anyone that uses it.

`initprocess` is started as the init process in the
LXC container set up by `container`.

This is all just for some temporary testing and
troubleshooting.
