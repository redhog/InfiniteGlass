cdef extern from "Python.h":
    object PyBytes_FromStringAndSize(char *, Py_ssize_t)
    object PyString_FromStringAndSize(char *, Py_ssize_t)
    char *PyString_AsString(object)
