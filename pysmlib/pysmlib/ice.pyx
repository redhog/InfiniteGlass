
from libc.stdlib cimport *
from libc.string cimport *
from pysmlib.SMlib cimport *

cedf class PyIceConn(object):
    cdef IceConn conn
    def __init__(self, IceConn conn):
        self.conn = conn

    def IceProcessMessages(self):
        res = IceProcessMessages(self.conn, NULL, NULL)
        if res != 0:
            raise Exception(
                ["IceProcessMessagesSuccess",
                 "IceProcessMessagesIOError",
                 "IceProcessMessagesConnectionClosed"][res])
        