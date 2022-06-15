import click
import os
import glass_action.main
import distutils

def substring_in_list(s, lst):
    for item in lst:
        if s in item:
            return True
    return False
        
@glass_action.main.main.command()
@click.option('--id', '-i', help="Set the IG_APP_ID for this instance of the app")
@click.option('--app', '-a', multiple=True, help="PROPNAME=value - set a property on the windows of an app")
@click.option('--group', '-g', multiple=True, help="PROPNAME=value - set a property on the windows of an app and all its children")
@click.argument('arguments', nargs=-1)
@click.pass_context
def run(ctx, id=None, app=[], group=[], restartable=False, arguments=[]):
    """Run an X application and annotate all its windows with extra properties.
Can set some properties on windows of all child processes too.

ARGUMENTS is the command to run, prefixed by --
"""
    
    env = dict(os.environ)
    preloads = []
    if "LD_PRELOAD" in env:
        preloads = env["LD_PRELOAD"].split(" ")
    if not substring_in_list('glass-annotator', preloads):
        preloads.append(distutils.spawn.find_executable('glass-annotator'))
        env["LD_PRELOAD"] = " ".join(preloads)
        
    if id is not None:
        env["IG_APP_ID"] = id
    for pair in app:
        name, value = pair.split("=")
        env["IG_APP_" + name] = value
    for pair in group:
        name, value = pair.split("=")
        env["IG_GROUP_" + name] = value
    os.execvpe(arguments[0], arguments, env)
