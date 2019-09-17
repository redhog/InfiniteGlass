cdef extern from "/usr/include/X11/ICE/ICElib.h":
    pass

cdef extern from "/usr/include/X11/ICE/ICEutil.h":
    struct _IceAuthDataEntry:
        char            *protocol_name
        char            *network_id
        char            *auth_name
        unsigned short  auth_data_length
        char            *auth_data
    ctypedef _IceAuthDataEntry IceAuthDataEntry

    void IceSetPaAuthData(int numEntries, IceAuthDataEntry *entries)
    char *IceGenerateMagicCookie(int length);

cdef extern from "/usr/include/X11/ICE/ICElib.h":
    struct _IceListenObj:
        pass
    struct _IceConn:
        pass
    struct _SmcConn:
        pass
    struct _SmsConn:
        pass
    ctypedef int Bool
    ctypedef int Status

# All below generated using autopxd
    
    ctypedef void* IcePointer

    ctypedef enum IcePoAuthStatus:
        IcePoAuthHaveReply
        IcePoAuthRejected
        IcePoAuthFailed
        IcePoAuthDoneCleanup

    ctypedef enum IcePaAuthStatus:
        IcePaAuthContinue
        IcePaAuthAccepted
        IcePaAuthRejected
        IcePaAuthFailed

    ctypedef enum IceConnectStatus:
        IceConnectPending
        IceConnectAccepted
        IceConnectRejected
        IceConnectIOError

    ctypedef enum IceProtocolSetupStatus:
        IceProtocolSetupSuccess
        IceProtocolSetupFailure
        IceProtocolSetupIOError
        IceProtocolAlreadyActive

    ctypedef enum IceAcceptStatus:
        IceAcceptSuccess
        IceAcceptFailure
        IceAcceptBadMalloc

    ctypedef enum IceCloseStatus:
        IceClosedNow
        IceClosedASAP
        IceConnectionInUse
        IceStartedShutdownNegotiation

    ctypedef enum IceProcessMessagesStatus:
        IceProcessMessagesSuccess
        IceProcessMessagesIOError
        IceProcessMessagesConnectionClosed

    ctypedef struct IceReplyWaitInfo:
        unsigned long sequence_of_request
        int major_opcode_of_request
        int minor_opcode_of_request
        IcePointer reply

    ctypedef _IceConn* IceConn

    ctypedef _IceListenObj* IceListenObj

    ctypedef void (*IceWatchProc)(IceConn, IcePointer, int, IcePointer*)

    ctypedef void (*IcePoProcessMsgProc)(IceConn, IcePointer, int, unsigned long, int, IceReplyWaitInfo*, int*)

    ctypedef void (*IcePaProcessMsgProc)(IceConn, IcePointer, int, unsigned long, int)

    ctypedef struct IcePoVersionRec:
        int major_version
        int minor_version
        IcePoProcessMsgProc process_msg_proc

    ctypedef struct IcePaVersionRec:
        int major_version
        int minor_version
        IcePaProcessMsgProc process_msg_proc

    ctypedef IcePoAuthStatus (*IcePoAuthProc)(IceConn, IcePointer*, int, int, int, IcePointer, int*, IcePointer*, char**)

    ctypedef IcePaAuthStatus (*IcePaAuthProc)(IceConn, IcePointer*, int, int, IcePointer, int*, IcePointer*, char**)

    ctypedef int (*IceHostBasedAuthProc)(char*)

    ctypedef int (*IceProtocolSetupProc)(IceConn, int, int, char*, char*, IcePointer*, char**)

    ctypedef void (*IceProtocolActivateProc)(IceConn, IcePointer)

    ctypedef void (*IceIOErrorProc)(IceConn)

    ctypedef void (*IcePingReplyProc)(IceConn, IcePointer)

    ctypedef void (*IceErrorHandler)(IceConn, int, int, unsigned long, int, int, IcePointer)

    ctypedef void (*IceIOErrorHandler)(IceConn)

    int IceRegisterForProtocolSetup(char*, char*, char*, int, IcePoVersionRec*, int, char**, IcePoAuthProc*, IceIOErrorProc)

    int IceRegisterForProtocolReply(char*, char*, char*, int, IcePaVersionRec*, int, char**, IcePaAuthProc*, IceHostBasedAuthProc, IceProtocolSetupProc, IceProtocolActivateProc, IceIOErrorProc)

    IceConn IceOpenConnection(char*, IcePointer, int, int, int, char*)

    IcePointer IceGetConnectionContext(IceConn)

    int IceListenForConnections(int*, IceListenObj**, int, char*)

    int IceListenForWellKnownConnections(char*, int*, IceListenObj**, int, char*)

    int IceGetListenConnectionNumber(IceListenObj)

    char* IceGetListenConnectionString(IceListenObj)

    char* IceComposeNetworkIdList(int, IceListenObj*)

    void IceFreeListenObjs(int, IceListenObj*)

    void IceSetHostBasedAuthProc(IceListenObj, IceHostBasedAuthProc)

    IceConn IceAcceptConnection(IceListenObj, IceAcceptStatus*)

    void IceSetShutdownNegotiation(IceConn, int)

    int IceCheckShutdownNegotiation(IceConn)

    IceCloseStatus IceCloseConnection(IceConn)

    int IceAddConnectionWatch(IceWatchProc, IcePointer)

    void IceRemoveConnectionWatch(IceWatchProc, IcePointer)

    IceProtocolSetupStatus IceProtocolSetup(IceConn, int, IcePointer, int, int*, int*, char**, char**, int, char*)

    int IceProtocolShutdown(IceConn, int)

    IceProcessMessagesStatus IceProcessMessages(IceConn, IceReplyWaitInfo*, int*)

    int IcePing(IceConn, IcePingReplyProc, IcePointer)

    char* IceAllocScratch(IceConn, unsigned long)

    int IceFlush(IceConn)

    int IceGetOutBufSize(IceConn)

    int IceGetInBufSize(IceConn)

    IceConnectStatus IceConnectionStatus(IceConn)

    char* IceVendor(IceConn)

    char* IceRelease(IceConn)

    int IceProtocolVersion(IceConn)

    int IceProtocolRevision(IceConn)

    int IceConnectionNumber(IceConn)

    char* IceConnectionString(IceConn)

    unsigned long IceLastSentSequenceNumber(IceConn)

    unsigned long IceLastReceivedSequenceNumber(IceConn)

    int IceSwapping(IceConn)

    IceErrorHandler IceSetErrorHandler(IceErrorHandler)

    IceIOErrorHandler IceSetIOErrorHandler(IceIOErrorHandler)

    char* IceGetPeerName(IceConn)

    int IceInitThreads()

    void IceAppLockConn(IceConn)

    void IceAppUnlockConn(IceConn)


