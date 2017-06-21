/**
 * Copyright (c) 2017, Andrew Gault, Nick Chadwick and Guillaume Egles.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
const char* GenerateOffer(PeerConnection* pc);
const char* GenerateAnswer(PeerConnection* pc);

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
