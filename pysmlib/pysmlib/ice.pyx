
from libc.stdlib cimport *
from libc.string cimport *
from pysmlib.SMlib cimport *

cdef void ice_ping_reply_wrapper(IceConn ice_conn, IcePointer client_data):
    data = <tuple> client_data
    self, method = data
    method()
    del self.refs[id(data)]

cedf class PyIceConn(object):
    cdef IceConn conn

    cdef object refs
    
    cdef PyIceConn init(self, IceConn conn):
        self.conn = conn
        self.refs = {}
        return self
        
    def IceProcessMessages(self):
        res = IceProcessMessages(self.conn, NULL, NULL)
        if res != 0:
            raise Exception(
                ["IceProcessMessagesSuccess",
                 "IceProcessMessagesIOError",
                 "IceProcessMessagesConnectionClosed"][res])

    def IcePing(self, ice_ping_reply):
        data = (self, ice_ping_reply)
        self.refs[id(data)] = data
        IcePing(self.iceconn, &ice_ping_reply_wrapper, <IcePointer> data)

cdef class PyIceListenObj(object):
    cedf IceListenObj obj
    cdef PyIceListenObj init(self, IceListenObj obj):
        self.obj = IceListenObj
        return self
    
    def IceGetListenConnectionNumber(self):
        return IceGetListenConnectionNumber(self.obj)
