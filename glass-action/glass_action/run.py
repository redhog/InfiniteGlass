import click
import os
import glass_action.main
import distutils

@glass_action.main.main.command()
@click.option('--restartable', '-r', is_flag=True)
@click.option('--set', '-s', multiple=True)
@click.argument('arguments', nargs=-1)
@click.pass_context
def run(ctx, restartable, set, arguments):
    env = dict(os.environ)
    preload = [distutils.spawn.find_executable('glass-annotator')]
    if "LD_PRELOAD" in env:
        preload.extend(env["LD_PRELOAD"].split(" "))
    env["LD_PRELOAD"] = " ".join(preload)
    for pair in set:
        name, value = pair.split("=")
        env["IG_ANNOTATE_" + name] = value
    if restartable:
        env["IG_ANNOTATE_WM_COMMAND"] = "__WM_COMMAND__"
    os.execvpe(arguments[0], arguments, env)
