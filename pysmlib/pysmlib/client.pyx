from libc.stdlib cimport malloc, free
import os
from pysmlib.SMlib cimport *
from pysmlib.ice cimport *
from pysmlib.helpers cimport *

SmcSaveYourselfProcMask      = (1L << 0)
SmcDieProcMask               = (1L << 1)
SmcSaveCompleteProcMask      = (1L << 2)
SmcShutdownCancelledProcMask = (1L << 3)

cdef void save_yourself_wrapper(SmcConn smc_conn, SmPointer client_data, int save_type, Bool shutdown, int interact_style, Bool fast):
    self = <PySmcConn> client_data
    if self.save_yourself:
        self.save_yourself(save_type, shutdown, interact_style, fast)

cdef void die_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <PySmcConn> client_data
    if self.die:
        self.die()
     
cdef void shutdown_cancelled_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <PySmcConn> client_data
    if self.shutdown_cancelled:
        self.shutdown_cancelled()
     
cdef void save_complete_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <PySmcConn> client_data
    if self.save_complete:
        self.save_complete()

cdef void save_yourself_phase2_wrapper(SmcConn smcConn, SmPointer client_data):
    data = <PySmcConn> client_data
    (self, proc) = data
    proc()
    del self.refs[id(data)]
        
cdef void prop_reply_proc_wrapper(SmcConn smcConn, SmPointer client_data, int numProps, SmProp **props):
    data = <PySmcConn> client_data
    (self, proc) = data
    proc(smprops_to_dict(numProps, props))
    del self.refs[id(data)]

cdef class PySmcConn(object):
    cdef SmcConn conn
    cdef public PyIceConn iceconn
    cdef public str network_ids_list

    cdef public char *client_id
    cdef SmcCallbacks callbacks

    cdef public object save_yourself
    cdef public object die
    cdef public object save_complete
    cdef public object shutdown_cancelled
    cdef object refs
        
    def __init__(self,
                 str network_ids_list = None,
                 object context = None,
                 int xsmp_major_rev = 1,
                 int xsmp_minor_rev = 0,
                 bytes previous_id = None,

                 save_yourself = None,
                 die = None,
                 save_complete = None,
                 shutdown_cancelled = None):

        self.refs = {}
        
        self.network_ids_list = network_ids_list or os.environ["SESSION_MANAGER"]
        
        cdef char error_string_ret[1024]

        cdef unsigned long mask = 0
        

        if save_yourself: self.save_yourself = save_yourself
        if die: self.die = die
        if save_complete: self.save_complete = save_complete
        if shutdown_cancelled: self.shutdown_cancelled = shutdown_cancelled
        
        self.callbacks.save_yourself.callback = save_yourself_wrapper
        self.callbacks.die.callback = die_wrapper
        self.callbacks.save_complete.callback = save_complete_wrapper
        self.callbacks.shutdown_cancelled.callback = shutdown_cancelled_wrapper

        self.callbacks.save_yourself.client_data = <SmPointer> self
        self.callbacks.die.client_data = <SmPointer> self
        self.callbacks.save_complete.client_data = <SmPointer> self
        self.callbacks.shutdown_cancelled.client_data = <SmPointer> self
        
        self.conn = SmcOpenConnection(
            self.network_ids_list.encode("utf-8"),
            <SmPointer>context if context is not None else NULL,
            xsmp_major_rev,
            xsmp_minor_rev,
            SmcSaveYourselfProcMask | SmcDieProcMask | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask,
            &self.callbacks,
            <char *>previous_id if previous_id else NULL,
            &self.client_id,
            1024,
            error_string_ret)
        self.iceconn = PyIceConn().init(SmcGetIceConnection(self.conn))
 
        if self.conn == NULL:
            raise Exception(error_string_ret)

    def SmcCloseConnection(self, reasons = []):
        cdef char **reasons_arr = <char **> malloc(sizeof(char *) * len(reasons))
        reasons = [reason.encode("utf-8") for reason in reasons]
        
        for idx in range(0, len(reasons)):
            reasons_arr[idx] = reasons[idx]
        res = SmcCloseConnection(self.conn, len(reasons), reasons_arr)
        free(reasons_arr)
        return ["SmcClosedNow",
                "SmcClosedASAP",
                "SmcConnectionInUse"][res]

    def SmcProtocolVersion(self):
        return SmcProtocolVersion(self.conn)
    def SmcProtocolRevision(self):
        return SmcProtocolRevision(self.conn)
    def SmcVendor(self):
        return bytes(SmcVendor(self.conn)).decode("utf-8")
    def SmcRelease(self):
        return bytes(SmcRelease(self.conn)).decode("utf-8")
    
    def SmcSetProperties(self, prop_dict):
        cdef int numProps
        cdef SmProp **props
        dict_to_smprops(prop_dict, &numProps, &props)
        SmcSetProperties(self.conn, numProps, props)
        free_smprops(numProps, props)
        
    def SmcDeleteProperties(self, prop_names):
        cdef char **namestr = <char **>malloc(sizeof(char *) * len(prop_names))
        prop_names = [name.encode("utf-8") for name in prop_names]
        for idx, name in enumerate(prop_names):
            namestr[idx] = name
        SmcDeleteProperties(self.conn, len(prop_names), namestr)
        free(namestr)

    def SmcSaveYourselfDone(self, success):
        SmcSaveYourselfDone(self.conn, success);

    def SmcRequestSaveYourselfPhase2(self, save_yourself_phase2):
        data = (self, save_yourself_phase2)
        self.refs[id(data)] = data
        if not SmcRequestSaveYourselfPhase2(self.conn, &save_yourself_phase2_wrapper, <SmPointer> data):
            raise Exception("Error calling SmcRequestSaveYourselfPhase2")

    def SmcGetProperties(self, prop_reply_proc):
        data = (self, prop_reply_proc)
        self.refs[id(data)] = data
        
        if not SmcGetProperties(self.conn, &prop_reply_proc_wrapper, <SmPointer> data):
            raise Exception("Error in SmcGetProperties")

    def __setitem__(self, name, value):
        self.SmcSetProperties({name:value})

    def __delitem__(self, name):
        self.SmcDeleteProperties([name])

    def properties(self):
        res = []
        
        @self.SmcGetProperties
        def properties(propdict):
            res.append(propdict)

        while not res:
            self.iceconn.IceProcessMessages()

        return res[0]

    def items(self):
        return self.properties().items()
    
    def __getitem__(self, name):
        return self.properties()[name]

def main():
    class MyConnection(PySmcConn):
        def signal_save_yourself(self, *arg):
            print("SAVE_YOURSELF", arg)
            self.save_yourself_done()
        def signal_die(self, *arg):
            print("DIE", arg)
        def signal_save_complete(self, *arg):
            print("SAVE_COMPLETE", arg)
        def signal_shutdown_cancelled(self, *arg):
            print("SHUTDOWN_CANCELLED", arg)

    c = MyConnection()
    try:
        while True:
            c.iceconn.IceProcessMessages()
    finally:
        c.SmcCloseConnection()
