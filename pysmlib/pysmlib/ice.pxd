from pysmlib.SMlib cimport *

cdef class PyIceConn(object):
    cdef PyIceConn init(self, IceConn conn)

cdef class PyIceListenObj(object):
    cdef PyIceListenObj init(self, IceListenObj obj)
