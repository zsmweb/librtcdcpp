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
struct f_descriptors {
      int first;
      int second;
};
//typedef struct f_descriptors f_descriptors;
struct f_descriptors newPeerConnection(struct RTCConfiguration_C config, on_ice_cb ice_cb, on_dc_cb dc_cb);

void destroyPeerConnection(pid_t cpid);
void ParseOffer(pid_t cpid, const char* sdp);
char* GenerateOffer(struct f_descriptors cpid);
char* GenerateAnswer(struct f_descriptors cpid);
bool SetRemoteIceCandidate(pid_t cpid, const char* candidate_sdp); 
bool SetRemoteIceCandidates(pid_t cpid, const GArray* candidate_sdps);
DataChannel *CreateDataChannel(pid_t cpid, const char* label, const char* protocol);
// DataChannel member functions
// TODO 
u_int16_t getDataChannelStreamID(DataChannel *dc);
u_int8_t getDataChannelType(DataChannel *dc);
const char* getDataChannelLabel(DataChannel *dc);
const char* getDataChannelProtocol(DataChannel *dc);
bool SendString(DataChannel *dc, const char* msg);
bool SendBinary(DataChannel *dc, const u_int8_t *msg, int len);
void closeDataChannel(DataChannel *dc);


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

/*
void _SetOnOpen(DataChannel *dc, open_cb on_open_cb);
void _SetOnStringMsgCallback(DataChannel *dc, on_string_msg recv_str_cb);
void _SetOnBinaryMsgCallback(DataChannel *dc, on_binary_msg msg_binary_cb);
void _SetOnClosedCallback(DataChannel *dc, on_close close_cb);
void _SetOnErrorCallback(DataChannel *dc, on_error error_cb);
*/


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
