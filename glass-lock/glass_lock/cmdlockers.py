from . import base
import subprocess


class LockerI3(base.Locker):
    cmd = ["i3lock"]
    
    def lock(self):
        proc = subprocess.Popen(self.cmd)
        @self.display.mainloop.add_process(pid=proc.pid)
        def child_exit(retcode):
            self.unlocked()


class LockerXScreensaver(base.Locker):
    cmd_daemon = ["xscreensaver", "-no-splash"]
    cmd_watch = ["xscreensaver-command", "--watch"]
    cmd_lock = ["xscreensaver-command", "-lock"]

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.daemon = subprocess.Popen(
            self.cmd_daemon)

        self.watch = subprocess.Popen(
            self.cmd_watch,
            stdout=subprocess.PIPE,
            text=True,
        )

        @self.display.mainloop.add_lines(self.watch.stdout.fileno())
        def xscreensaver_watch(fd, line):
            if "UNBLANK" in line:
                self.unlocked()
                
    def lock(self):
        subprocess.run(self.cmd_lock)

