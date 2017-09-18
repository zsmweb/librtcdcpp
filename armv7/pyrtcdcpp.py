from libpyrtcdcpp import ffi, lib
from typing import List, Callable

class DataChannel():
    def __init__(self, dc, pc):
        self.dc = dc
        self.pc = pc #This is effectively nothing/invalid

    def SendString(self, msg:str):
        if len(msg) > 0:
            msg = ffi.new("char[]", bytes(msg, 'utf-8'))
            lib._SendString(self.dc, msg)

    def SendBinary(self, msg:bytes):
        msg = ffi.new("u_int_8_t", msg) 
        lib._SendBinary(self.dc, msg, len(msg))

    def SetOnOpen(self, on_open):
        @ffi.def_extern()
        def onOpened():
            if on_open is not None:
                on_open()
        lib._SetOnOpen(self.dc, lib.onOpened)

    def SetOnStringMsgCallback(self, on_string):
        @ffi.def_extern()
        def onStringMsg(msg):
            if on_string is not None:
                on_string(ffi.string(msg).decode('utf-8'))
        lib._SetOnStringMsgCallback(self.dc, lib.onStringMsg)

    def SetOnBinaryMsgCallback(self, on_binary):
        @ffi.def_extern()
        def onBinaryMsg(msg):
            if on_binary is not None:
                on_binary(msg) #
        lib._SetOnBinaryMsgCallback(self.dc, lib.onBinaryMsg)

    def SetOnClosedCallback(self, on_closed):
        @ffi.def_extern()
        def onClosed():
            if on_closed is not None:
                on_closed()
        lib._SetOnClosedCallback(self.dc, lib.onClosed)

    def SetOnErrorCallback(self, on_error):
        @ffi.def_extern()
        def onError(description):
            if on_error is not None:
                on_error(ffi.string(description).decode('utf-8'))
        lib._SetOnErrorCallback(self.dc, lib.onError)

    def Close(self):
        lib._closeDataChannel(self.dc);

    def getStreamID(self) -> int:
        return lib._getDataChannelStreamID(self.dc)

    #u_int8_t getDataChannelType(DataChannel *dc); # TODO

    def getLabel(self) -> str:
        return ffi.string(lib._getDataChannelLabel(self.dc)).decode('utf-8')

    def getProtocol(self) -> str:
        return ffi.string(lib._getDataChannelProtocol(self.dc)).decode('utf-8')

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
        channel.SetOnBinaryMsgCallback(self.onMessage)
        self.onChannel(channel)

    def __init__(self, rtc_conf:RTCConf = None, onIceCallback_p = None, onDCCallback_p = None):
        if rtc_conf is None:
            rtc_conf = RTCConf([("stun3.l.google.com", 19302)]) # Hardcoded default config/setting for STUN server

        if onIceCallback_p is None:
            onIceCallback_p = self.onCandidate
        if onDCCallback_p is None:
            onDCCallback_p = self._onChannel

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

        @ffi.def_extern()
        def onDCCallback(dc, pc):
            if onDCCallback_p is not None:
                argument = DataChannel(dc, pc)
                onDCCallback_p(argument, pc)
        
        @ffi.def_extern()
        def onIceCallback(ice):
            if onIceCallback_p is not None:
                arguments = {}
                arguments["candidate"] = ffi.string(ice.candidate).decode('utf-8')
                arguments["sdpMid"] = ffi.string(ice.sdpMid).decode('utf-8')
                arguments["sdpMLineIndex"] = ice.sdpMLineIndex
                onIceCallback_p(arguments)
        self.pc = lib.newPeerConnection(rtc_conf_s[0], lib.onIceCallback, lib.onDCCallback)

    def GenerateOffer(self) -> str:
        return ffi.string(lib.GenerateOffer(self.pc)).decode()

    def GenerateAnswer(self) -> str:
        return ffi.string(lib.GenerateAnswer(self.pc)).decode()

    def ParseOffer(self, offer:str) -> bool:
        sdp = ffi.new("char[]", bytes(offer, 'utf-8'))
        return lib.ParseOffer(self.pc, sdp)

    def CreateDataChannel(self, label:str, protocol:str = None): # CDATA ret
        protocol = ffi.new("char[]", bytes('', 'utf-8')) if protocol is None else ffi.new("char[]", bytes(label, 'utf-8'))
        label = ffi.new("char[]", bytes(label, 'utf-8'))
        retval = lib.CreateDataChannel(self.pc, label, protocol)
        dc = DataChannel(retval, self.pc)
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

def processWait():
    lib.processWait()

def exitter(arg:int):
    lib.exitter(arg)
