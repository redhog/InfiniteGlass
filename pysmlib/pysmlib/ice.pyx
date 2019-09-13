
from libc.stdlib cimport *
from libc.string cimport *
from pysmlib.SMlib cimport *

cedf class PyIceConn(object):
    cdef IceConn conn
    
    cdef PyIceConn init(self, IceConn conn):
        self.conn = conn
        return self
        
    def IceProcessMessages(self):
        res = IceProcessMessages(self.conn, NULL, NULL)
        if res != 0:
            raise Exception(
                ["IceProcessMessagesSuccess",
                 "IceProcessMessagesIOError",
                 "IceProcessMessagesConnectionClosed"][res])
        