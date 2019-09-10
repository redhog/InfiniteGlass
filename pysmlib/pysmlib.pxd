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
    
cdef extern from "X11/SM/SMlib.h":
    ctypedef void * SmPointer
    struct _SmcConn:
        pass
    ctypedef _SmcConn * SmcConn

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

    ctypedef void (*SmcSaveYourselfPhase2Proc) (
        SmcConn,
        SmPointer
    );

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
