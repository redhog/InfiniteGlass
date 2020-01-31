import unittest
import subprocess
import os
import signal
import InfiniteGlass
import glass_theme
import time
import math

class RendererTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.xserver = subprocess.Popen(["Xephyr", ":100", "-ac", "-screen", "1280x768", "-host-cursor"])
        os.environ["DISPLAY"] = ":100"
        os.environ["GLASS_EVENTLOG_renderer"] = "1"
    
    @classmethod
    def tearDownClass(cls):
        os.kill(cls.xserver.pid, signal.SIGKILL)
        cls.xserver.wait()

    def setUp(self):
        self.renderer = subprocess.Popen(["build/glass-renderer"])
        while True:
            try:
                self.display = InfiniteGlass.Display()
                break
            except:
                time.sleep(0.5)
        glass_theme.setup_views(self.display)
        glass_theme.setup_shaders(self.display)
        self.test_done = False
        
    def tearDown(self):
        self.display.flush()
        while not self.test_done:
            self.display.mainloop.do()
        os.kill(self.renderer.pid, signal.SIGUSR1)
        self.renderer.wait()

    def test_terminal1(self):
        self.terminal1 = subprocess.Popen(["xterm"])
        
        @self.display.mainloop.add_timeout(time.time() + 3)
        def done(timestamp):
            os.kill(self.terminal1.pid, signal.SIGKILL)
            self.terminal1.wait()
            self.test_done = True


    def test_terminal_view_animation(self):
        self.terminal1 = subprocess.Popen(["xterm"])

        @self.display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            event.window["IG_COORDS"] = [2.0, 1.5, 0.5, 0.5]
            self.display.flush()
        
        @self.display.mainloop.add_timeout(time.time() + 6)
        def done(timestamp):
            os.kill(self.terminal1.pid, signal.SIGKILL)
            self.terminal1.wait()
            self.test_done = True
            
        @self.display.mainloop.add_interval(0.03)
        def step(timestamp, idx):
            v = 2*math.pi * idx / 100
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = [math.cos(v), math.sin(v), 4.0, 0.0]
            self.display.flush()

    def test_terminal_coords_animation(self):
        self.terminal1 = subprocess.Popen(["xterm"])

        self.display.root["IG_VIEW_DESKTOP_VIEW"] = [0., 0., 4.0, 0.0]

        
        @self.display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            window = event.window
            
            @self.display.mainloop.add_interval(0.03)
            def step(timestamp, idx):
                v = 2*math.pi * idx / 100
                window["IG_COORDS"] = [1.5 + math.cos(v), 1.5 + math.sin(v), 0.5, 0.5]
                self.display.flush()
        
        @self.display.mainloop.add_timeout(time.time() + 6)
        def done(timestamp):
            os.kill(self.terminal1.pid, signal.SIGKILL)
            self.terminal1.wait()
            self.test_done = True
            
    def test_multiterminal_view_animation(self):
        self.terminals = [subprocess.Popen(["xterm"]) for x in range(0, 40)]
        self.terminals_by_pid = {t.pid:(idx, t) for idx, t in enumerate(self.terminals)}
        
        @self.display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            pid = event.window["_NET_WM_PID"]
            idx, t = self.terminals_by_pid[pid]
            event.window["IG_COORDS"] = [2. + 0.1 * (idx % 10), 1.5 + 0.1 * (idx // 10), 0.09, 0.09]
            self.display.flush()
        
        @self.display.mainloop.add_timeout(time.time() + 12)
        def done(timestamp):
            for t in self.terminals:
                os.kill(t.pid, signal.SIGKILL)
                t.wait()
            self.test_done = True
            
        @self.display.mainloop.add_interval(0.03)
        def step(timestamp, idx):
            v = 2*math.pi * idx / 100
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = [math.cos(v), math.sin(v), 4.0, 0.0]
            self.display.flush()
