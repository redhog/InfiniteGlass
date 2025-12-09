import re
from . import utils

def load_shader(name, defines = {}):
    defines_str = ''.join(
        "#define %s %s\n" % (name, value)
        for name, value in defines.items()).encode("utf-8")
    preamble = ""
    src = _load_shader(name, defines)
    if b"#version" in src:
        vi = src.index(b"#version")
        pi = vi + src[vi:].index(b"\n") + 1
        preamble = src[:pi]
        src = src[pi:]
    return preamble + defines_str + src

def _load_shader(name, defines):
    src = utils.read_file(name)
    includes = re.findall(rb'^#include  *"(.*)"', src, re.MULTILINE)
    for name in includes:
        src = re.sub(
            rb'#include  *"%s"' % (name,),
            _load_shader(name.decode("utf-8"), defines),
            src, re.MULTILINE)
    return src
