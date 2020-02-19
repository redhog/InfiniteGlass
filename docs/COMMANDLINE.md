InfiniteGlass internally uses window properties to communicate between
its various components. This makes it very easy to script from most
programming languages, in particular Python, for which there is a
library of functions, glass-lib to help you.

However, to control InfiniteGlass from the command line, you need a
command line tool that's preferably a bit more high-level than window
properties.

    glass-action

is that tool.

glass-action provides subcommands to animate and inspect window
properties, to send messages to windows, to list, start and stop
components, to list import and export ghosts and session clients and
to inspect the current key bindings and get help on possible keyboard
actions.

Some of the command, like the window interactions require the window
manager to be running, while others, like manipulating ghosts, work
directly with the stored database and can be run at any time.

The export and import commands all use JSON as interchange format, and
the semantics of the exported JSON is compatible to the relevant
sections of the config files (e.g. ghosts.json).
