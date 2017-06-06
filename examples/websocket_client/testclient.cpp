/**
 * Simple WebRTC test client.
 */

#include "WebSocketWrapper.hpp"
#include "json/json.h"

#include <rtcdcpp/PeerConnection.hpp>
#include <rtcdcpp/Logging.hpp>

#include <ios>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unistd.h>
#define SCTP_DEBUG
#define SCTP_MBUF_LOGGING

#include<thread>

#include<chrono>
#include<ctime>

extern "C" {
#include "cpslib/pslib.h"
}
bool running = true;
using namespace rtcdcpp;
#include<usrsctp.h>

void send_loop(std::shared_ptr<DataChannel> dc) {
  std::ifstream bunnyFile;
  bunnyFile.open("frag_bunny.mp4", std::ios_base::in | std::ios_base::binary);

  char buf[100 * 1024];

  while (bunnyFile.good()) {
    bunnyFile.read(buf, 100 * 1024);
    int nRead = bunnyFile.gcount();
    if (nRead > 0) {
      dc->SendBinary((const uint8_t *)buf, nRead);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Sent message of size " << std::to_string(nRead) << std::endl;
  }
}
std::chrono::time_point<std::chrono::system_clock> start, end;
ChunkQueue messages;

  void onDCMessage(std::string message)
  {
    std::cout << message << "\n";
  };

  void onDCClose()
    {
     std::cout << "DC Close!" << "\n";
     running = false;
     messages.Stop();
    };
  int gdbbreak = 0;
  void stress1(std::shared_ptr<DataChannel> dc) {
    int i = 1;
    int count = 0;
    int max_count = 2000;
    int bytecount1 = 0, bytecount2 = 0;
    
    int j;
    j = 32; // size in kilobytes
    i = j * 1024;
    std::vector<CpuTimes> cpuvec;
    auto cpuvec_val = cpu_times(false);
    cpuvec.push_back(*cpuvec_val);
    std::cout << "===Testing fixed payloads of " << j << " kB " << max_count << " times===\n";
    start = std::chrono::system_clock::now();
    while (running && count < max_count) {
      count += 1;
      std::string test_str((size_t) i, 'A');
      try {
        dc->SendString(test_str);
      } catch(std::runtime_error& e) {
        std::cout << "BROKE at count: " << count << "\n";
        count--;
        break;
      }
      bytecount1 += i;
    }
    cpuvec_val = cpu_times(true);
    cpuvec.push_back(*cpuvec_val);
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    
    CpuTimes ct1 = cpuvec.front();
    double *cpures = cpu_util_percent(false, &ct1);
    std::cout << "CPU util: " << std::fixed << std::setprecision(1) << *cpures << "%\n";
    //std::cout << bytecount1 << " bytes sent.\n";

    int inc_stop;
    inc_stop = count;

    i = 1; 
    count = 0;
    int wait_1, close_wait;
    wait_1 = 4;

    std::cout << "TOTAL successful send_string calls: " << inc_stop + count << "\n";
    std::cout << "TOTAL data sent: " << bytecount1 / (1024 * 1024) << " MB\n";
    std::cout << "TOTAL time taken: " << elapsed_seconds.count() << " seconds\n";
    std::cout << "Data rate: " << ((bytecount1) / elapsed_seconds.count()) / (1024 * 1024) << " MB/s\n";
    
    close_wait = 3;
    std::cout << "\nWaiting " << close_wait << " seconds before closing DC.\n";
    usleep(close_wait * 1000000); //5s
    dc->Close();
    
  }
int main(void) {
  usrsctp_sysctl_set_sctp_debug_on(1);
  usrsctp_sysctl_set_sctp_blackhole(2);
  usrsctp_sysctl_set_sctp_ecn_enable(0);
  usrsctp_sysctl_set_sctp_logging_level(1);
  usrsctp_sysctl_set_sctp_buffer_splitting(1);
int main() {
#ifndef SPDLOG_DISABLED
  auto console_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
  spdlog::create("rtcdcpp.PeerConnection", console_sink);
  spdlog::create("rtcdcpp.SCTP", console_sink);
  spdlog::create("rtcdcpp.Nice", console_sink);
  spdlog::create("rtcdcpp.DTLS", console_sink);
  spdlog::set_level(spdlog::level::debug);
#endif

  WebSocketWrapper ws("ws://localhost:5000/channel/test");
  std::shared_ptr<PeerConnection> pc;
  std::shared_ptr<DataChannel> dc;
  if (!ws.Initialize()) {
    std::cout << "WebSocket connection failed\n";
    return 0;
  }

  RTCConfiguration config;
  config.ice_servers.emplace_back(RTCIceServer{"stun3.l.google.com", 19302});

// bool run

  std::function<void(std::string)> onMessage = [](std::string msg) {
    messages.push(std::shared_ptr<Chunk>(new Chunk((const void *)msg.c_str(), msg.length())));
  };

  std::function<void(PeerConnection::IceCandidate)> onLocalIceCandidate = [&ws](PeerConnection::IceCandidate candidate) {
    Json::Value jsonCandidate;
    jsonCandidate["type"] = "candidate";
    jsonCandidate["msg"]["candidate"] = candidate.candidate;
    jsonCandidate["msg"]["sdpMid"] = candidate.sdpMid;
    jsonCandidate["msg"]["sdpMLineIndex"] = candidate.sdpMLineIndex;

    Json::StreamWriterBuilder wBuilder;
    ws.Send(Json::writeString(wBuilder, jsonCandidate));
  };

  std::thread stress_thread;
  std::function<void(std::shared_ptr<DataChannel> channel)> onDataChannel = [&dc](std::shared_ptr<DataChannel> channel) {
    std::cout << "Hey cool, got a data channel\n";
    dc = channel;
    dc->SetOnClosedCallback(onDCClose);
    std::thread send_thread = std::thread(stress1, channel);
    send_thread.detach();
  };

  ws.SetOnMessage(onMessage);
  ws.Start();
  ws.Send("{\"type\": \"client_connected\", \"msg\": {}}");

  Json::Reader reader;
  Json::StreamWriterBuilder msgBuilder;

  while (running) {
    ChunkPtr cur_msg = messages.wait_and_pop();
    if (!running) {
      std::cout << "Breaking\n";
      break;
    }
    std::string msg((const char *)cur_msg->Data(), cur_msg->Length());
    std::cout << msg << "\n";
    Json::Value root;
    if (reader.parse(msg, root)) {
      //std::cout << "Got msg of type: " << root["type"] << "\n";
      if (root["type"] == "offer") {
        //std::cout << "Time to get the rtc party started\n";
        pc = std::make_shared<PeerConnection>(config, onLocalIceCandidate, onDataChannel);

        pc->ParseOffer(root["msg"]["sdp"].asString());
        Json::Value answer;
        answer["type"] = "answer";
        answer["msg"]["sdp"] = pc->GenerateAnswer();
        answer["msg"]["type"] = "answer";

        //std::cout << "Sending Answer: " << answer << "\n";
        ws.Send(Json::writeString(msgBuilder, answer));
      } else if (root["type"] == "candidate") {
        pc->SetRemoteIceCandidate("a=" + root["msg"]["candidate"].asString());
      }
    } else {
      std::cout << "Json parse failed"
                << "\n";
    }
  }
  
  pc.reset(); //lose pc.
  ws.Close();
  
  return 0;
}
