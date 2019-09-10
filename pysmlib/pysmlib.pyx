from libc.stdlib cimport malloc, free
import os
cimport pysmlib

SmcSaveYourselfProcMask      = (1L << 0)
SmcDieProcMask               = (1L << 1)
SmcSaveCompleteProcMask      = (1L << 2)
SmcShutdownCancelledProcMask = (1L << 3)

cdef void save_yourself_wrapper(SmcConn smc_conn, SmPointer client_data, int save_type, Bool shutdown, int interact_style, Bool fast):
    self = <Connection> client_data
    if self.save_yourself:
        self.save_yourself(save_type, shutdown, interact_style, fast)

cdef void die_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <Connection> client_data
    if self.die:
        self.die()
     
cdef void shutdown_cancelled_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <Connection> client_data
    if self.shutdown_cancelled:
        self.shutdown_cancelled()
     
cdef void save_complete_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <Connection> client_data
    if self.save_complete:
        self.save_complete()

cdef void save_yourself_phase2_wrapper(SmcConn smcConn, SmPointer client_data):
    self = <Connection> client_data
    if self.save_yourself_phase2:
        self.save_yourself_phase2()
        
cdef void error_handler(
    SmcConn smc_conn,
    Bool swap,
    int offending_minor_opcode,
    unsigned long offending_sequence_num,
    int error_class,
    int severity,
    IcePointer values):
    pass

        
cdef void prop_reply_proc_wrapper(SmcConn smcConn, SmPointer client_data, int numProps, SmProp **props):
    self = <Connection> client_data

    propdict = {}
    for idx in range(numProps):
        values = []
        for valueidx in range(props[idx].num_vals):
            values.append(PyBytes_FromStringAndSize(<char *> props[idx].vals[valueidx].value, props[idx].vals[valueidx].length).decode("utf-8"))
        propdict[bytes(props[idx].name).decode("utf-8")] = (bytes(props[idx].type).decode("utf-8"), values)

    self.prop_reply_proc(propdict)

cdef class Connection(object):
    cdef SmcConn conn
    cdef IceConn iceconn
    cdef public str network_ids_list

    cdef public char *client_id
    cdef SmcCallbacks callbacks

    cdef object save_yourself
    cdef object die
    cdef object save_complete
    cdef object shutdown_cancelled
    cdef object save_yourself_phase2

    cdef object propdict
    
    def __init__(self,
                 str network_ids_list = None,
                 object context = None,
                 int xsmp_major_rev = 1,
                 int xsmp_minor_rev = 0,
                 bytes previous_id = None,

                 save_yourself = None,
                 die = None,
                 save_complete = None,
                 shutdown_cancelled = None,
                 save_yourself_phase2 = None):

        self.network_ids_list = network_ids_list or os.environ["SESSION_MANAGER"]
        
        cdef char error_string_ret[1024]

        cdef unsigned long mask = 0
        

        if save_yourself: self.save_yourself = save_yourself
        if die: self.die = die
        if save_complete: self.save_complete = save_complete
        if shutdown_cancelled: self.shutdown_cancelled = shutdown_cancelled
        if save_yourself_phase2: self.save_yourself_phase2 = save_yourself_phase2
        
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
        self.iceconn = SmcGetIceConnection(self.conn)
 
        if self.conn == NULL:
            raise Exception(error_string_ret)

    def close(self, reasons = []):
        cdef char **reasons_arr = <char **> malloc(sizeof(char *) * len(reasons))
        reasons = [reason.encode("utf-8") for reason in reasons]
        
        for idx in range(0, len(reasons)):
            reasons_arr[idx] = reasons[idx]
        res = SmcCloseConnection(self.conn, len(reasons), reasons_arr)
        free(reasons_arr)
        return ["SmcClosedNow",
                "SmcClosedASAP",
                "SmcConnectionInUse"][res]

    @property
    def protocol_version(self):
        return SmcProtocolVersion(self.conn), SmcProtocolRevision(self.conn)

    @property
    def vendor(self):
        return bytes(SmcVendor(self.conn)).decode("utf-8")

    @property
    def vendor(self):
        return bytes(SmcRelease(self.conn)).decode("utf-8")
    
    def __setitem__(self, name, value):
        cdef SmProp prop
        cdef SmProp *props = &prop

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
        for idx in range(0, len(value)):
            prop.vals[idx].length = len(value[idx])
            prop.vals[idx].value = <char*> value[idx]
        SmcSetProperties(self.conn, 1, &props)
        free(prop.vals)
        
    def __delitem__(self, name):
        cdef char *namestr = name
        SmcDeleteProperties(self.conn, 1, &namestr)

    def save_yourself_done(self, success):
        SmcSaveYourselfDone(self.conn, success);

    def request_save_yourself_phase2(self):
        if not SmcRequestSaveYourselfPhase2(self.conn, &save_yourself_phase2_wrapper, <SmPointer> self):
            raise Exception("Error calling SmcRequestSaveYourselfPhase2")
        
    def prop_reply_proc(self, propdict):
        self.propdict = propdict

    def process(self):
        res = IceProcessMessages(self.iceconn, NULL, NULL)
        if res != 0:
            raise Exception(
                ["IceProcessMessagesSuccess",
                 "IceProcessMessagesIOError",
                 "IceProcessMessagesConnectionClosed"][res])
        
    def properties(self):
        self.propdict = None
        
        if not SmcGetProperties(self.conn, &prop_reply_proc_wrapper, <SmPointer> self):
            raise Exception("Error in SmcGetProperties")

        while self.propdict is None:
            self.process()

        return self.propdict

    def items(self):
        return self.properties().items()
    
    def __getitem__(self, name):
        return self.properties()[name]

def main():
    class MyConnection(Connection):
        def signal_save_yourself(self, *arg):
            print("SAVE_YOURSELF", arg)
            self.save_yourself_done()
        def signal_die(self, *arg):
            print("DIE", arg)
        def signal_save_complete(self, *arg):
            print("SAVE_COMPLETE", arg)
        def signal_shutdown_cancelled(self, *arg):
            print("SHUTDOWN_CANCELLED", arg)

    c = Connection()
    try:
        while True:
            c.process()
    finally:
        c.close()
