# Shell script sourced by the startup scripts before any components come up.

# Main usage is to set environment variables. Below is a set of
# example variables and values that you can uncomment.

# See DEBUGGING.md for more information

# export GLASS_DEBUG_renderer=1
# export GLASS_DEBUG_renderer_property=0
# export GLASS_DEBUG_renderer_property_c_property_load_prop_changed=1

# export GLASS_DEBUG_glass_ghosts=1
# export GLASS_DEBUG_glass_input=1

# The eventlog is a newline separated JSON file that can be used to
# generate statistics of X events to the renderer and their processing
# time.
# export GLASS_EVENTLOG_renderer=1

# Error paths work just like debug paths, but are =1 by default. You
# can disable all error reporting by uncommenting the following line
# export GLASS_ERROR_renderer=0
