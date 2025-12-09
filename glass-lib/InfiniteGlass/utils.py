import importlib
import pkg_resources
import os.path
import contextlib
import slugify

def _generate_key_stringify(value):
    if value is None:
        return ""
    if isinstance(value, (list, tuple)):
        return "-".join(_generate_key_stringify(item) for item in value)
    if isinstance(value, bytes):
        try:
            return value.decode("utf-8")
        except:
            pass
    return str(value)

def generate_key(properties, use):
    return slugify.slugify(_generate_key_stringify([properties.get(name, None) for name in sorted(use)]))

@contextlib.contextmanager
def open_file(name):
    name = os.path.expanduser(name)
    if name.startswith("resource://"):
        pkg, name = name.split("://")[1].split("/", 1)
        with pkg_resources.resource_stream(pkg, name) as f:
            yield f
    else:
        with open(name) as f:
            yield f

def read_file(name):
    with open_file(name) as f:
        return f.read()
