from libc.stdlib cimport *
from libc.string cimport *
import os
from pysmlib.SMlib cimport *
from pysmlib.Python cimport *
from pysmlib.helpers cimport *

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

cdef Status register_client_wrapper(SmsConn sms_conn, SmPointer manager_data, char *previous_id):
    self = <Connection>manager_data;
    if self.register_client:
        return self.register_client(previous_id)
    return 0
    
cdef void interact_request_wrapper(SmsConn sms_conn, SmPointer manager_data, int dialog_type):
    self = <Connection>manager_data;
    if self.interact_request:
        self.interact_request(dialog_type)

cdef void interact_done_wrapper(SmsConn sms_conn, SmPointer manager_data, Bool cancel_shutdown):
    self = <Connection>manager_data;
    if self.interact_done:
        self.interact_done(cancel_shutdown)

cdef void save_yourself_request_wrapper(SmsConn sms_conn, SmPointer manager_data, int save_type, Bool shutdown, int interact_style, Bool fast, Bool glb):
    self = <Connection>manager_data;
    if self.save_yourself_request:
        self.save_yourself_request(save_type, shutdown, interact_style, fast, glb)

cdef void save_yourself_phase2_request_wrapper(SmsConn sms_conn, SmPointer manager_data):
    self = <Connection>manager_data;
    if self.save_yourself_phase2_request:
        self.save_yourself_phase2_request()

cdef void save_yourself_done_wrapper(SmsConn sms_conn, SmPointer manager_data, Bool success):
    self = <Connection>manager_data;
    if self.save_yourself_done:
        self.save_yourself_done(success)

cdef void close_connection_wrapper(SmsConn sms_conn, SmPointer manager_data, int count, char **reason_msgs):
    self = <Connection>manager_data;
    if self.close_connection:
        self.close_connection([bytes(reason_msgs[idx]).decode("utf-8") for idx in range(count)])

cdef void set_properties_wrapper(SmsConn sms_conn, SmPointer manager_data, int num_props, SmProp **props):
    self = <Connection>manager_data;
    if self.set_properties:
        self.set_properties(smprops_to_dict(num_props, props))

cdef void delete_properties_wrapper(SmsConn sms_conn, SmPointer manager_data, int num_props, char **prop_names):
    self = <Connection>manager_data;
    if self.delete_properties:
        self.delete_properties([bytes(prop_names[idx]).decode("utf-8") for idx in range(num_props)])

cdef void get_properties_wrapper(SmsConn sms_conn, SmPointer manager_data):
    cdef int numProps
    cdef SmProp **props
    self = <Connection>manager_data;
    if self.get_properties:
        dict_to_smprops(self.get_properties(), &numProps, &props)
        SmsReturnProperties(sms_conn, numProps, props);
        free_smprops(numProps, props)
    

cdef class SmsConnection(object):
    cdef SmsConn conn
    cdef set_conn(self, SmsConn conn):
        self.conn = conn

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

    cdef SmsConnection connw = SmsConnection()
    connw.set_conn(sms_conn)
    try:
        conn = self.Connection(self, connw)
    except Exception as e:
        failure_reason_ret[0] = <char *> malloc(1024)
        strcpy(failure_reason_ret[0], str(e).encode("utf-8"))
        return 0
    
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

cdef void ice_ping_reply_wrapper(IceConn ice_conn, IcePointer client_data):
    self = <Connection> client_data
    self.ice_ping_reply()

cdef class Connection(object):
    cdef SmsConn conn
    cdef IceConn iceconn

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

    def __init__(self, manager, SmsConnection conn, **kw):
        self.manager = manager
        self.connection = conn
        self.conn = conn.conn
        self.iceconn = SmsGetIceConnection(self.conn)
        for name, value in kw.items():
            setattr(self, name, value)

        
    def process(self):
        res = IceProcessMessages(self.iceconn, NULL, NULL)
        if res != 0:
            raise Exception(
                ["IceProcessMessagesSuccess",
                 "IceProcessMessagesIOError",
                 "IceProcessMessagesConnectionClosed"][res])

    def close(self):
        SmsCleanUp(self.conn)

    def save_yourself(self, save_type, shutdown, interact_style, fast):
        SmsSaveYourself(self.conn, save_type, shutdown, interact_style, fast)
       
    def save_yourself_phase2(self):
        SmsSaveYourselfPhase2(self.conn)

    def interact(self):
        SmsInteract(self.conn)

    def save_complete(self):
        SmsSaveComplete(self.conn)

    def die(self):
        SmsDie(self.conn)

    def shutdown_cancelled(self):
        SmsShutdownCancelled(self.conn)

    def ping(self):
        IcePing(self.iceconn, &ice_ping_reply_wrapper, <IcePointer> self)

    def version(self):
        return (SmsProtocolVersion(self.conn), SmsProtocolRevision(self.conn))

    def client_id(self):
        return bytes(SmsClientID(self.conn)).decode("utf-8")

    def client_hostname(self):
        return bytes(SmsClientHostName(self.conn)).decode("utf-8")


cdef Bool auth_failed_proc(char *hostname):
    return 0        
        
cdef class Server(object):
    vendor = b"pysmlib"
    release = b"0.1"

    Connection = Connection
    filenos = []
    
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

        cdef IceListenObj *listen_objs_ret
        cdef int count_ret
        
        if not IceListenForConnections(&count_ret, &listen_objs_ret, 1024, error_string_ret):
            raise Exception("SmsInitialize failed: %s" % error_string_ret)
        
        self.filenos = [IceGetListenConnectionNumber(listen_objs_ret[idx]) for idx in range(count_ret)]

