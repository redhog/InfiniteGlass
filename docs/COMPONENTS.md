Implemented by: [`glass-components`](../glass-components)

The components configuration is done in
[~/.config/glass/components.yml](../glass-config-init/glass_config_init/components.yml).

# Components

Components are applications to run during your session or at startup
that provide some part of the desktop environment. As opposed to
normal applications used by the user, they are not normally started
and stopped by the user and do not provide windows managed by the
user. In particular, they are not SMlib clients and do not store a
RestartCommand with `glass-ghosts`.

However, each component is given a name, and can be restarted or
replaced using the `glass-action component` subcommands with that
name.
