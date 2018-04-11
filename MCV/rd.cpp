#include <zmq.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <thread>
#include <stdio.h>

int main() {
  char connect_path[35];
  int rc;
  int msg;
  char string_msg[35];
  pid_t child_pid = fork();

  if (child_pid == -1) {
    perror("fork error");
    return child_pid;
  }

  if (child_pid == 0) {
    // Child
    void* child_context = zmq_ctx_new ();
    if (child_context == NULL) {
      std::cerr << "\nChild context error\n";
    }

    void* router = zmq_socket(child_context, ZMQ_ROUTER);
    if (router == NULL) {
      perror("zmq_socket of type router error");
    }
    char bind_path[35];
    
    snprintf(bind_path, sizeof(bind_path), "ipc:///tmp/zmqtest%d-router", getpid());
    rc = zmq_bind(router, bind_path);
    assert (rc == 0);

    void* dealer = zmq_socket(child_context, ZMQ_DEALER);
    if (dealer == NULL) {
      perror("zmq_socket of type dealer error");
    }

    snprintf(bind_path, sizeof(bind_path), "ipc:///tmp/zmqtest%d-dealer", getpid());
    rc = zmq_bind(dealer, bind_path);
    assert (rc == 0);

    std::thread z_proxy (zmq_proxy, router, dealer, nullptr);
    z_proxy.detach();

    void* rep_socket = zmq_socket (child_context, ZMQ_REP);
    if (rep_socket == NULL) {
      perror("zmq_socket of type rep error");
    }

    snprintf(connect_path, sizeof(connect_path), "ipc:///tmp/zmqtest%d-dealer", getpid());
    rc = zmq_connect(rep_socket, connect_path);
    assert (rc == 0);

    while(1) {
      if (zmq_recv (rep_socket, &msg, sizeof(msg), 0) == -1) {
        perror("zmq_recv error");
      }
      printf("\nReceived msg %d in process %d\n", msg, getpid());
      break;
    }
    if (zmq_close(rep_socket) == -1) {
      perror("zmq_close of rep_socket in child error");
    }
    if (zmq_close(dealer) == -1) {
      perror("zmq_close of dealer in child error");
    }
    if (zmq_close(router) == -1) {
      perror("zmq_close of router in child error");
    }
    if (zmq_ctx_term(child_context) == -1) {
      perror("zmq_ctx_term of child_context error");
    }
  } else {
    // Parent
    sleep(1);
    
    void* parent_context = zmq_ctx_new ();
    if (parent_context == NULL) {
      std::cerr << "\nParent ctx error\n";
    }

    void* req_socket = zmq_socket (parent_context, ZMQ_REQ);
    if (req_socket == NULL) {
      perror("zmq_socket of type req error in parent");
    }

    snprintf(connect_path, sizeof(connect_path), "ipc:///tmp/zmqtest%d-router", child_pid);
    rc = zmq_connect(req_socket, connect_path);
    assert (rc == 0);

    msg = 30;
    if (zmq_send (req_socket, &msg, sizeof(msg), 0) == -1) {
      perror("zmq_send error in parent");
    }

    if (zmq_close(req_socket) == -1) {
      perror("zmq_close of req_socket in parent error");
    }
    if (zmq_ctx_term(parent_context) == -1) {
      perror("zmq_ctx_term of parent_context error");
    }
  }
  return 0;
}
