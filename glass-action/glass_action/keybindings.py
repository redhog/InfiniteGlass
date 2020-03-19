import glass_action.main
import glass_action.input2txt

@glass_action.main.main.command()
@click.pass_context
def keybindings(ctx):
    glass_action.input2txt.format_input_config()
    
