import click

@click.group()
@click.pass_context
def main(ctx, **kw):
    ctx.obj = {}
