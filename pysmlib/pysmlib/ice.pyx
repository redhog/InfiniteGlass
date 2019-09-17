
from libc.stdlib cimport *
from libc.string cimport *
from pysmlib.SMlib cimport *
from pysmlib.ice cimport *

cdef void ice_ping_reply_wrapper(IceConn ice_conn, IcePointer client_data):
    data = <tuple> client_data
    self, method = data
    method()
    del self.refs[id(data)]

cdef class PyIceConn(object):
    cdef PyIceConn init(self, IceConn conn):
        self.conn = conn
        self.refs = {}
        return self
    
    def IceConnectionNumber(self):
        return IceConnectionNumber(self.conn)

    def IceConnectionStatus(self):
        return ["IceConnectPending",
                "IceConnectAccepted",
                "IceConnectRejected",
                "IceConnectIOError"][IceConnectionStatus(self.conn)]
    
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
        IcePing(self.conn, &ice_ping_reply_wrapper, <IcePointer> data)

cdef class PyIceListenObj(object):
    cdef PyIceListenObj init(self, IceListenObj obj):
        self.obj = obj
        return self
    
    def IceGetListenConnectionNumber(self):
        return IceGetListenConnectionNumber(self.obj)

    def IceGetListenConnectionString(self):
        return IceGetListenConnectionString(self.obj);

    def IceAcceptConnection(self):
        cdef IceAcceptStatus status_ret
        cdef IceConn conn = IceAcceptConnection(self.obj, &status_ret)
        if status_ret != 0:
            raise Exception(["IceAcceptSuccess",
                             "IceAcceptFailure",
                             "IceAcceptBadMalloc"][status_ret])
        return PyIceConn().init(conn)
        
def PyIceSetPaAuthData(entries):
    cdef IceAuthDataEntry *data = <IceAuthDataEntry *>malloc(sizeof(IceAuthDataEntry) * len(entries))
    for idx, entry in enumerate(entries):
        data[idx].protocol_name = entry[b"protocol_name"]
        data[idx].network_id = entry[b"network_id"]
        data[idx].auth_name = entry[b"auth_name"]
        data[idx].auth_data_length = len(entry[b"auth_data"])
        data[idx].auth_data = entry[b"auth_data"]
    IceSetPaAuthData(len(entries), data)
    free(data)

def PyIceGenerateMagicCookie(int length):
    return IceGenerateMagicCookie(length)
