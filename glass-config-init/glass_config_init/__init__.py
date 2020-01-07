import pkg_resources
import os.path
import fnmatch

def fnmatchany(name, *pats):
    for pat in pats:
        if fnmatch.fnmatch(name, pat):
            return True
    return False
    
def copy_dir(package, dst, src="/", exclude=[]):
    if not os.path.exists(dst):
        os.makedirs(dst)
    for name in pkg_resources.resource_listdir(package, src):
        srcpath = os.path.join(src, name)
        dstpath = os.path.join(dst, name)
        if fnmatchany(name, *exclude):
            continue
        if pkg_resources.resource_isdir(package, srcpath):
            copy_dir(package, dstpath, srcpath, exclude)
        else:
            if not os.path.exists(dstpath):
                with pkg_resources.resource_stream(package, srcpath) as inf:
                    with open(dstpath, "wb") as outf:
                        outf.write(inf.read())

def main(*arg, **kw):
    print("Copying 1")
    copy_dir(
        "glass_config_init",
        os.path.expanduser(os.environ.get("GLASS_CONFIG", "~/.config/glass")),
        src="/",
        exclude=["*.py", "*.pyc", "__*__"])
