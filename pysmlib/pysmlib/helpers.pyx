from pysmlib.SMlib cimport *
from pysmlib.Python cimport *
from pysmlib.helpers cimport *
from libc.stdlib cimport malloc, free

cdef object smprops_to_dict(int numProps, SmProp **props):
    propdict = {}
    for idx in range(numProps):
        values = []
        for valueidx in range(props[idx].num_vals):
            values.append(PyBytes_FromStringAndSize(<char *> props[idx].vals[valueidx].value, props[idx].vals[valueidx].length).decode("utf-8"))
        propdict[bytes(props[idx].name).decode("utf-8")] = (bytes(props[idx].type).decode("utf-8"), values)
    return propdict

cdef void dict_to_smprops(object propsdict, int *numProps, SmProp ***props):
    cdef SmProp **propsarr
    cdef SmProp *prop
    
    numProps[0] = len(propsdict)

    propsarr = <SmProp **>malloc(sizeof(SmProp *) * len(propsdict))
    props[0] = propsarr
    
    for idx, (name, value) in enumerate(propsdict):
        prop = <SmProp *>malloc(sizeof(SmProp))
        propsarr[idx] = prop
         
        name = name.encode("utf-8")
        prop.name = name

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
            prop.vals[idx].length = len(value[idx]) + 1
            prop.vals[idx].value = <char*> value[idx]
        
cdef void free_smprops(int numProps, SmProp **props):
    cdef SmProp *prop
    for idx in range(numProps):
        prop = props[idx]
        free(prop.vals)
        free(prop)
    free(props)
