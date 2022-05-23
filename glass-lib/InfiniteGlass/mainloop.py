import sys
import time
import traceback
import math
import select

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
            
