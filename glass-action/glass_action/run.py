import click
import os
import glass_action.main
import distutils

@glass_action.main.main.command()
@click.option('--id', '-i', help="Set the application id for this instance of the app")
@click.option('--app', '-a', multiple=True, help="PROPNAME=value - set a property on the windows of an app")
@click.option('--group', '-g', multiple=True, help="PROPNAME=value - set a property on the windows of an app and all its children")
@click.option('--restartable', '-r', is_flag=True, help="Set WM_COMMAND")
@click.argument('arguments', nargs=-1)
@click.pass_context
def run(ctx, id=None, app=[], group=[], restartable=False, arguments=[]):
    """Run an X application and annotate all its windows with extra properties.
Optionally sets WM_COMMAND for the application with a unique application ID.
Can set some properties on windows of all child processes too.

Property values can be any string, or __WM_COMMAND__ or __APPID__.

ARGUMENTS is the command to run, prefixed by --
"""
    env = dict(os.environ)
    preload = [distutils.spawn.find_executable('glass-annotator')]
    if "LD_PRELOAD" in env:
        preload.extend(env["LD_PRELOAD"].split(" "))
    env["LD_PRELOAD"] = " ".join(preload)
    if id is not None:
        env["IG_APPID"] = id
    for pair in app:
        name, value = pair.split("=")
        env["IG_APP_" + name] = value
    for pair in group:
        name, value = pair.split("=")
        env["IG_GROUP_" + name] = value
    if restartable:
        env["IG_APP_WM_COMMAND"] = "__WM_COMMAND__"
    os.execvpe(arguments[0], arguments, env)
