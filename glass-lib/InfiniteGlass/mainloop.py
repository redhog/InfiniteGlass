import sys
import time
import traceback
import math
import select
import os
import ctypes
from .utils import pidfd

class MainLoop(object):
    def __init__(self):
        self.handlers = {}
        self.timeouts = {}
        self.hf = set()

    def add(self, fd, handler=None):
        if handler is None:
            def wrapper(handler):
                self.add(fd, handler)
            return wrapper
        self.handlers[fd] = handler
        return fd

    def add_hf(self, handler=None):
        if handler is None:
            def wrapper(handler):
                self.add_hf(handler)
            return wrapper
        self.hf.add(handler)
        return handler
    
    def add_timeout(self, timestamp, handler=None):
        if handler is None:
            def wrapper(handler):
                self.add_timeout(timestamp, handler)
            return wrapper
        self.timeouts[timestamp] = handler
        return timestamp

    def add_interval(self, interval, handler=None):
        start = time.time()
        if handler is None:
            def wrapper(handler):
                self.add_interval(interval, handler)
            return wrapper
        def timeout_handler(timestamp):
            idx = math.floor((timestamp - start) / interval)
            try:
                handler(timestamp, idx)
            except StopIteration:
                pass
            else:
                self.add_timeout(interval * (idx + 1), timeout_handler)
        self.add_timeout(start, timeout_handler)

    def remove(self, fd):
        if fd not in self.handlers: return
        del self.handlers[fd]

    def do(self, catch_errors=True):
        keys = self.handlers.keys()
        rlist, wlist, xlist = select.select(keys, [], [], 0.01)
        timestamp = time.time()
        for timeout in list(self.timeouts.keys()):
            if timeout < timestamp:
                try:
                    self.timeouts.pop(timeout)(timestamp)
                except Exception as e:
                    if not catch_errors:
                        raise
                    sys.stderr.write("%s\n" % e)
                    traceback.print_exc(file=sys.stderr)
                    sys.stderr.flush()
        for fd in rlist:
            try:
                self.handlers[fd](fd)
            except Exception as e:
                if not catch_errors:
                    raise
                sys.stderr.write("%s\n" % e)
                traceback.print_exc(file=sys.stderr)
                sys.stderr.flush()
        for handler in self.hf:
            try:
                handler()
            except Exception as e:
                if not catch_errors:
                    raise
                sys.stderr.write("%s\n" % e)
                traceback.print_exc(file=sys.stderr)
                sys.stderr.flush()

    def add_lines(self, fd, handler=None):
        """
        Decorator: invoke the wrapped callback whenever a *full line*
        is read from the given file object.

        Handles:
        - registering FD with display.mainloop
        - buffering partial reads
        - non-blocking multi-byte draining of FD
        """

        if handler is None:
            def wrapper(handler):
                self.add_lines(fd, handler)
            return wrapper
        
        buffer = bytearray()

        @self.add(fd)
        def reader(_):
            nonlocal buffer

            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                self.remove(fd)
                return
            except OSError:
                self.remove(fd)
                return

            if not chunk:
                if buffer:
                    try:
                        line = buffer.decode(errors="replace")
                        handler(fileobj, line)
                    except Exception:
                        pass
                self.remove(fd)
                return

            buffer.extend(chunk)

            while len(buffer):
                nl = buffer.find(b"\n")
                if nl == -1:
                    break
                line = buffer[:nl+1]
                buffer = buffer[nl+1:]
                handler(fd, line.decode(errors="replace"))
        return reader

    def add_process(self, pid, handler=None):
        """
        Wait for a process to exit using its PID and pidfd_open (Linux 5.3+).
        The handler is called with the process exit code.
        """

        if handler is None:
            def wrapper(handler):
                return self.add_process(pid, handler)
            return wrapper
        
        fd = pidfd(pid)

        @self.add(fd)
        def on_exit(fd_ready):
            try:
                retcode, _ = os.waitpid(pid, os.WNOHANG)
            finally:
                self.remove(fd)
                os.close(fd)
            handler(retcode)

        return fd
