from libc.stdlib cimport *
from libc.string cimport *
import os
from pysmlib.SMlib cimport *
from pysmlib.Python cimport *
from pysmlib.helpers cimport *
from pysmlib.ice cimport *
import sys
from libc.stdio cimport printf, stdout, fflush
import traceback

SmsRegisterClientProcMask               = 1L << 0
SmsInteractRequestProcMask              = 1L << 1
SmsInteractDoneProcMask                 = 1L << 2
SmsSaveYourselfRequestProcMask          = 1L << 3
SmsSaveYourselfP2RequestProcMask        = 1L << 4
SmsSaveYourselfDoneProcMask             = 1L << 5
SmsCloseConnectionProcMask              = 1L << 6
SmsSetPropertiesProcMask                = 1L << 7
SmsDeletePropertiesProcMask             = 1L << 8
SmsGetPropertiesProcMask                = 1L << 9

SmInteractStyleNone   = 0
SmInteractStyleErrors = 1
SmInteractStyleAny    = 2

SmSaveGlobal = 0
SmSaveLocal  = 1
SmSaveBoth   = 2


cdef Status register_client_wrapper(SmsConn sms_conn, SmPointer manager_data, char *previous_id):
    self = <PySmsConn>manager_data;
    if self.register_client:
        return self.register_client(previous_id if previous_id != NULL else None)
    return 0
    
cdef void interact_request_wrapper(SmsConn sms_conn, SmPointer manager_data, int dialog_type):
    sys.stdout.flush()
    self = <PySmsConn>manager_data;
    if self.interact_request:
        self.interact_request(dialog_type)

cdef void interact_done_wrapper(SmsConn sms_conn, SmPointer manager_data, Bool cancel_shutdown):
    self = <PySmsConn>manager_data;
    sys.stdout.flush()
    if self.interact_done:
        self.interact_done(cancel_shutdown)

cdef void save_yourself_request_wrapper(SmsConn sms_conn, SmPointer manager_data, int save_type, Bool shutdown, int interact_style, Bool fast, Bool glb):
    self = <PySmsConn>manager_data;
    if self.save_yourself_request:
        self.save_yourself_request(save_type, shutdown, interact_style, fast, glb)

cdef void save_yourself_phase2_request_wrapper(SmsConn sms_conn, SmPointer manager_data):
    self = <PySmsConn>manager_data;
    if self.save_yourself_phase2_request:
        self.save_yourself_phase2_request()

cdef void save_yourself_done_wrapper(SmsConn sms_conn, SmPointer manager_data, Bool success):
    self = <PySmsConn>manager_data;
    if self.save_yourself_done:
        self.save_yourself_done(success)

cdef void close_connection_wrapper(SmsConn sms_conn, SmPointer manager_data, int count, char **reason_msgs):
    self = <PySmsConn>manager_data;
    if self.close_connection:
        self.close_connection([bytes(reason_msgs[idx]).decode("utf-8") for idx in range(count)])

cdef void set_properties_wrapper(SmsConn sms_conn, SmPointer manager_data, int num_props, SmProp **props):
    self = <PySmsConn>manager_data;
    if self.set_properties:
        self.set_properties(smprops_to_dict(num_props, props))

cdef void delete_properties_wrapper(SmsConn sms_conn, SmPointer manager_data, int num_props, char **prop_names):
    self = <PySmsConn>manager_data;
    if self.delete_properties:
        self.delete_properties([bytes(prop_names[idx]).decode("utf-8") for idx in range(num_props)])

cdef void get_properties_wrapper(SmsConn sms_conn, SmPointer manager_data):
    cdef int numProps
    cdef SmProp **props
    self = <PySmsConn>manager_data;
    if self.get_properties:
        dict_to_smprops(self.get_properties(), &numProps, &props)
        SmsReturnProperties(sms_conn, numProps, props);
        free_smprops(numProps, props)
    
