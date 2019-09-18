import InfiniteGlass, Xlib.X
import struct
import array
import pkg_resources
import sqlite3
import os.path
import json
import array
import base64
import uuid
import glass_ghosts.shadow
import glass_ghosts.helpers
            
class Client(object):
    def __init__(self, session, client_id = None, properties = None):
        self.session = session
        self.client_id = client_id or uuid.uuid4().hex.encode("ascii")
        self.properties = properties or {}

    def __getitem__(self, name):
        return self.properties[name]
    def __setitem__(self, name, value):
        self.properties[name] = value
    def __delitem__(self, name):
        del self.properties[name]
    def update(self, props):
        self.properties.update(props)
