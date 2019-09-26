from pysmlib.SMlib cimport *

cdef object smprops_to_dict(int numProps, SmProp **props)
cdef void dict_to_smprops(object propsdict, int *numProps, SmProp ***props)
cdef void free_smprops(int numProps, SmProp **props)
