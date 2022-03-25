from pysmlib.SMlib cimport *
from pysmlib.Python cimport *
from pysmlib.helpers cimport *
from libc.stdlib cimport malloc, free
from libc.string cimport strcpy, strlen

cdef extern size_t strnlen(const char *s, size_t maxlen)

cdef object smprops_to_dict(int numProps, SmProp **props):
    cdef size_t len;
    cdef char *s;
    propdict = {}
    for idx in range(numProps):
        values = []
        for valueidx in range(props[idx].num_vals):
            value = <char *> props[idx].vals[valueidx].value
            len = props[idx].vals[valueidx].length
            len = strnlen(value, len);
            values.append(PyBytes_FromStringAndSize(value, len).decode("utf-8"))
        name = bytes(props[idx].name).decode("utf-8")
        type = bytes(props[idx].type).decode("utf-8")
        propdict[name] = (type, values)
    return propdict

cdef char *tostr_malloc(obj):
    cdef char *res
    if not isinstance(obj, (bytes, str)):
        obj = str(obj)
    if isinstance(obj, str):
        obj = obj.encode("utf-8")
    res = <char *> malloc(len(obj) + 1)
    strcpy(res, obj)
    return res

cdef void dict_to_smprops(object propsdict, int *numProps, SmProp ***props):
    cdef SmProp **propsarr
    cdef SmProp *prop
    
    numProps[0] = len(propsdict)

    propsarr = <SmProp **>malloc(sizeof(SmProp *) * len(propsdict))
    props[0] = propsarr

    if isinstance(propsdict, dict):
        propsdict = propsdict.items()
    for idx, (name, value) in enumerate(propsdict):
        prop = <SmProp *>malloc(sizeof(SmProp))
        propsarr[idx] = prop
         
        prop.name = tostr_malloc(name)

        if isinstance(value, (tuple, list)):
            prop.type = b"SmLISTofARRAY8"
            prop.num_vals = len(value)
        else:
            prop.type = b"SmARRAY8"
            prop.num_vals = 1
            value = [value]

        value = [val.encode("utf-8") for val in value]
            
        prop.vals = <SmPropValue*> malloc(sizeof(SmPropValue) * len(value))
        for idx in range(len(value)):
            prop.vals[idx].length = len(value[idx])
            prop.vals[idx].value = tostr_malloc(value[idx])
        
cdef void free_smprops(int numProps, SmProp **props):
    cdef SmProp *prop
    for idx in range(numProps):
        prop = props[idx]
        free(prop.name)
        for validx in range(prop.num_vals):
            free(prop.vals[validx].value)
        free(prop.vals)
        free(prop)
    free(props)
