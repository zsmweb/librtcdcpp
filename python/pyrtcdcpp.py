from libpyrtcdcpp import ffi, lib
from typing import List, Callable

class DataChannel():
    def __init__(self, dc, pc, cb_loop):
        self.dc = dc # This is pid now
        self.pc = pc
        self.cb_loop = cb_loop

    def SendString(self, msg:str):
        if len(msg) > 0:
            msg = ffi.new("char[]", bytes(msg, 'utf-8'))
            lib.SendString(self.pc, msg)

    def SendBinary(self, msg:bytes):
        msg = ffi.new("u_int_8_t", msg) 
        lib.SendBinary(self.pc, msg, len(msg))

    def SetOnOpen(self, on_open):
        lib.SetOnOpen(self.dc, self.cb_loop, lib.onOpened)

    def SetOnStringMsgCallback(self, on_string):
        lib.SetOnStringMsgCallback(self.dc, self.cb_loop, lib.onStringMsg)

    def SetOnBinaryMsgCallback(self, on_binary):
        @ffi.def_extern()
        def onBinaryMsg(msg):
            if on_binary is not None:
                on_binary(msg) #
        lib.SetOnBinaryMsgCallback(self.pc, lib.onBinaryMsg)

    def SetOnClosedCallback(self, on_closed):
        lib.SetOnClosedCallback(self.dc, self.cb_loop, lib.onClosed)

    def SetOnErrorCallback(self, on_error):
        @ffi.def_extern()
        def onError(description):
            if on_error is not None:
                on_error(ffi.string(description).decode('utf-8'))
        lib.SetOnErrorCallback(self.pc, lib.onError)

    def Close(self):
        lib.closeDataChannel(self.pc);

    def getStreamID(self) -> int:
        return lib.getDataChannelStreamID(self.pc)

    #u_int8_t getDataChannelType(DataChannel *dc); # TODO

    def getLabel(self) -> str:
        return ffi.string(lib.getDataChannelLabel(self.pc)).decode('utf-8')

    def getProtocol(self) -> str:
        return ffi.string(lib.getDataChannelProtocol(self.pc)).decode('utf-8')

# This can be a dict instead of a class
class RTCConf():
    def __init__(self, ice_servers: List[tuple], ice_ufrag:str = None, ice_pwd:str = None):
        self._ice_servers = ice_servers
        self._ice_ufrag = ice_ufrag
        self._ice_pwd = ice_pwd