cdef Bool new_client(
    SmsConn sms_conn,
    SmPointer manager_data,
    unsigned long *mask_ret,
    SmsCallbacks *callbacks_ret,
    char **failure_reason_ret):
    self = <Server>manager_data

    if not self.Connection:
        failure_reason_ret[0] = <char *> malloc(1024)
        strcpy(failure_reason_ret[0], "Manager does not define new_client()")
        return 0

    print("NEW CONN", self)
    sys.stdout.flush()
    try:
        # Split up object construction so that PySmsConn.conn is available in __init__()
        conn = self.Connection.__new__(self.Connection, self)
        (<PySmsConn?> conn).init(sms_conn)
        conn.__init__(self)
    except Exception as e:
        print("NEW CONN ERR", e)
        traceback.print_exc()
        failure_reason_ret[0] = <char *> malloc(1024)
        strcpy(failure_reason_ret[0], str(e).encode("utf-8"))
        return 0
    self.connections[id(conn)] = conn

    print("NEW CONN DONE", self, id(conn), self.connections, <long>sms_conn, <long><SmPointer>conn)
    sys.stdout.flush()


    mask_ret[0] = SmsRegisterClientProcMask | SmsInteractRequestProcMask | SmsInteractDoneProcMask | SmsSaveYourselfRequestProcMask | SmsSaveYourselfP2RequestProcMask | SmsSaveYourselfDoneProcMask | SmsCloseConnectionProcMask | SmsSetPropertiesProcMask | SmsDeletePropertiesProcMask | SmsGetPropertiesProcMask
    
    callbacks_ret.register_client.callback = register_client_wrapper
    callbacks_ret.interact_request.callback = interact_request_wrapper
    callbacks_ret.interact_done.callback = interact_done_wrapper
    callbacks_ret.save_yourself_request.callback = save_yourself_request_wrapper
    callbacks_ret.save_yourself_phase2_request.callback = save_yourself_phase2_request_wrapper
    callbacks_ret.save_yourself_done.callback = save_yourself_done_wrapper
    callbacks_ret.close_connection.callback = close_connection_wrapper
    callbacks_ret.set_properties.callback = set_properties_wrapper
    callbacks_ret.delete_properties.callback = delete_properties_wrapper
    callbacks_ret.get_properties.callback = get_properties_wrapper

    callbacks_ret.register_client.manager_data = <SmPointer>conn
    callbacks_ret.interact_request.manager_data = <SmPointer>conn
    callbacks_ret.interact_done.manager_data = <SmPointer>conn
    callbacks_ret.save_yourself_request.manager_data = <SmPointer>conn
    callbacks_ret.save_yourself_phase2_request.manager_data = <SmPointer>conn
    callbacks_ret.save_yourself_done.manager_data = <SmPointer>conn
    callbacks_ret.close_connection.manager_data = <SmPointer>conn
    callbacks_ret.set_properties.manager_data = <SmPointer>conn
    callbacks_ret.delete_properties.manager_data = <SmPointer>conn
    callbacks_ret.get_properties.manager_data = <SmPointer>conn
 
    return 1



cdef class PySmsConn(object):
    cdef SmsConn _conn

    manager = None
    connection = None
    register_client = None
    interact_request = None
    interact_done = None
    save_yourself_request = None
    save_yourself_phase2_request = None
    save_yourself_done = None
    close_connection = None
    set_properties = None
    delete_properties = None
    get_properties = None
    ice_ping_reply = None

    def __init__(self, manager, **kw):
        self.manager = manager
        for name, value in kw.items():
            setattr(self, name, value)
    cdef init(self, SmsConn conn):
        self._conn = conn
        return self
    cdef SmsConn conn(self):
        if self._conn == NULL:
            raise AttributeError("conn not available from __new__(), or after SmsCleanUp()")
        return self._conn
    def SmsGetIceConnection(self):
        return PyIceConn().init(SmsGetIceConnection(self.conn()))
    def SmsRegisterClientReply(self, client_id):
        return SmsRegisterClientReply(self.conn(), client_id)
    def SmsCleanUp(self):
        SmsCleanUp(self.conn())
        self._conn = NULL
        del self.manager.connections[id(self)]
    def SmsSaveYourself(self, save_type, shutdown, interact_style, fast):
        SmsSaveYourself(self.conn(), save_type, shutdown, interact_style, fast)
    def SmsSaveYourselfPhase2(self):
        SmsSaveYourselfPhase2(self.conn())
    def SmsInteract(self):
        SmsInteract(self.conn())
    def SmsSaveComplete(self):
        SmsSaveComplete(self.conn())
    def SmsDie(self):
        SmsDie(self.conn())
    def SmsShutdownCancelled(self):
        SmsShutdownCancelled(self.conn())
    def SmsProtocolVersion(self):
        return SmsProtocolVersion(self.conn())
    def SmsProtocolRevision(self):
        return SmsProtocolRevision(self.conn())
    def SmsClientID(self):
        return bytes(SmsClientID(self.conn())).decode("utf-8")
    def SmsClientHostName(self):
        return bytes(SmsClientHostName(self.conn())).decode("utf-8")


cdef Bool auth_failed_proc(char *hostname):
    return 1

cdef class Server(object):
    vendor = b"pysmlib"
    release = b"0.1"

    Connection = PySmsConn
    filenos = []
    connections = {}
    
    def __init__(self):
        cdef char error_string_ret[1024]
        
        if not SmsInitialize(
                self.vendor, self.release,
                &new_client,
                <SmPointer> self,
                &auth_failed_proc,
                1024,
                error_string_ret):
            raise Exception("SmsInitialize failed: %s" % error_string_ret)

    def IceListenForConnections(self):
        cdef IceListenObj *listen_objs_ret = NULL
        cdef int count_ret
        cdef char error_string_ret[1024]

        if not IceListenForConnections(&count_ret, &listen_objs_ret, 1024, error_string_ret):
            raise Exception("SmsInitialize failed: %s" % error_string_ret)

        res = [PyIceListenObj().init(listen_objs_ret[idx]) for idx in range(count_ret)]
        free(listen_objs_ret)
        return res

