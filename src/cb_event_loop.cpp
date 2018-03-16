#include <rtcdcpp/cb_event_loop.hpp>
#include <thread>
#include <zmq.h>
#include <callbacks.pb.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
struct IceCandidate_C {
    int pid;
    const char* candidate;
    const char* sdpMid;
    int sdpMLineIndex;
};
cb_event_loop::cb_event_loop() {
  cb_event_loop_thread = new std::thread(cb_event_loop::parent_cb_loop, this);
  cb_event_loop_thread->detach();
}

cb_event_loop::~cb_event_loop() {
  cb_event_loop_thread->~thread();
  delete cb_event_loop_thread;
}

void* cb_event_loop::getSocket(int pid) {
  return this->pids_vs_sockets[pid];
}

void cb_event_loop::addSocket(int pid, void* socket) {
  if (!this->pids_vs_sockets.emplace(pid, socket).second) { 
    this->pids_vs_sockets.erase(pid);
    this->pids_vs_sockets.emplace(pid, socket);
  }
}

void cb_event_loop::add_pull_socket(int pid, void *socket) {
  this->pull_sockets.insert_or_assign(pid, socket);
}

void cb_event_loop::add_pull_context(void *context) {
  this->cb_pull_context = context;
}

void cb_event_loop::ctx_term() {
  if (zmq_ctx_term(this->cb_pull_context) != 0) {
    perror("cb_pull_socket ctx term error");
  }
}

void cb_event_loop::add_on_candidate(int pid, on_ice_cb fn_ptr) {
  if (!this->on_candidate_cb.emplace(pid, fn_ptr).second) {
    this->on_candidate_cb.erase(pid);
    this->on_candidate_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::add_on_datachannel(int pid, dc_fn_ptr_pid fn_ptr) {
  if (!this->on_datachannel_cb.emplace(pid, fn_ptr).second) {
    this->on_datachannel_cb.erase(pid);
    this->on_datachannel_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::add_on_open(int pid, open_cb fn_ptr) {
  if (!this->on_open_cb.emplace(pid, fn_ptr).second) {
    this->on_open_cb.erase(pid);
    this->on_open_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::add_on_string(int pid, on_string_msg fn_ptr) {
  if (!this->on_stringmsg_cb.emplace(pid, fn_ptr).second) {
    this->on_stringmsg_cb.erase(pid);
    this->on_stringmsg_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::add_on_binary(int pid, on_binary_msg fn_ptr) {
  if (!this->on_binarymsg_cb.emplace(pid, fn_ptr).second) {
    this->on_binarymsg_cb.erase(pid);
    this->on_binarymsg_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::add_on_close(int pid, on_close fn_ptr) {
  if (!this->on_close_cb.emplace(pid, fn_ptr).second) {
    this->on_close_cb.erase(pid);
    this->on_close_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::add_on_error(int pid, on_error fn_ptr) {
  if (!this->on_error_cb.emplace(pid, fn_ptr).second) {
    this->on_error_cb.erase(pid);
    this->on_error_cb.emplace(pid, fn_ptr);
  }
}

void cb_event_loop::parent_cb_loop(cb_event_loop* cb_evt_loop) {
  bool alive = true;
  while(alive) {
    zmq_msg_t cb_msg;
    int cb_msg_init = zmq_msg_init (&cb_msg);
    assert (cb_msg_init == 0);
    int cb_zmq_recv;
    for (auto const &pull_socket : cb_evt_loop->pull_sockets) {
      int pid = pull_socket.first; // auto pid?
      cb_zmq_recv = zmq_recvmsg (pull_socket.second, &cb_msg, ZMQ_DONTWAIT);
      if (cb_zmq_recv == -1) {
        if (errno == 11) {
          // EAGAIN; which means it was in non blocking mode and no messages were found
          // printf("\nNo message found\n");
          // std::this_thread::sleep_for(std::chrono::milliseconds(50));
          continue;
        }
      }
      std::string recv_string((const char *) zmq_msg_data(&cb_msg));
      librtcdcpp::Callback cb_obj;
      // printf("\nRecv string size: %zu\n", recv_string.size());
      if (cb_obj.ParseFromString(recv_string) == false) {
        // printf("\nParseFromString returned false\n"); // not sure what that bool actually means. Wasn't able to find proper doc
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // continue;
      }
      if (cb_obj.has_on_cand()) {
        const librtcdcpp::Callback::onCandidate& parsed_candidate = cb_obj.on_cand();
        // cb_obj
        auto callback_fn = cb_evt_loop->on_candidate_cb[pid];
        IceCandidate_C cand_arg;
        cand_arg.pid = pid;
        cand_arg.candidate = parsed_candidate.candidate().c_str();
        cand_arg.sdpMid = parsed_candidate.sdpmid().c_str();
        cand_arg.sdpMLineIndex = parsed_candidate.sdpmlineindex();
        //printf("\n[C] candidate for pid %d is %s\n", pid, cand_arg.candidate);
        callback_fn(cand_arg);
      } else {
        if (cb_obj.has_on_msg()) {
          const librtcdcpp::Callback::onMessage& parsed_message = cb_obj.on_msg();
          auto callback_fn = cb_evt_loop->on_stringmsg_cb[pid];
          if (callback_fn != NULL) {
            callback_fn(pid, parsed_message.message().c_str());
          }
        } else {
          if (cb_obj.cbwo_args() == librtcdcpp::Callback::ON_CHANNEL) {
            auto callback_fn = cb_evt_loop->on_datachannel_cb[pid];
            callback_fn(pid, cb_evt_loop->getSocket(pid), cb_evt_loop);
          } else if (cb_obj.cbwo_args() == librtcdcpp::Callback::ON_CLOSE) {
            auto callback_fn = cb_evt_loop->on_close_cb[pid];
            if (callback_fn) {
              callback_fn(pid);
            }
            //signal the command loop on its child to exit out of it
            zmq_send(cb_evt_loop->getSocket(pid), "a", sizeof(char), 0);
            if (zmq_close(cb_evt_loop->pull_sockets[pid]) != 0) {
              perror("Close pull_socket error:");
            } else {
              if (zmq_close(cb_evt_loop->getSocket(pid)) != 0) {
                perror("Close requester socket error:");
              }
            }
            cb_evt_loop->pull_sockets.erase(pid);
            alive = false;
            break;
          }
          //TODO: onError
        }
      }
    }
    if (cb_evt_loop->pull_sockets.empty() && alive == false) {
      break;
    } else {
      alive = true;
      continue;
    }
  }
}
