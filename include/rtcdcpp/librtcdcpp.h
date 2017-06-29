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
typedef void (*on_dc_cb)(DataChannel *dc);
PeerConnection* newPeerConnection(struct RTCConfiguration_C config, on_ice_cb ice_cb, on_dc_cb dc_cb);
void destroyPeerConnection(PeerConnection* pc);

void ParseOffer(PeerConnection* pc, const char* sdp);
char* GenerateOffer(PeerConnection* pc);
char* GenerateAnswer(PeerConnection* pc);

bool SetRemoteIceCandidate(PeerConnection* pc, const char* candidate_sdp); 
bool SetRemoteIceCandidates(PeerConnection* pc, const GArray* candidate_sdps);

DataChannel *CreateDataChannel(PeerConnection* pc, const char* label, const char* protocol);

// DataChannel member functions
u_int16_t getDataChannelStreamID(DataChannel *dc);
u_int8_t getDataChannelType(DataChannel *dc);
const char* getDataChannelLabel(DataChannel *dc);
const char* getDataChannelProtocol(DataChannel *dc);

bool SendString(DataChannel *dc, const char* msg);
bool SendBinary(DataChannel *dc, const u_int8_t *msg, int len);

void closeDataChannel(DataChannel *dc);

//DataChannel Callback related methods
typedef void (*open_cb)(void);
typedef void (*on_string_msg)(const char* message);
typedef void (*on_binary_msg)(void* message);
typedef void (*on_close)(void);
typedef void (*on_error)(const char* description);

//DataChannel Callback setters
void SetOnOpen(DataChannel *dc, open_cb on_open_cb);
void SetOnStringMsgCallback(DataChannel *dc, on_string_msg recv_str_cb);
void SetOnBinaryMsgCallback(DataChannel *dc, on_binary_msg msg_binary_cb);
void SetOnClosedCallback(DataChannel *dc, on_close close_cb);
void SetOnErrorCallback(DataChannel *dc, on_error error_cb);
#ifdef __cplusplus
}
#endif
