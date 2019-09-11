from libc.stdlib cimport malloc, free
import os
from pysmlib.pysmlib cimport *

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
        self.close_connection([bytes(reasons_msg[idx]).decode("utf-8") for idx in range(count)])

cdef void set_properties_wrapper(SmsConn sms_conn, SmPointer manager_data, int num_props, SmProp **props):
    self = <Connection>manager_data;
    if self.set_properties:
        propdict = {}
        for idx in range(num_props):
            values = []
            for valueidx in range(props[idx].num_vals):
                values.append(PyBytes_FromStringAndSize(<char *> props[idx].vals[valueidx].value, props[idx].vals[valueidx].length).decode("utf-8"))
            propdict[bytes(props[idx].name).decode("utf-8")] = (bytes(props[idx].type).decode("utf-8"), values)
        self.set_properties(propdict)

cdef void delete_properties_wrapper(SmsConn sms_conn, SmPointer manager_data, int num_props, char **prop_names):
    self = <Connection>manager_data;
    if self.delete_properties:
        self.delete_properties([bytes(prop_names[idx]).decode("utf-8") for idx in range(num_props)])

cdef void get_properties_wrapper(SmsConn sms_conn, SmPointer manager_data):
    self = <Connection>manager_data;
    if self.get_properties:
        cdef int numProps
        cdef SmProp **props
        dict_to_smprops(self.get_properties(), &numProps, &props)
        SmsReturnProperties(sms_conn, numProps, props);
        free_smprops(numProps, props)
    
cdef new_client(
    SmsConn sms_conn,
    SmPointer manager_data,
    unsigned long *mask_ret,
    SmsCallbacks *callbacks_ret,
    char **failure_reason_ret):
    self = <Server>manager_data

    if not self.new_client:
        *failure_reason_ret = malloc(1024)
        strcpy(*failure_reason_ret, "Manager does not define new_client()")
        return 0

    try:
        conn = self.new_client(Connection(self, sms_conn))
    except Exception as e:
        *failure_reason_ret = malloc(1024)
        strcpy(*failure_reason_ret, str(e).encode("utf-8"))
        return 0
    
    *mask_ret = SmsRegisterClientProcMask | SmsInteractRequestProcMask | SmsInteractDoneProcMask | SmsSaveYourselfRequestProcMask | SmsSaveYourselfP2RequestProcMask | SmsSaveYourselfDoneProcMask | SmsCloseConnectionProcMask | SmsSetPropertiesProcMask | SmsDeletePropertiesProcMask | SmsGetPropertiesProcMask
    
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

cdef class Connection(object):
    cdef SmsConn conn
    cdef Server manager
    def __init__(self, Server manager, SmsConn conn):
        self.manager = manager
        self.conn = conn
        
cdef class Server(object):
    vendor = "pysmlib"
    release = "0.1"
    
    def __init__(self):
        if not SmsInitialize(
                self.vendor, self.release,
                SmsNewClientProc new_client_proc,
                SmPointer manager_data,
                IceHostBasedAuthProc host_based_auth_proc,
                int error_length,
                char *error_string_ret):
            raise Exception("SmsInitialize failed")
        
        
