/**
 * C Wrapper around the C++ classes.
 */

#include <unistd.h>
#include <strings.h>
#include <stdio.h>

#include <rtcdcpp/PeerConnection.hpp>
#include <rtcdcpp/librtcdcpp.h> //hpp?
#include <rtcdcpp/DataChannel.hpp>
#include <glib.h>
#include <stdbool.h>
#include <memory>
#include <functional>
#include <iostream>
#include <sys/types.h>

#include <zmq.h>
#define DESTROY_PC 0
#define PARSE_SDP 1
#define GENERATE_OFFER 2
#define GENERATE_ANSWER 3
#define SET_REMOTE_ICE_CAND 4
#define SET_REMOTE_ICE_CANDS 5
#define CREATE_DC 6
#define GET_DC_SID 7
#define GET_DC_TYPE 8
#define GET_DC_LABEL 9
#define GET_DC_PROTO 10
#define SEND_STRING 11
#define SEND_BINARY 12
#define CLOSE_DC 13
#define SET_ON_OPEN_CB 14
#define SET_ON_STRING_MSG_CB 15
#define SET_ON_BINARY_MSG_CB 16
#define SET_ON_CLOSED_CB 17
#define SET_ON_ERROR_CB 18

extern "C" {
  IceCandidate_C* newIceCandidate(const char* candidate, const char* sdpMid, int sdpMLineIndex) {
    //IceCandidate_C ice_cand;
    IceCandidate_C* ice_cand = (IceCandidate_C *) malloc(sizeof(IceCandidate_C));
    ice_cand->candidate = candidate; //&
    ice_cand->sdpMid = sdpMid;
    ice_cand->sdpMLineIndex = sdpMLineIndex;
    return ice_cand;
  }

  void sendSignal(void* zmqsock) {
    zmq_send (zmqsock, "", 0, 0);
  }

  void signalSink(void *zmqsock) {
    void* nothing;
    zmq_recv (zmqsock, nothing, 0, 0);
  }


  size_t length; // variable to reuse for storing length

  void* newPeerConnection(RTCConfiguration_C config_c, on_ice_cb ice_cb, on_dc_cb dc_cb) {
    spdlog::set_level(spdlog::level::trace);
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
    //TODO: Fix this.
    /*
       rtcdcpp::RTCCertificate *rtc_cert;
       rtc_cert = (rtcdcpp::RTCCertificate*) malloc(sizeof(rtcdcpp::RTCCertificate));
       for(int i = 0; i <= config_c.certificates->len; i++) {
       rtc_cert = &g_array_index(config_c.certificates, rtcdcpp::RTCCertificate, i);
       config.certificates.emplace_back(&rtc_cert);
       }
    */ 
    pid_t cpid = fork();

    // Create a new REQ zmq soc
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);

    if (cpid == 0) {
      //child
      PeerConnection* child_pc;

      DataChannel* child_dc;
      child_pc = new rtcdcpp::PeerConnection(config, onLocalIceCandidate, onDataChannel);
      void *child_context = zmq_ctx_new ();
      void *responder = zmq_socket (child_context, ZMQ_REP);
      int rc1 = zmq_bind (responder, "ipc:///tmp/testpath");

      //void *child_requester = zmq_socket (child_context,ZMQ_REQ);
      //int rc1_c = zmq_bind (child_requester, "ipc:///tmp/childpath");
      //
      //assert (rc1 == 0);
      //assert (rc1_c);
      bool alive = true;
      int command;

      while(alive) {
        zmq_recv (responder, &command, sizeof(command), 0);
        switch(command) {
          case DESTROY_PC:
            sendSignal(responder); // Respond.
            _destroyPeerConnection(child_pc);
            break;
          case PARSE_SDP:
            {
              // send dummy response on command socket. This means it can start sending arguments
              sendSignal(responder);
              // Wait for arg. (which is length)
              zmq_recv (responder, &length, sizeof(length), 0); // Wait for response
              // Ask for actual content in our response using dummy signal
              sendSignal(responder);
              char parse_sdp_arg[length];
              zmq_recv (responder, parse_sdp_arg, length, 0);
              _ParseOffer(child_pc, parse_sdp_arg);
              sendSignal(responder);
            }
            break;
          case GENERATE_OFFER:
            {
              char* offer;
              offer = _GenerateOffer(child_pc);
              length = strlen(offer);
              zmq_send (responder, &length, sizeof(length), 0); // Respond with length of generate offer
              signalSink(responder); // Wait for next dummy REQ to send the content
              zmq_send (responder, offer, length, 0); // Respond with content
            }
            break;
          case GENERATE_ANSWER:
            {
              char* answer;
              answer = _GenerateAnswer(child_pc);
              length = strlen(answer);
              zmq_send (responder, &length, sizeof(length), 0);
              signalSink(responder);
              zmq_send (responder, answer, length, 0);
            }
            break;
          case SET_REMOTE_ICE_CAND:
            {
              bool ret_bool;
              sendSignal(responder); // Respond with dummy signal to get length
              zmq_recv (responder, &length, sizeof(length), 0);
              sendSignal(responder); // Respond with dummy signal to get content
              char candidate_sdp_arg[length];
              zmq_recv (responder, candidate_sdp_arg, length, 0);
              ret_bool = _SetRemoteIceCandidate(child_pc, candidate_sdp_arg);
              zmq_send (responder, &ret_bool, sizeof(ret_bool), 0); // Respond with return value
            }
            break;
          case SET_REMOTE_ICE_CANDS:
            {
              GArray *args_garray; //GArray
              // !
              // Send signal to send GArray
              sendSignal(responder);
              GArray *candidate_sdps_arg;
              candidate_sdps_arg = (GArray *) malloc(sizeof(GArray));
              zmq_recv (responder, candidate_sdps_arg, sizeof(candidate_sdps_arg), 0);
              bool ret_bool = _SetRemoteIceCandidates(child_pc, candidate_sdps_arg);
              signalSink(responder);
              zmq_send (responder, &ret_bool, sizeof(ret_bool), 0);
            }
            break;
          case CREATE_DC:
            {
            sendSignal(responder);
            // Get label length
            size_t label_arg_length;
            size_t proto_arg_length;
            zmq_recv(responder, &label_arg_length, sizeof(label_arg_length), 0);
            sendSignal(responder); // req proto length
            zmq_recv(responder, &proto_arg_length, sizeof(proto_arg_length), 0);
            sendSignal(responder); // req label
            char label_arg[label_arg_length];
            char proto_arg[proto_arg_length];
            zmq_recv(responder, label_arg, label_arg_length, 0);
            sendSignal(responder); // req proto
            zmq_recv(responder, proto_arg, proto_arg_length, 0);
            child_dc = _CreateDataChannel(child_pc, label_arg, proto_arg);
            sendSignal(responder);
            }
            break;
          case GET_DC_SID:
            u_int16_t sid;
            //sid = _getDataChannelStreamID(child_dc);
            break;
          case GET_DC_TYPE:
            u_int8_t type;
            //type = _getDataChannelType(child_dc);
            break;
          case GET_DC_LABEL:
            const char* chan_label;
            //chan_label = _getDataChannelLabel(child_dc);
            break;
          case GET_DC_PROTO:
            const char* chan_proto;
            //chan_proto = _getDataChannelProtocol(child_dc);
            break;
          case SEND_STRING:
            // !
            //ret_bool = _SendString(child_dc, args_char);
            break;
          case SEND_BINARY:
            u_int8_t len;
            u_int8_t *msg_8t;
            //ret_bool = _SendBinary(child_dc, msg_8t, len);
            break;
          case SET_ON_OPEN_CB:
            //Get onOpenCB function pointer
            open_cb open_callback;
            //_SetOnOpen(child_dc, open_callback);
            break;
          case SET_ON_STRING_MSG_CB:
            on_string_msg string_callback;
            //_SetOnStringMsgCallback(child_dc, string_callback);
            break;
          case SET_ON_BINARY_MSG_CB:
            on_binary_msg binary_callback;
            //_SetOnBinaryMsgCallback(child_dc, binary_callback);
            break;
          case SET_ON_ERROR_CB:
            on_error error_callback;
            //_SetOnErrorCallback(child_dc, error_callback);
            break;
          case SET_ON_CLOSED_CB:
            on_close close_callback;
            //_SetOnClosedCallback(child_dc, close_callback);
            break;
          default:
            break;
        }
      }
    } else {
      // Parent
      int rc2 = zmq_connect(requester, "ipc:///tmp/testpath");
      //assert (rc2 == 0);
    }
    return requester;
  }


  void _destroyPeerConnection(PeerConnection *pc) {
    delete pc;
  }

  void _ParseOffer(PeerConnection* pc, const char* sdp) {
    std::string sdp_string (sdp);
    pc->ParseOffer(sdp_string);
  }

  void destroyPeerConnection(void *socket) {
    int command = DESTROY_PC;
    zmq_send (socket, &command, sizeof(command), 0);
    signalSink(socket);
  }

  void ParseOffer(void *socket, const char *sdp) {
    int child_command = PARSE_SDP;
    zmq_send (socket, &child_command, sizeof(child_command), 0); // Send command
    size_t sdp_length = strlen(sdp);
    signalSink(socket);
    zmq_send (socket, &sdp_length, sizeof(sdp_length), 0);
    signalSink(socket);
    zmq_send (socket, sdp, sdp_length, 0);
    signalSink(socket);
  }

  char* GenerateOffer(void* socket) {
    int child_command = GENERATE_OFFER;
    zmq_send (socket, &child_command, sizeof(child_command), 0); // Send command request
    // Response will contain length of generated offer
    zmq_recv (socket, &length, sizeof(length), 0);
    sendSignal(socket); // dummy request for content
    char* recv_offer = (char*) malloc(length);
    zmq_recv (socket, recv_offer, length, 0);
    return recv_offer;
  }

  char* GenerateAnswer(void *socket) {
    int command = GENERATE_ANSWER;
    zmq_send (socket, &command, sizeof(command), 0);
    zmq_recv (socket, &length, sizeof(length), 0);
    sendSignal(socket);
    char *answer = (char *) malloc(length);
    zmq_recv (socket, answer, length, 0);
    return answer;
  }

  bool SetRemoteIceCandidate(void *socket, const char* candidate_sdp) {
    int command = SET_REMOTE_ICE_CAND;
    zmq_send (socket, &command, sizeof(command), 0);
    signalSink(socket);
    size_t candidate_sdp_len = strlen(candidate_sdp);
    zmq_send (socket, &candidate_sdp_len, sizeof(candidate_sdp_len), 0);
    signalSink(socket);
    // send content
    zmq_send (socket, candidate_sdp, candidate_sdp_len, 0);
    // Get response that contains return boolean
    bool ret_val;
    zmq_recv (socket, &ret_val, sizeof(ret_val), 0);
    return ret_val;
  }

  bool SetRemoteIceCandidates(void *socket, const GArray* candidate_sdps) {
    //return false; // WIP
    int command = SET_REMOTE_ICE_CANDS;
    zmq_send (socket, &command, sizeof(command), 0); // Send command request
    signalSink(socket);
    zmq_send (socket, candidate_sdps, sizeof(candidate_sdps), 0);
    bool ret_val;
    zmq_recv (socket, &ret_val, sizeof(ret_val), 0);
    return ret_val;
  }

  void CreateDataChannel(void* socket, const char* label, const char* protocol) {
    int command = CREATE_DC;
    zmq_send (socket, &command, sizeof(command), 0);
    signalSink(socket);
    // send lengths
    size_t label_length = strlen(label);
    size_t protocol_length = strlen(protocol);
    zmq_send (socket, &label_length, sizeof(label_length), 0);
    signalSink(socket);
    zmq_send (socket, &protocol_length, sizeof(protocol_length), 0);
    signalSink(socket);
    zmq_send (socket, label, label_length, 0);
    signalSink(socket);
    zmq_send (socket, protocol, protocol_length, 0);
    signalSink(socket);
  };

  char* _GenerateOffer(PeerConnection *pc) {
    std::string ret_val;
    ret_val = pc->GenerateOffer();
    char* ret_val1 = (char*) malloc(ret_val.size());
    snprintf(ret_val1, ret_val.size(), ret_val.c_str());
    return ret_val1;
  }

  char* _GenerateAnswer(PeerConnection *pc) {
    std::string ret_val;
    ret_val = pc->GenerateAnswer();
    char* ret_val1 = (char*) malloc(ret_val.size());
    snprintf(ret_val1, ret_val.size(), ret_val.c_str());
    return ret_val1;
  }

  bool _SetRemoteIceCandidate(PeerConnection *pc, const char* candidate_sdp) {
    std::string candidate_sdp_string (candidate_sdp);
    return pc->SetRemoteIceCandidate(candidate_sdp_string);
  }

  bool _SetRemoteIceCandidates(PeerConnection *pc, const GArray* candidate_sdps) {
    std::vector<std::string> candidate_sdps_vec;
    for(int i = 0; i <= candidate_sdps->len; i++) {
      std::string candidate_sdp_string (&g_array_index(candidate_sdps, char, i));
      candidate_sdps_vec.emplace_back(candidate_sdp_string);
    }
    return pc->SetRemoteIceCandidates(candidate_sdps_vec);
  }

  DataChannel* _CreateDataChannel(PeerConnection *pc, const char* label, const char* protocol) {
    std::string label_string (label);
    std::string protocol_string (protocol);
    if (protocol_string.size() > 0) {
      return pc->CreateDataChannel(label, protocol).get();
    } else {
      return pc->CreateDataChannel(label).get();
    }
  }

  u_int16_t _getDataChannelStreamID(DataChannel *dc) {
    return dc->GetStreamID();
  }

  u_int8_t _getDataChannelType(DataChannel *dc) {
    return dc->GetChannelType();
  }

  const char* _getDataChannelLabel(DataChannel *dc) {
    std::string dc_label_string;
    dc_label_string = dc->GetLabel();
    return dc_label_string.c_str();
  }

  const char* _getDataChannelProtocol(DataChannel *dc) {
    std::string dc_proto_string;
    dc_proto_string = dc->GetProtocol();
    return dc_proto_string.c_str();
  }

  bool _SendString(DataChannel *dc, const char* msg) {
    std::string message (msg);
    return dc->SendString(message);
  }

  bool _SendBinary(DataChannel *dc, u_int8_t *msg, int len) {
    return dc->SendBinary(msg, len);
  }

  void _closeDataChannel(DataChannel *dc) {
    delete dc;
  }

  void _SetOnOpen(DataChannel *dc, open_cb on_open_cb) {
    dc->SetOnOpen(on_open_cb);
  }

  void _SetOnStringMsgCallback(DataChannel *dc, on_string_msg recv_str_cb) {
    std::function<void(std::string string1)> recv_callback = [recv_str_cb](std::string string1) {
      recv_str_cb(string1.c_str());
    };
    dc->SetOnStringMsgCallback(recv_callback);
  }

  void _SetOnBinaryMsgCallback(DataChannel *dc, on_binary_msg msg_binary_cb) {
    std::function<void(rtcdcpp::ChunkPtr)> msg_binary_callback = [msg_binary_cb](rtcdcpp::ChunkPtr chkptr) {
      msg_binary_cb((void *) chkptr.get()->Data());
    };
    dc->SetOnBinaryMsgCallback(msg_binary_callback);
  }

  void _SetOnClosedCallback(DataChannel *dc, on_close close_cb) {
    std::function<void()> close_callback = [close_cb]() {
      close_cb();
    };
    dc->SetOnClosedCallback(close_callback);
  }

  void _SetOnErrorCallback(DataChannel *dc, on_error error_cb) {
    std::function<void(std::string description)> error_callback = [error_cb](std::string description) {
      error_cb(description.c_str());
    };
    dc->SetOnErrorCallback(error_callback);
  }
}
