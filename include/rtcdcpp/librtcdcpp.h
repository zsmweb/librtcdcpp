/**
 * C Wrapper around the C++ Classes.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
typedef struct rtcdcpp::PeerConnection PeerConnection;
typedef struct rtcdcpp::DataChannel DataChannel;
#else
typedef struct PeerConnection PeerConnection;
typedef struct DataChannel DataChannel;
#endif

#include <stdbool.h>
#include <glib.h>
#include <sys/types.h>

struct RTCIceServer_C {
  const char* hostname;
  int port;
};

struct RTCConfiguration_C {
  const GArray* ice_servers; // RTCIceServer_C
  unsigned int ice_port_range1;
  unsigned int ice_port_range2;
  const char* ice_ufrag;
  const char* ice_pwd;
  const GArray* certificates; // RTCCertificate
};
struct IceCandidate_C {
    const char* candidate;
    const char* sdpMid;
    int sdpMLineIndex;
};
typedef struct IceCandidate_C IceCandidate_C;
IceCandidate_C* newIceCandidate(const char* candidate, const char* sdpMid, int sdpMLineIndex);

typedef void (*on_ice_cb)(IceCandidate_C ice_c);
typedef void (*on_dc_cb)(DataChannel *dc, void* socket);

void* newPeerConnection(struct RTCConfiguration_C config, on_ice_cb ice_cb, on_dc_cb dc_cb);

void sendSignal(void* zmqsock);
void signalSink(void* zmqsock);

void destroyPeerConnection(void* socket);
void ParseOffer(void* socket, const char* sdp);
char* GenerateOffer(void* socket);
char* GenerateAnswer(void* socket);
bool SetRemoteIceCandidate(void* socket, const char* candidate_sdp); 
bool SetRemoteIceCandidates(void* socket, const GArray* candidate_sdps);
void CreateDataChannel(void* socket, const char* label, const char* protocol);
// DataChannel member functions
// TODO 
u_int16_t getDataChannelStreamID(void* socket, DataChannel *dc);
u_int8_t getDataChannelType(void* socket, DataChannel *dc);
const char* getDataChannelLabel(void* socket, DataChannel *dc);
const char* getDataChannelProtocol(void* socket, DataChannel *dc);
bool SendString(void* socket, DataChannel *dc, const char* msg);
bool SendBinary(void* socket, DataChannel *dc, const u_int8_t *msg, int len);
void closeDataChannel(void* socket, DataChannel *dc);


void _destroyPeerConnection(PeerConnection* pc);
void _ParseOffer(PeerConnection* pc, const char* sdp);
char* _GenerateOffer(PeerConnection* pc);
char* _GenerateAnswer(PeerConnection* pc);
bool _SetRemoteIceCandidate(PeerConnection* pc, const char* candidate_sdp); 
bool _SetRemoteIceCandidates(PeerConnection* pc, const GArray* candidate_sdps);
DataChannel *_CreateDataChannel(PeerConnection* pc, const char* label, const char* protocol);
// DataChannel member functions
u_int16_t _getDataChannelStreamID(DataChannel *dc);
u_int8_t _getDataChannelType(DataChannel *dc);
const char* _getDataChannelLabel(DataChannel *dc);
const char* _getDataChannelProtocol(DataChannel *dc);
bool _SendString(DataChannel *dc, const char* msg);
bool _SendBinary(DataChannel *dc, u_int8_t *msg, int len);
void _closeDataChannel(DataChannel *dc);

//DataChannel Callback related methods
typedef void (*open_cb)(void);
typedef void (*on_string_msg)(const char* message);
typedef void (*on_binary_msg)(void* message);
typedef void (*on_close)(void);
typedef void (*on_error)(const char* description);

void SetOnOpen(void *socket, DataChannel *dc, open_cb on_open_cb);
void SetOnStringMsgCallback(void *socket, DataChannel *dc, on_string_msg recv_str_cb);
void SetOnBinaryMsgCallback(void *socket, DataChannel *dc, on_binary_msg msg_binary_cb);
void SetOnClosedCallback(void *socket, DataChannel *dc, on_close close_cb);
void SetOnErrorCallback(void *socket, DataChannel *dc, on_error error_cb);

void _SetOnOpen(DataChannel *dc, open_cb on_open_cb);
void _SetOnStringMsgCallback(DataChannel *dc, on_string_msg recv_str_cb);
void _SetOnBinaryMsgCallback(DataChannel *dc, on_binary_msg msg_binary_cb);
void _SetOnClosedCallback(DataChannel *dc, on_close close_cb);
void _SetOnErrorCallback(DataChannel *dc, on_error error_cb);

void processWait();
void exitter(int ret);
#ifdef __cplusplus
}
#endif
