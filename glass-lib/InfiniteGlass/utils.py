import importlib
import pkg_resources
import os.path
import contextlib
import slugify
import ctypes
import os
import platform

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



libc = ctypes.CDLL(None, use_errno=True)

# glibc ≥ 2.36
try:
    _pidfd_open = libc.pidfd_open
    _pidfd_open.argtypes = (ctypes.c_int, ctypes.c_uint)
    _pidfd_open.restype = ctypes.c_int

    def pidfd(pid: int) -> int:
        fd = _pidfd_open(pid, 0)
        if fd == -1:
            err = ctypes.get_errno()
            raise OSError(err, os.strerror(err))
        return fd

except AttributeError:
    # Syscall numbers by architecture (Linux)
    SYS_PIDFD_OPEN = {
        "x86_64":   434,
        "aarch64":  434,
        "armv7l":   434,
        "armv6l":   434,
        "riscv64":  434,
        "ppc64le":  434,
        "s390x":    434,
        "loongarch64": 434,
        "mips64":   434,
        "mips64el": 434,
    }

    arch = platform.machine()
    try:
        syscall_nr = SYS_PIDFD_OPEN[arch]
    except KeyError:
        raise RuntimeError(f"pidfd_open syscall not supported on architecture: {arch}")

    libc.syscall.argtypes = (
        ctypes.c_long,  # syscall
        ctypes.c_int,   # pid
        ctypes.c_uint,  # flags
    )
    libc.syscall.restype = ctypes.c_long

    def pidfd(pid: int) -> int:
        fd = libc.syscall(syscall_nr, pid, 0)
        if fd == -1:
            err = ctypes.get_errno()
            raise OSError(err, os.strerror(err))
        return fd
