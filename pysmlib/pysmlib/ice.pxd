from pysmlib.SMlib cimport *

cdef class PyIceConn(object):
    cdef init(self, IceConn conn)

