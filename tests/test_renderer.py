import unittest
import subprocess
import os
import signal
import InfiniteGlass
import glass_theme
import time

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
            self.test_done = True
