# Session handling integrations

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
