cdef extern from "Python.h":
    object PyBytes_FromStringAndSize(char *, Py_ssize_t)
    object PyString_FromStringAndSize(char *, Py_ssize_t)
    char *PyString_AsString(object)

cdef extern from "X11/ICE/ICElib.h":
    ctypedef void *IcePointer
    struct _IceConn:
        pass
    ctypedef _IceConn * IceConn
    struct _IceReplyWaitInfo:
        unsigned long       sequence_of_request
        int                 major_opcode_of_request
        int                 minor_opcode_of_request
        IcePointer          reply
    ctypedef _IceReplyWaitInfo IceReplyWaitInfo
    
    ctypedef int Bool

    int IceProcessMessages(IceConn ice_conn, IceReplyWaitInfo *reply_wait, Bool *reply_ready_ret)

    ctypedef Bool (*IceHostBasedAuthProc) (char *)

    ctypedef void (*IcePingReplyProc)(IceConn ice_conn, IcePointer client_data)
    void IcePing(IceConn ice_conn, IcePingReplyProc ping_reply_proc, IcePointer client_data)

    
cdef extern from "X11/SM/SMlib.h":
    ctypedef void * SmPointer
    struct _SmcConn:
        pass
    struct _SmsConn:
        pass
    ctypedef _SmcConn *SmcConn
    ctypedef _SmsConn *SmsConn

    ctypedef int Status

    struct _SmPropValue:
        int         length  # length (in bytes) of the value
        SmPointer   value   # the value
    ctypedef _SmPropValue SmPropValue

    struct _SmProp:
        char        *name          # name of property
        char        *type          # type of property
        int         num_vals       # number of values in property
        SmPropValue *vals          # the values
    ctypedef _SmProp SmProp
    
    ctypedef void (*SmcSaveYourselfProc) (
        SmcConn,            # smcConn
        SmPointer,          # clientData
        int,                # saveType
        Bool,               # shutdown
        int,                # interactStyle
        Bool                # fast
    )

    ctypedef void (*SmcSaveYourselfPhase2Proc) (
        SmcConn,            # smcConn
        SmPointer           # clientData
    )

    ctypedef void (*SmcInteractProc) (
        SmcConn,            # smcConn
        SmPointer           # clientData
    )

    ctypedef void (*SmcDieProc) (
        SmcConn,            # smcConn
        SmPointer           # clientData
    )

    ctypedef void (*SmcShutdownCancelledProc) (
        SmcConn,            # smcConn
        SmPointer           # clientData
    )

    ctypedef void (*SmcSaveCompleteProc) (
        SmcConn,            # smcConn
        SmPointer           # clientData
    )

    ctypedef void (*SmcPropReplyProc) (
        SmcConn,            # smcConn
        SmPointer,          # clientData
        int,                # numProps
        SmProp **           # props
    )
    
    struct save_yourself:
        SmcSaveYourselfProc      callback
        SmPointer                client_data

    struct die:
        SmcDieProc               callback
        SmPointer                client_data

    struct save_complete:
        SmcSaveCompleteProc      callback
        SmPointer                client_data

    struct shutdown_cancelled:
        SmcShutdownCancelledProc callback
        SmPointer                client_data

    struct _SmcCallbacks:
        save_yourself save_yourself
        die die
        save_complete save_complete
        shutdown_cancelled shutdown_cancelled

    ctypedef _SmcCallbacks SmcCallbacks

    ctypedef void (*SmcErrorHandler)(
        SmcConn smc_conn, Bool swap,
        int offending_minor_opcode,
        unsigned long offending_sequence_num,
        int error_class,
        int severity,
        IcePointer values);

    SmcConn SmcOpenConnection(char *network_ids_list, SmPointer context, int xsmp_major_rev, int xsmp_minor_rev, unsigned long mask, SmcCallbacks *callbacks, char *previous_id, char **client_id_ret, int error_length, char *error_string_ret)
    int SmcCloseConnection(SmcConn smc_conn, int count, char **reason_msgs)
    void SmcSetProperties(SmcConn smc_conn, int num_props, SmProp **props)
    void SmcDeleteProperties(SmcConn smc_conn, int num_props, char **prop_names)
    Status SmcGetProperties(SmcConn smc_conn, SmcPropReplyProc prop_reply_proc, SmPointer client_data)
    void SmcSaveYourselfDone(SmcConn smc_conn, Bool success);
    Status SmcRequestSaveYourselfPhase2(SmcConn smc_conn, SmcSaveYourselfPhase2Proc save_yourself_phase2_proc, SmPointer client_data);

    int SmcProtocolVersion(SmcConn smc_conn);
    int SmcProtocolRevision(SmcConn smc_conn);
    char *SmcVendor(SmcConn smc_conn);
    char *SmcRelease(SmcConn smc_conn);
    char *SmcClientID(SmcConn smc_conn);
    IceConn SmcGetIceConnection(SmcConn smc_conn);
    
    SmcErrorHandler SmcSetErrorHandler(SmcErrorHandler handler);
    ctypedef Status (*SmsRegisterClientProc)(SmsConn sms_conn, SmPointer manager_data, char *previous_id);
    ctypedef void (*SmsInteractRequestProc)(SmsConn sms_conn, SmPointer manager_data, int dialog_type);
    ctypedef void (*SmsInteractDoneProc)(SmsConn sms_conn, SmPointer manager_data, Bool cancel_shutdown);
    ctypedef void (*SmsSaveYourselfRequestProc)(SmsConn sms_conn, SmPointer manager_data, int save_type, Bool shutdown, int interact_style, Bool fast, Bool glb);
    ctypedef void (*SmsSaveYourselfPhase2RequestProc)(SmsConn sms_conn, SmPointer manager_data);
    ctypedef void (*SmsSaveYourselfDoneProc)(SmsConn sms_conn, SmPointer manager_data, Bool success);
    ctypedef void (*SmsCloseConnectionProc)(SmsConn sms_conn, SmPointer manager_data, int count, char **reason_msgs);
    ctypedef void (*SmsSetPropertiesProc)(SmsConn sms_conn, SmPointer manager_data, int num_props, SmProp **props);
    ctypedef void (*SmsDeletePropertiesProc)(SmsConn sms_conn, SmPointer manager_data, int num_props, char **prop_names);
    ctypedef void (*SmsGetPropertiesProc)(SmsConn sms_conn, SmPointer manager_data);

    struct sms_register_client:
        SmsRegisterClientProc callback
        SmPointer manager_data

    struct sms_interact_request:
        SmsInteractRequestProc callback
        SmPointer manager_data

    struct sms_interact_done:
        SmsInteractDoneProc callback
        SmPointer manager_data

    struct sms_save_yourself_request:
        SmsSaveYourselfRequestProc callback
        SmPointer manager_data

    struct sms_save_yourself_phase2_request:
        SmsSaveYourselfPhase2RequestProc callback
        SmPointer manager_data

    struct sms_save_yourself_done:
        SmsSaveYourselfDoneProc callback
        SmPointer manager_data

    struct sms_close_connection:
        SmsCloseConnectionProc callback;
        SmPointer manager_data;

    struct sms_set_properties:
        SmsSetPropertiesProc callback
        SmPointer manager_data

    struct sms_delete_properties:
        SmsDeletePropertiesProc callback
        SmPointer manager_data

    struct sms_get_properties:
        SmsGetPropertiesProc callback
        SmPointer manager_data

    struct SmsCallbacks:
        sms_register_client register_client
        sms_interact_request interact_request
        sms_interact_done interact_done
        sms_save_yourself_request save_yourself_request
        sms_save_yourself_phase2_request save_yourself_phase2_request
        sms_save_yourself_done save_yourself_done
        sms_close_connection close_connection
        sms_set_properties set_properties
        sms_delete_properties delete_properties
        sms_get_properties get_properties
     
    ctypedef Status (*SmsNewClientProc)(
        SmsConn sms_conn,
        SmPointer manager_data,
        unsigned long *mask_ret,
        SmsCallbacks *callbacks_ret,
        char **failure_reason_ret);

    ctypedef void (*SmsErrorHandler)(
        SmsConn sms_conn,
        Bool swap,
        int offending_minor_opcode,
        unsigned long offending_sequence_num,
        int error_class,
        int severity,
        IcePointer values);

    Status SmsInitialize(
        char *vendor, char *release,
        SmsNewClientProc new_client_proc,
        SmPointer manager_data,
        IceHostBasedAuthProc host_based_auth_proc,
        int error_length,
        char *error_string_ret);

    Status SmsRegisterClientReply(SmsConn sms_conn, char *client_id);
    char *SmsGenerateClientID(SmsConn sms_conn);
    void SmsSaveYourself(SmsConn sms_conn, int save_type, Bool shutdown, int interact_style, Bool fast);
    void SmsSaveYourselfPhase2(SmsConn sms_conn);
    void SmsInteract(SmsConn sms_conn);
    void SmsSaveComplete(SmsConn sms_conn);
    void SmsDie(SmsConn sms_conn);
    void SmsShutdownCancelled(SmsConn sms_conn);
    void SmsReturnProperties(SmsConn sms_conn, int num_props, SmProp **props);
    void SmsCleanUp(SmsConn sms_conn);
    int SmsProtocolVersion(SmsConn sms_conn);
    int SmsProtocolRevision(SmsConn sms_conn);
    char *SmsClientID(SmsConn sms_conn);
    char *SmsClientHostName(SmsConn sms_conn);
    IceConn SmsGetIceConnection(SmsConn sms_conn);
    SmsErrorHandler SmsSetErrorHandler(SmsErrorHandler handler);