cdef extern from "/usr/include/X11/SM/SMlib.h":
    ctypedef IcePointer SmPointer

    ctypedef _SmcConn* SmcConn

    ctypedef _SmsConn* SmsConn

    ctypedef struct SmPropValue:
        int length
        SmPointer value

    ctypedef struct SmProp:
        char* name
        char* type
        int num_vals
        SmPropValue* vals

    ctypedef enum SmcCloseStatus:
        SmcClosedNow
        SmcClosedASAP
        SmcConnectionInUse

    ctypedef void (*SmcSaveYourselfProc)(SmcConn, SmPointer, int, int, int, int)

    ctypedef void (*SmcSaveYourselfPhase2Proc)(SmcConn, SmPointer)

    ctypedef void (*SmcInteractProc)(SmcConn, SmPointer)

    ctypedef void (*SmcDieProc)(SmcConn, SmPointer)

    ctypedef void (*SmcShutdownCancelledProc)(SmcConn, SmPointer)

    ctypedef void (*SmcSaveCompleteProc)(SmcConn, SmPointer)

    ctypedef void (*SmcPropReplyProc)(SmcConn, SmPointer, int, SmProp**)

    cdef struct _SmcCallbacks_SmcCallbacks_save_yourself_s:
        SmcSaveYourselfProc callback
        SmPointer client_data

    cdef struct _SmcCallbacks_SmcCallbacks_die_s:
        SmcDieProc callback
        SmPointer client_data

    cdef struct _SmcCallbacks_SmcCallbacks_save_complete_s:
        SmcSaveCompleteProc callback
        SmPointer client_data

    cdef struct _SmcCallbacks_SmcCallbacks_shutdown_cancelled_s:
        SmcShutdownCancelledProc callback
        SmPointer client_data

    ctypedef struct SmcCallbacks:
        _SmcCallbacks_SmcCallbacks_save_yourself_s save_yourself
        _SmcCallbacks_SmcCallbacks_die_s die
        _SmcCallbacks_SmcCallbacks_save_complete_s save_complete
        _SmcCallbacks_SmcCallbacks_shutdown_cancelled_s shutdown_cancelled

    ctypedef int (*SmsRegisterClientProc)(SmsConn, SmPointer, char*)

    ctypedef void (*SmsInteractRequestProc)(SmsConn, SmPointer, int)

    ctypedef void (*SmsInteractDoneProc)(SmsConn, SmPointer, int)

    ctypedef void (*SmsSaveYourselfRequestProc)(SmsConn, SmPointer, int, int, int, int, int)

    ctypedef void (*SmsSaveYourselfPhase2RequestProc)(SmsConn, SmPointer)

    ctypedef void (*SmsSaveYourselfDoneProc)(SmsConn, SmPointer, int)

    ctypedef void (*SmsCloseConnectionProc)(SmsConn, SmPointer, int, char**)

    ctypedef void (*SmsSetPropertiesProc)(SmsConn, SmPointer, int, SmProp**)

    ctypedef void (*SmsDeletePropertiesProc)(SmsConn, SmPointer, int, char**)

    ctypedef void (*SmsGetPropertiesProc)(SmsConn, SmPointer)

    cdef struct _SmsCallbacks_SmsCallbacks_register_client_s:
        SmsRegisterClientProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_interact_request_s:
        SmsInteractRequestProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_interact_done_s:
        SmsInteractDoneProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_save_yourself_request_s:
        SmsSaveYourselfRequestProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_save_yourself_phase2_request_s:
        SmsSaveYourselfPhase2RequestProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_save_yourself_done_s:
        SmsSaveYourselfDoneProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_close_connection_s:
        SmsCloseConnectionProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_set_properties_s:
        SmsSetPropertiesProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_delete_properties_s:
        SmsDeletePropertiesProc callback
        SmPointer manager_data

    cdef struct _SmsCallbacks_SmsCallbacks_get_properties_s:
        SmsGetPropertiesProc callback
        SmPointer manager_data

    ctypedef struct SmsCallbacks:
        _SmsCallbacks_SmsCallbacks_register_client_s register_client
        _SmsCallbacks_SmsCallbacks_interact_request_s interact_request
        _SmsCallbacks_SmsCallbacks_interact_done_s interact_done
        _SmsCallbacks_SmsCallbacks_save_yourself_request_s save_yourself_request
        _SmsCallbacks_SmsCallbacks_save_yourself_phase2_request_s save_yourself_phase2_request
        _SmsCallbacks_SmsCallbacks_save_yourself_done_s save_yourself_done
        _SmsCallbacks_SmsCallbacks_close_connection_s close_connection
        _SmsCallbacks_SmsCallbacks_set_properties_s set_properties
        _SmsCallbacks_SmsCallbacks_delete_properties_s delete_properties
        _SmsCallbacks_SmsCallbacks_get_properties_s get_properties

    ctypedef int (*SmsNewClientProc)(SmsConn, SmPointer, unsigned long*, SmsCallbacks*, char**)

    ctypedef void (*SmcErrorHandler)(SmcConn, int, int, unsigned long, int, int, SmPointer)

    ctypedef void (*SmsErrorHandler)(SmsConn, int, int, unsigned long, int, int, SmPointer)

    SmcConn SmcOpenConnection(char*, SmPointer, int, int, unsigned long, SmcCallbacks*, char*, char**, int, char*)

    SmcCloseStatus SmcCloseConnection(SmcConn, int, char**)

    void SmcModifyCallbacks(SmcConn, unsigned long, SmcCallbacks*)

    void SmcSetProperties(SmcConn, int, SmProp**)

    void SmcDeleteProperties(SmcConn, int, char**)

    int SmcGetProperties(SmcConn, SmcPropReplyProc, SmPointer)

    int SmcInteractRequest(SmcConn, int, SmcInteractProc, SmPointer)

    void SmcInteractDone(SmcConn, int)

    void SmcRequestSaveYourself(SmcConn, int, int, int, int, int)

    int SmcRequestSaveYourselfPhase2(SmcConn, SmcSaveYourselfPhase2Proc, SmPointer)

    void SmcSaveYourselfDone(SmcConn, int)

    int SmcProtocolVersion(SmcConn)

    int SmcProtocolRevision(SmcConn)

    char* SmcVendor(SmcConn)

    char* SmcRelease(SmcConn)

    char* SmcClientID(SmcConn)

    IceConn SmcGetIceConnection(SmcConn)

    int SmsInitialize(char*, char*, SmsNewClientProc, SmPointer, IceHostBasedAuthProc, int, char*)

    char* SmsClientHostName(SmsConn)

    char* SmsGenerateClientID(SmsConn)

    int SmsRegisterClientReply(SmsConn, char*)

    void SmsSaveYourself(SmsConn, int, int, int, int)

    void SmsSaveYourselfPhase2(SmsConn)

    void SmsInteract(SmsConn)

    void SmsDie(SmsConn)

    void SmsSaveComplete(SmsConn)

    void SmsShutdownCancelled(SmsConn)

    void SmsReturnProperties(SmsConn, int, SmProp**)

    void SmsCleanUp(SmsConn)

    int SmsProtocolVersion(SmsConn)

    int SmsProtocolRevision(SmsConn)

    char* SmsClientID(SmsConn)

    IceConn SmsGetIceConnection(SmsConn)

    SmcErrorHandler SmcSetErrorHandler(SmcErrorHandler)

    SmsErrorHandler SmsSetErrorHandler(SmsErrorHandler)

    void SmFreeProperty(SmProp*)

    void SmFreeReasons(int, char**)


###################

cdef struct A:
    char a
    char b
    IceConn             iceConn
    int                 proto_major_version
    int                 proto_minor_version
    char                *client_id
    SmsCallbacks        callbacks
    char                interaction_allowed
