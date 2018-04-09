#include <rtcdcpp/librtcdcpp.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
extern bool running;
#include <fcntl.h>

void onIceCallback(struct IceCandidate_C ice_cand) {

}

void custom_close1(int pid) {
  printf("\nCLOSE1 at %d of %d\n", getpid(), pid);
}

void onDCCallback(int pid, pc_info pc_info1, cb_event_loop* cb_loop) {
  printf("\n====Got datachannel!=======%d\n", getpid());
  SetOnClosedCallback(pid, cb_loop, custom_close1);
}

void custom_close2(int pid) {
  printf("\nCLOSE2 at %d of %d\n", getpid(), pid);
}

void onDCCallback1(int pid, pc_info pc_info1, cb_event_loop* cb_loop) {
  printf("\n====Got datachannel!=======%d\n", getpid());
  SetOnClosedCallback(pid, cb_loop, custom_close2);
  closeDataChannel(pc_info1);
}

int main() {
  bool running = false;

  struct RTCIceServer_C rtc_ice;
  rtc_ice.hostname = "54.172.47.69";
  rtc_ice.port = 3478;
  struct RTCConfiguration_C rtc_conf;
  GArray* ice_servers;
  ice_servers = g_array_new(false, false, sizeof(rtc_ice));
  g_array_append_val(ice_servers, rtc_ice);
  rtc_conf.ice_ufrag = NULL;
  rtc_conf.ice_pwd = NULL;
  rtc_conf.ice_servers = ice_servers;
  rtc_conf.ice_port_range1 = rtc_conf.ice_port_range2 = 0;
  struct RTCIceServer_C rtc_ice1;
  rtc_ice1.hostname = "54.172.47.69";
  rtc_ice1.port = 3478;
  struct RTCConfiguration_C rtc_conf1;
  GArray* ice_servers1;
  ice_servers1 = g_array_new(false, false, sizeof(rtc_ice1));
  g_array_append_val(ice_servers1, rtc_ice1);
  rtc_conf1.ice_ufrag = NULL;
  rtc_conf1.ice_pwd = NULL;
  rtc_conf1.ice_servers = ice_servers1;
  rtc_conf1.ice_port_range1 = rtc_conf1.ice_port_range2 = 0;

  struct DataChannel* dc;
  cb_event_loop* cb_loop;
  cb_loop = init_cb_event_loop();

  void* sock1;
  void* sock2;

  pc_info pc_info_ret1;
  pc_info pc_info_ret2;
  pc_info_ret1.pid = pc_info_ret2.pid = 0;
  pc_info_ret1.context = pc_info_ret2.context = NULL;
  pc_info_ret1 = newPeerConnection(rtc_conf, onIceCallback, onDCCallback1, cb_loop);
  pc_info_ret2 = newPeerConnection(rtc_conf1, onIceCallback, onDCCallback, cb_loop);

  ParseOffer(pc_info_ret1, ""); //trigger ICE
  ParseOffer(pc_info_ret2, ""); // ""

  CreateDataChannel(pc_info_ret1, "testchannel", "", 0x00, 0);

  printf("\n====================++++%d++\n", getpid());
  char* offer = GenerateOffer(pc_info_ret1);
  ParseOffer(pc_info_ret2, offer);
  free(offer);
  char* answer = GenerateAnswer(pc_info_ret2);
  ParseOffer(pc_info_ret1, answer);
  free(answer);
  processWait(cb_loop);
  return 0;
}
