The session configuration is done in
[~/.config/glass/ghosts.json](../glass-config-init/glass_config_init/ghosts.json).

# Components

Components are applications to run during your session or at startup
that provide some part of the desktop environment. As opposed to
normal applications used by the user, they are not normally started
and stopped by the user and do not provide windows managed by the
user. In particular, they are not SMlib clients and do not store a
RestartCommand with glass-ghosts.

However, each component is given a name, and can be restarted or
replaced using the 'glass-action component' subcommands with that
name.

# Clients and ghosts

glass-ghosts stores window positions and sizes. Such a stored
window state is called a ghost. Ghosts are displayed on the desktop
as special windows at the position and size of the original window.
They can be moved, resized and closed as any window. When a new
application window is opened, it is matched against exisating ghosts,
and if a match is found, the window is moved and resized to match the
ghost and the ghost removed.

Exactly what properties are saved, restored and used for matching is
configurable.

glass-ghosts additionally acts as an SMlib server, meaning that it can
store and restore application state for supporting applications. Each
such application is called a client.

Ghosts for windows of SMlib supporting clients are marked with the
client id, and the client can be restarted by clicking on the ghost.

## Pre-defined ghosts

Ghosts (and client states) are generally stored between restarts in
an sqlite file. However, in some cases it is necessary to practical to
pre-define some ghosts in the configuration file. This is
particularly useful for windows of components (see above). To generate the correct json for such a ghost, the following command can be used

    glass-action ghost export
