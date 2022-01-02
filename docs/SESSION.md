The session configuration is done in
[~/.config/glass/ghosts.yml](../glass-config-init/glass_config_init/ghosts.yml).


# Clients and ghosts

InfiniteGlass supports a slightly different session management paradigm to most desktop environments. It allows you to save
the states and positions of individual windows and apps:

* You can save an app using WIN+S. If the app does not support saving, it just exits, closing all its windows.
* You can close a window using WIN+C.
* All windows will leave a ghost window when they are closed. If they are connected to an app that supports saving,
  clicking on them will restart the app,
* Ghosts are stored between sessions in ~/.config/glass/ghosts.sqlite3
* Ghosts can be closed, that is, deleted, using WIN+C.
* When a new window appear with the same window title as a ghost, it is moved and resized to where the ghost is,
  and replaces the ghost. 

The reasoning behind this is that you should be able to set up groups of workspaces on the infinite desktop for different
purposes, and restart them as you see fit, always having say "social apps" far up to the left in the same spot.

Exactly what window properties are saved, restored and used for matching ghosts against new windows is
configurable.

`glass-ghosts` acts as an SMlib server, meaning that it can
store and restore application state for supporting applications. Each
such application is called a client.

[INTEGRATIONS.md](INTEGRATIONS.md) describes how to adapt/wrap applications that do not natively support SMlib to
support restart, and in some cases, rudimentary restore functionality.

## Pre-defined ghosts

Ghosts (and client states) are generally stored between restarts in
an sqlite file. However, in some cases it is necessary to practical to
pre-define some ghosts in the configuration file. This is
particularly useful for windows of components (see above). To generate the correct json for such a ghost, the following command can be used

    glass-action ghost export

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