class PeerConnection():
    # Methods that could be overridden.
    def onMessage(self, message):
        '''
        When a message is received on the data channel
        '''
        pass

    def onClose(self):
        '''
        When the data channel closes
        '''
        pass

    def onChannel(self, channel):
        '''
        When data channel is obtained
        '''
        pass

    def onCandidate(self, candidate):
        '''
        When the peer connection gets a new local ICE candidate
        '''
        pass

    # Private methods
    def _onChannel(self, channel, pc):
        # Set our private onMessage and onClose callbacks
        channel.SetOnClosedCallback(self.onClose)
        channel.SetOnStringMsgCallback(self.onMessage)
        # TODO: Implement OnBinary
        #channel.SetOnBinaryMsgCallback(self.onMessage)
        self.onChannel(channel)

    def __init__(self, cb_evt_loop, rtc_conf:RTCConf = None, onIceCallback_p = None, onDCCallback_p = None):
        if rtc_conf is None:
            rtc_conf = RTCConf([("stun3.l.google.com", 19302)]) # Hardcoded default config/setting for STUN server

        if onIceCallback_p is None:
            onIceCallback_p = self.onCandidate
        if onDCCallback_p is None:
            onDCCallback_p = self._onChannel
        
        # Note: These two below aren't 'redefinied'. Take it like one global callback function
        # that then calls the correct callback.

        @ffi.def_extern()
        def onStringMsg(pid: int, msg):
            callback_fn = cb_evt_loop.get_py_strmsg_cb(pid)
            callback_fn(ffi.string(msg).decode('utf-8'))

        #Same issue with this
        @ffi.def_extern()
        def onClosed(pid: int):
            callback_fn = cb_evt_loop.get_py_close_cb(pid)
            callback_fn()

        garray = ffi.new("GArray* ice_servers")
        ice_struct = ffi.new("struct RTCIceServer_C *")
        garray = lib.g_array_new(False, False, ffi.sizeof(ice_struct[0]))
        for ice_server in rtc_conf._ice_servers:
            hostname = ffi.new("char[]", bytes(ice_server[0], 'utf-8'))
            ice_struct.hostname = hostname
            ice_struct.port = ice_server[1]
            lib.g_array_append_vals(garray, ice_struct, 1)
        rtc_conf_s = ffi.new("struct RTCConfiguration_C *") #nm
        rtc_conf_s.ice_ufrag = ffi.NULL if rtc_conf._ice_ufrag is None else rtc_conf._ice_ufrag
        rtc_conf_s.ice_pwd = ffi.NULL if rtc_conf._ice_pwd is None else rtc_conf._ice_pwd
        rtc_conf_s.ice_servers = garray

        # Note: Both onDCCallback and onIceCallback aren't redefined.
        
        @ffi.def_extern()
        def onDCCallback(pid: int, pc, cb_loop):
            argument = DataChannel(pid, pc, cb_loop)
            callback_fn = cb_evt_loop.get_py_dc_cb(pid)
            callback_fn(argument, pc)
        
        @ffi.def_extern()
        def onIceCallback(ice):
            arguments = {}
            pid: int = ice.pid
            callback_fn = cb_evt_loop.get_py_ice_cb(pid) 
            arguments["candidate"] = ffi.string(ice.candidate).decode('utf-8')
            arguments["sdpMid"] = ffi.string(ice.sdpMid).decode('utf-8')
            arguments["sdpMLineIndex"] = ice.sdpMLineIndex
            callback_fn(arguments)

        ret_val = lib.newPeerConnection(rtc_conf_s[0], lib.onIceCallback, lib.onDCCallback, cb_evt_loop.get_event_loop())
        cb_evt_loop.add_py_dc_cb(ret_val.pid, onDCCallback_p)
        cb_evt_loop.add_py_ice_cb(ret_val.pid, onIceCallback_p)
        cb_evt_loop.add_py_close_cb(ret_val.pid, self.onClose)
        cb_evt_loop.add_py_strmsg_cb(ret_val.pid, self.onMessage)
        self.pc = ret_val.socket

    def GenerateOffer(self) -> str:
        retvl = lib.GenerateOffer(self.pc)
        if retvl != ffi.NULL:
            return ffi.string(retvl).decode()
        else:
            return None

    def GenerateAnswer(self) -> str:
        retvl = lib.GenerateAnswer(self.pc)
        if retvl != ffi.NULL:
            return ffi.string(retvl).decode()
        else:
            return None

    def ParseOffer(self, offer:str) -> bool:
        sdp = ffi.new("char[]", bytes(offer, 'utf-8'))
        if str(self.pc)[16:-1] == '0x1':
            #print('PC object is null')
            return
        return lib.ParseOffer(self.pc, sdp)

    def CreateDataChannel(self, label:str, cb_loop, protocol:str = None): # CDATA ret
        if str(self.pc)[16:-1] == '0x1':
            #print('PC object is null')
            return
        protocol = ffi.new("char[]", bytes('', 'utf-8')) if protocol is None else ffi.new("char[]", bytes(label, 'utf-8'))
        label = ffi.new("char[]", bytes(label, 'utf-8'))
        pid = lib.CreateDataChannel(self.pc, label, protocol)
        dc = DataChannel(pid, self.pc, cb_loop)
        return dc
    
    def SetRemoteIceCandidate(self, candidate:str) -> bool:
        candidate = ffi.new("char[]", bytes(candidate, 'utf-8')) #sm nm
        return lib.SetRemoteIceCandidate(self.pc, candidate)

    def SetRemoteIceCandidates(self, candidate_sdps:List[str]) -> bool:
        garray = ffi.new("GArray* candidate_sdps")
        for sdp in candidate_sdps:
            candidate = ffi.new("char[]", bytes(sdp[0], 'utf-8')) #s[
            lib.g_array_append_vals(garray, candidate, 1)
        return lib.SetRemoteIceCandidates(self.pc, garray) #

    def Close(self):
        lib.closeDataChannel(self.pc);

class init_cb_event_loop():
    def __init__(self):
        self.dc_callbacks = {}
        self.ice_callbacks = {}
        self.close_callbacks = {}
        self.strmsg_callbacks = {} # h
        self.cb_event_loop = None
    def get_event_loop(self):
        if self.cb_event_loop == None:
            self.cb_event_loop = lib.init_cb_event_loop()
        return self.cb_event_loop
    def add_py_dc_cb(self, pid:int, callback):
        self.dc_callbacks[pid] = callback
    def add_py_ice_cb(self, pid:int, callback):
        self.ice_callbacks[pid] = callback
    def add_py_close_cb(self, pid:int, callback):
        self.close_callbacks[pid] = callback
    def get_py_close_cb(self, pid:int):
        return self.close_callbacks[pid]
    def add_py_strmsg_cb(self, pid:int, callback):
        self.strmsg_callbacks[pid] = callback
    def get_py_strmsg_cb(self, pid:int):
        return self.strmsg_callbacks[pid]
    def get_py_dc_cb(self, pid:int):
        return self.dc_callbacks[pid]
    def get_py_ice_cb(self, pid:int):
        return self.ice_callbacks[pid]

def processWait():
    lib.processWait()

def exitter(arg:int):
    lib.exitter(arg)
