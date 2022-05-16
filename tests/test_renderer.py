import unittest
import subprocess
import os
import signal
import InfiniteGlass
import glass_theme.default
import time
import math
import tempfile
import Xlib.X
import yaml

class DataType(object):
    def __init__(self, name, data):
        self.name, self.data = name, data
    def __repr__(self):
        return "%s %s" % (self.name, repr(self.data))

yaml.add_constructor("!atom", lambda l, n: DataType("!atom", l.construct_sequence(n)))
yaml.add_constructor("!coords", lambda l, n: DataType("!coords", l.construct_sequence(n)))
yaml.add_constructor("!float", lambda l, n: DataType("!float", l.construct_sequence(n)))
yaml.add_constructor("!int", lambda l, n: DataType("!int", l.construct_sequence(n)))
yaml.add_constructor("!_NET_WM_ICON", lambda l, n: DataType("!_NET_WM_ICON", l.construct_scalar(n)))
yaml.add_constructor("!shader_program", lambda l, n: DataType("!shader_program", l.construct_scalar(n)))
yaml.add_constructor("!STRING", lambda l, n: DataType("!STRING", l.construct_scalar(n)))
yaml.add_constructor("!svg", lambda l, n: DataType("!svg", l.construct_scalar(n)))
yaml.add_constructor("!UTF8_STRING", lambda l, n: DataType("!UTF8_STRING", l.construct_scalar(n)))
yaml.add_constructor("!window", lambda l, n: DataType("!window", l.construct_sequence(n)))
yaml.add_constructor("!WM_SIZE_HINTS", lambda l, n: DataType("!WM_SIZE_HINTS", l.construct_scalar(n)))



class Theme(glass_theme.default.Theme):
    mode = "no_splash"


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
        self.stdout_file = tempfile.NamedTemporaryFile(buffering=0)
        self.stderr_file = tempfile.NamedTemporaryFile(buffering=0)
        self.renderer = subprocess.Popen(["build/glass-renderer"], stdout=self.stdout_file, stderr=self.stderr_file)
        while True:
            try:
                self.display = InfiniteGlass.Display()
                break
            except:
                time.sleep(0.5)
        self.theme = Theme(self.display)
        self.display.flush()
        self.test_done = False
        
    def tearDown(self):
        self.display.flush()
        while not self.test_done:
            self.display.mainloop.do()
        self.stdout_file.close()
        self.stderr_file.close()
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
        self.terminals = [subprocess.Popen(["xterm"]) for x in range(0, 100)]
        self.terminals_by_pid = {t.pid:(idx, t) for idx, t in enumerate(self.terminals)}
        
        @self.display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            pid = event.window["_NET_WM_PID"]
            idx, t = self.terminals_by_pid[pid]
            event.window["IG_COORDS"] = [2. + 0.1 * (idx % 10), 1.5 + 0.1 * (idx // 10), 0.09, 0.09]
            self.display.flush()
        
        @self.display.mainloop.add_timeout(time.time() + 60)
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

    def test_multiwindow_view_animation(self):
        self.windows = [self.display.root.create_window(map=False, width=1024, height=1024) for x in range(0, 100)]
        for idx, window in enumerate(self.windows):
            window["IG_COORDS"] = [2. + 0.1 * (idx % 10), 1.5 + 0.1 * (idx // 10), 0.09, 0.09]
            window.map()
        
        @self.display.mainloop.add_timeout(time.time() + 60)
        def done(timestamp):
            self.test_done = True
            
        @self.display.mainloop.add_interval(0.03)
        def step(timestamp, idx):
            v = 2*math.pi * idx / 100
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = [math.cos(v), math.sin(v), 4.0, 0.0]
            self.display.flush()

    def test_subwindow(self):
        self.subwindow = self.display.root.create_window(map=False, width=1024, height=1024)
        self.subwindow["IG_LAYER"] = "IG_NONE"
        self.subwindow["IG_CONTENT"] = ("IG_SVG", "@resource://glass_widgets/fontawesome-free-5.9.0-desktop/svgs/solid/search-dollar.svg")
        self.subwindow["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"
        self.subwindow["IG_COORDS"] = [1.0, -1.0, 0.0, 0.0,
                                       0.01, 0.03, 0.03, 0.03]
        self.subwindow["IG_COORD_TYPES"] = ["IG_COORD_PARENT_BASE", "IG_COORD_SCREEN_X"]
        self.subwindow.map()
        
        self.window = self.display.root.create_window(map=False, width=1024, height=1024)
        self.window["IG_COORDS"] = [0.25, 0.5, 0.5, 0.4]
        self.window["IG_CONTENT"] = ("IG_SVG", "@resource://glass_widgets/fontawesome-free-5.9.0-desktop/svgs/solid/search-minus.svg")
        self.window.map()

        self.display.root["IG_SUB1"] = ("IG_ITEM", self.subwindow)
        self.display.flush()
        
        @self.display.mainloop.add_timeout(time.time() + 60)
        def done(timestamp):
            self.test_done = True

    def test_list_shaders(self):
        self.display.root.send(self.display.root, "IG_DEBUG_LIST_SHADERS", event_mask=Xlib.X.SubstructureNotifyMask)
        @self.display.mainloop.add_timeout(time.time() + 1)
        def done(timestamp):
            self.test_done = True
            with open(self.stdout_file.name) as f:
                output = f.read()
                output = yaml.load(output, Loader=yaml.Loader)
                assert "shaders" in output
                
