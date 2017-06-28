/**
 * C Wrapper around the C++ classes.
 */

#include <unistd.h>

#include <rtcdcpp/PeerConnection.hpp>
#include <rtcdcpp/librtcdcpp.h> //hpp?
#include <rtcdcpp/DataChannel.hpp>
#include <glib.h>
#include <stdbool.h>
#include <memory>
#include <functional>
#include <iostream>
#include <sys/types.h>

extern "C" {
  IceCandidate_C* newIceCandidate(const char* candidate, const char* sdpMid, int sdpMLineIndex) {
    //IceCandidate_C ice_cand;
    IceCandidate_C* ice_cand = (IceCandidate_C *) malloc(sizeof(IceCandidate_C));
    ice_cand->candidate = candidate; //&
    ice_cand->sdpMid = sdpMid;
    ice_cand->sdpMLineIndex = sdpMLineIndex;
    return ice_cand;
  }

  PeerConnection* newPeerConnection(RTCConfiguration_C config_c, on_ice_cb ice_cb, on_dc_cb dc_cb) {
    std::function<void(rtcdcpp::PeerConnection::IceCandidate)> onLocalIceCandidate = [ice_cb](rtcdcpp::PeerConnection::IceCandidate candidate) 
    {
      IceCandidate_C ice_cand_c;
      ice_cand_c.candidate = candidate.candidate.c_str(); //
      ice_cand_c.sdpMid = candidate.sdpMid.c_str();
      ice_cand_c.sdpMLineIndex = candidate.sdpMLineIndex;
      ice_cb(ice_cand_c);
    };

    std::function<void(std::shared_ptr<rtcdcpp::DataChannel> channel)> onDataChannel = [dc_cb](std::shared_ptr<rtcdcpp::DataChannel> channel) {
      dc_cb((DataChannel*) channel.get());
    };
    rtcdcpp::RTCConfiguration config;
    for(int i = 0; i < config_c.ice_servers->len; i++) {
      config.ice_servers.emplace_back(rtcdcpp::RTCIceServer{"stun3.l.google.com", 19302});
    }
    std::pair<unsigned, unsigned> port_range = std::make_pair(config_c.ice_port_range1, config_c.ice_port_range2);
    config.ice_port_range = port_range;
    
    if (config_c.ice_ufrag) {
      std::string ice_ufrag_string (config_c.ice_ufrag);
      config.ice_ufrag = ice_ufrag_string;
    }
    if (config_c.ice_pwd) {
      std::string ice_pwd_string (config_c.ice_pwd);
      config.ice_pwd = ice_pwd_string;
    }
    //TODO: Fix this
    /*
    for(int i = 0; i <= config_c.certificates->len; i++) {
      rtcdcpp::RTCCertificate *rtc_cert;
      rtc_cert = &g_array_index(config_c.certificates, rtcdcpp::RTCCertificate, i);
      config.certificates.emplace_back(&rtc_cert);
    }
    */
    return (PeerConnection*) new rtcdcpp::PeerConnection(config, onLocalIceCandidate, onDataChannel);
  }
  
  void destroyPeerConnection(PeerConnection *pc) {
    delete pc;
  }

  void ParseOffer(PeerConnection* pc, const char* sdp) {
    std::string sdp_string (sdp);
    pc->ParseOffer(sdp_string);
  }

  char* GenerateOffer(PeerConnection *pc) {
    std::string ret_val;
    ret_val = pc->GenerateOffer();
    char* ret_val1 = (char*) malloc(ret_val.size());
    snprintf(ret_val1, ret_val.size(), ret_val.c_str());
    return ret_val1; //
  }

  char* GenerateAnswer(PeerConnection *pc) {
    std::string ret_val;
    ret_val = pc->GenerateAnswer();
    char* ret_val1 = (char*) malloc(ret_val.size());
    snprintf(ret_val1, ret_val.size(), ret_val.c_str());
    return ret_val1;
  }

  bool SetRemoteIceCandidate(PeerConnection *pc, const char* candidate_sdp) {
    std::string candidate_sdp_string (candidate_sdp);
    return pc->SetRemoteIceCandidate(candidate_sdp_string);
  }

  bool SetRemoteIceCandidates(PeerConnection *pc, const GArray* candidate_sdps) {
    std::vector<std::string> candidate_sdps_vec;
    for(int i = 0; i <= candidate_sdps->len; i++) {
      std::string candidate_sdp_string (&g_array_index(candidate_sdps, char, i));
      candidate_sdps_vec.emplace_back(candidate_sdp_string);
    }
    return pc->SetRemoteIceCandidates(candidate_sdps_vec);
  }

  DataChannel* CreateDataChannel(PeerConnection *pc, const char* label, const char* protocol) {
    std::string label_string (label);
    std::string protocol_string (protocol);
    if (protocol_string.size() > 0) {
      return pc->CreateDataChannel(label, protocol).get();
    } else {
      return pc->CreateDataChannel(label).get();
    }
  }

  u_int16_t getDataChannelStreamID(DataChannel *dc) {
    return dc->GetStreamID();
  }

  u_int8_t getDataChannelType(DataChannel *dc) {
    return dc->GetChannelType();
  }

  const char* getDataChannelLabel(DataChannel *dc) {
    std::string dc_label_string; 
    dc_label_string = dc->GetLabel();
    return dc_label_string.c_str();
  }

  const char* getDataChannelProtocol(DataChannel *dc) {
    std::string dc_proto_string;
    dc_proto_string = dc->GetProtocol();
    return dc_proto_string.c_str();
  }

  bool SendString(DataChannel *dc, const char* msg) {
    std::string message (msg);
    return dc->SendString(message);
  }

  bool SendBinary(DataChannel *dc, const u_int8_t *msg, int len) {
    return dc->SendBinary(msg, len);
  }
  
  void closeDataChannel(DataChannel *dc) {
    delete dc;
  }

  void SetOnOpen(DataChannel *dc, open_cb on_open_cb) {
    dc->SetOnOpen(on_open_cb);
  }

  void SetOnStringMsgCallback(DataChannel *dc, on_string_msg recv_str_cb) {
    std::function<void(std::string string1)> recv_callback = [recv_str_cb](std::string string1) {
      recv_str_cb(string1.c_str());
    };
    dc->SetOnStringMsgCallback(recv_callback);
  }

  void SetOnBinaryMsgCallback(DataChannel *dc, on_binary_msg msg_binary_cb) {
    std::function<void(rtcdcpp::ChunkPtr)> msg_binary_callback = [msg_binary_cb](rtcdcpp::ChunkPtr chkptr) {
      msg_binary_cb((void *) chkptr.get()->Data());
    };
    dc->SetOnBinaryMsgCallback(msg_binary_callback);
  }

  void SetOnClosedCallback(DataChannel *dc, on_close close_cb) {
    std::function<void()> close_callback = [close_cb]() {
      close_cb();
    };
    dc->SetOnClosedCallback(close_callback);
  }

  void SetOnErrorCallback(DataChannel *dc, on_error error_cb) {
    std::function<void(std::string description)> error_callback = [error_cb](std::string description) {
      error_cb(description.c_str());
    };
    dc->SetOnErrorCallback(error_callback);
  }
}
