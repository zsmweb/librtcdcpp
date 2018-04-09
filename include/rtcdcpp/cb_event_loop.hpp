#include <vector>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
typedef void (*open_cb)(void);
typedef void (*on_string_msg)(int pid, const char* message);
typedef void (*on_binary_msg)(void* message);
typedef void (*on_close)(int pid);
typedef void (*on_error)(const char* description);


typedef struct IceCandidate_C IceCandidate_C;
typedef void (*on_ice_cb)(IceCandidate_C ice_c);

class cb_event_loop {

typedef void (*dc_fn_ptr_pid)(int, void*, cb_event_loop*);

  public:
    cb_event_loop();
    ~cb_event_loop();
    void add_pull_socket(int pid, void *socket);
    void addSocket_pid(int pid);
    void add_pull_socket_pid(int pid);
    void addContext(int pid, void *context);
    void add_on_candidate(int pid, on_ice_cb fn_ptr);
    void add_on_datachannel(int pid, dc_fn_ptr_pid fn_ptr);
    void add_on_open(int pid, open_cb fn_ptr);
    void add_on_string(int pid, on_string_msg fn_ptr);
    void add_on_binary(int pid, on_binary_msg fn_ptr);
    void add_on_close(int pid, on_close fn_ptr);
    void add_on_error(int pid, on_error fn_ptr);
    void* getSocket(int pid);
    void addSocket(int pid, void* socket);
    void ctx_term();
  private:
    std::mutex vec_lk;
    void* getContext(int pid);
    std::thread* cb_event_loop_thread;
    std::unordered_map<int, void*> pids_vs_sockets;
    std::map<int, void*> pull_sockets;
    std::vector<int> socket_pids;
    std::vector<int> pull_socket_pids;
    std::unordered_map<int, void*> cb_pull_contexts;
    static void parent_cb_loop(cb_event_loop* cb_evt_loop);
    std::unordered_map<int, on_ice_cb> on_candidate_cb;
    std::unordered_map<int, dc_fn_ptr_pid> on_datachannel_cb;
    std::unordered_map<int, open_cb> on_open_cb;
    std::unordered_map<int, on_string_msg> on_stringmsg_cb;
    std::unordered_map<int, on_binary_msg> on_binarymsg_cb;
    std::unordered_map<int, on_close> on_close_cb;
    std::unordered_map<int, on_error> on_error_cb;
};

