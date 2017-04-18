/**
 * Simple WebRTC test client.
 */

#include "WebSocketWrapper.hpp"
#include "json/json.h"

#include <rtcdcpp/PeerConnection.hpp>
#include <rtcdcpp/Logging.hpp>

#include <ios>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include<thread>

bool running = true;
using namespace rtcdcpp;

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
    int bytecount1 = 0, bytecount2 = 0;
    //std::string s(262145, 'a');
    //dc->SendString(s);
    
    std::cout << "===Testing throughput using incremented strings===\n";
    while (running && count < 723) {
      count += 1;
      //std::cout << "Sending " << (size_t) i << " bytes...\n";
      std::string test_str((size_t) i, 'A');
      try {
        //usleep(30000);
        
        dc->SendString(test_str);
      } catch(std::runtime_error& e) {
        std::cout << "BROKE at count: " << count << "\n";
        count--;
        break;
      }
      i++;
      bytecount1 += i;
    }
    std::cout << bytecount1 << " bytes sent.\n";
    int inc_stop;
    inc_stop = count;
    i = 1; count = 0;
    // sleep here?
    int wait_1, close_wait;
    wait_1 = 4;
    
    std::cout << "\nWaiting " << wait_1 << " seconds.\n";
    usleep(wait_1 * 1000000);
    
    std::cout << "===Testing throughput using single char spam===\n";
    while (running && count < 419) {
      count += 1;
      std::string test_str((size_t) i, 'A');
      try {
        //usleep(30000);
        if (count == 419) { gdbbreak = 1; }
        dc->SendString(test_str);
      } catch(std::runtime_error& e) {
        std::cout << "BROKE at count: " << count << "\n";
        count--;
        break;
      }
    }
    std::cout << count << " bytes sent.\n\n";
    std::cout << "Incremental throughput stops at: " << inc_stop << "\n";
    std::cout << "Single char spam stops at count: " << count << "\n";
    std::cout << "TOTAL successful send_string calls: " << inc_stop + count << "\n";
    std::cout << "TOTAL bytes sent: " << bytecount1 + count << "\n";
    
    close_wait = 3;
    std::cout << "\nWaiting " << close_wait << " seconds before closing DC.\n";
    usleep(close_wait * 1000000); //5s
    dc->Close();
    
  }
int main(void) {
#ifndef SPDLOG_DISABLED
  auto console_sink = std::make_shared<spdlog::sinks::ansicolor_sink>(spdlog::sinks::stdout_sink_mt::instance());
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

  std::function<void(std::string)> onMessage = [&messages](std::string msg) {
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
  std::function<void(std::shared_ptr<DataChannel> channel)> onDataChannel = [&dc, &messages, &stress_thread, &start_stress](std::shared_ptr<DataChannel> channel) {
    std::cout << "Hey cool, got a data channel\n";
    dc = channel;
    std::thread send_thread = std::thread(send_loop, channel);
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
