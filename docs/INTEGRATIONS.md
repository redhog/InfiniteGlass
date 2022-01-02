# Session handling integrations

Some applications do not have SMLib support, or might need special configuration to allow saving state.
This document describes some tools for this as well as how to integrate some common applications
that need such configuration.

# Providing a restart command 

For applications that just can't save state, but that you'd like to be able to restart anyway,
you can set up a `template` in [~/.config/glass/ghosts.yml](../glass-config-init/glass_config_init/ghosts.yml)
that matches the window key using a regexp and sets the `WM_COMMAND` window property to the command
line as a list of strings. The template properties will be applied to a matching window when it is first
created and will then be stored in the ghost window when stopped.

# The session wrapper

Some applications can be told to save their state in a specific directory on disk when exiting,
and to reload the state from the same directory. Such applications can be wrapped using the `glass-session-wrapper`
SMLib client. By starting such an application with a command like

    glass-session-wrapper myapp --foo --fie=123 --someswitch=~/myappstate/%(sessionid)s bar

it will be run as a child of `glass-session-wrapper`, with `%(sessionid)s` replaced by the SMLib client id.
It will be sent `SIGINT` when `glass-session-wrapper` receives the `die` signal.

# Common applications

## Chrome/chromium

Chrome does not have any SMlib support. However, it reads/writes its
state from/to a user-specified directory. This means that it can be
started using glass-session-wrapper to get session support:

    glass-session-wrapper chromium-browser --user-data-dir=chrome-sessions/%(sessionid)s

Such a wrapper .desktop menu entry is installed when you run 'make install'.

## Emacs

Emacs have SMlib support, but by default it does not save its list of
open files, or really anything usefull, in its session store. However
this can be easily enabled with [a few simple lines of
emacs-lisp](../scripts/glass-emacs-xsession.el) (as anything can :)

This code is installed for emacs globally on your system when you run
'make install'. You can alternatively just copy this code to your
~/.emacs.
