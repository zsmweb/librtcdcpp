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
//#include "/usr/include/linux/fcntl.h"
// Set callbacks insie onChannel and use the underscore prefix APIs
// TODO: Remove the _ from prefix APIs and deprecate the other ones
gchar* getlines() {
  size_t len = 0, linesize, inc_size;
  gchar *line, *lines=NULL;
  inc_size = 0;
  linesize = 2;
  while (linesize > 1) {
    linesize = getline(&line, &len, stdin);
    size_t strlength = strlen(line);
    size_t old_size = inc_size;
    inc_size += (sizeof(char) * strlength);
    lines = realloc(lines, inc_size);
    for (int i = 0; i < strlength; i++) {
      lines[old_size + i] = line[i];
    }
  }
  lines = realloc(lines, inc_size + 1);
  lines[inc_size + 1] = '\0';
  return lines;
}

void stress1(void *socket) {
  int i = 1;
  int count = 0;
  int max_count = 2000;
  int bytecount1 = 0, bytecount2 = 0;

  int j;
  j = 16; // size in kilobytes
  i = j * 1024;
  //std::vector<CpuTimes> cpuvec;
  //auto cpuvec_val = cpu_times(false);
  //cpuvec.push_back(*cpuvec_val);
  printf("\n===Testing fixed payloads of %d kB %d times===\n", j, max_count);
  time_t start;
  time(&start);
  char test_str[i];
  for (int str_p = 0; str_p < i; str_p++) {
    test_str[str_p] = 'A';
  }
  test_str[i] = '\0';
  while (count < max_count) {
    count += 1;
    if (SendString(socket, test_str) == 0) {
      printf("\n BROKE at count: %d\n", count);
      count--;
      break;
    }
    bytecount1 += i;
  }
  //cpuvec_val = cpu_times(true);
  //cpuvec.push_back(*cpuvec_val);
  time_t end;
  time(&end);
  int elapsed_seconds;
  elapsed_seconds = end - start;
  //elapsed_seconds = difftime(end, start);
  //CpuTimes ct1 = cpuvec.front();
  //double *cpures = cpu_util_percent(false, &ct1);
  //std::cout << "CPU util: " << std::fixed << std::setprecision(1) << *cpures << "%\n";
  //std::cout << bytecount1 << " bytes sent.\n";

  int inc_stop;
  inc_stop = count;

  i = 1;
  count = 0;
  int wait_1, close_wait;
  wait_1 = 4;

  printf("Incremental throughput stops at: %d\n", inc_stop);
  printf("Single char spam stops at count: %d\n", count);
  printf("TOTAL successful send_string calls: %d\n", inc_stop + count);
  printf("TOTAL data sent: %d MB\n", bytecount1 / (1024 * 1024));
  printf("TOTAL time taken: %d seconds\n", elapsed_seconds);
  printf("Data rate: %d MB/s\n", (bytecount1 / elapsed_seconds) / (1024 * 1024));
  close_wait = 1;
  printf("\nWaiting %d seconds before closing DC.\n", close_wait);
  usleep(close_wait * 1000000); //5s
  closeDataChannel(socket);
}

void run_stress_test(void *socket) {
  printf("\nRunning stress tests\n");
  pthread_t t1;
  pthread_create(&t1, NULL, (void *)&stress1, socket);
  pthread_detach(t1);
}

void onIceCallback(struct IceCandidate_C ice_cand) {
}

void custom_close1(int pid) {
  printf("\nCLOSE1 at %d of %d\n", getpid(), pid);
}

void onStringMsg1(int pid, const char* message) {
  /*printf("\nMSG1: %s\n", message);*/
}
void onDCCallback(int pid, void *socket, cb_event_loop* cb_loop) {
  printf("\n=sock1===Got datachannel!=======%d\n", getpid());
  //printf("\nCB loop process: %p\n", cb_loop);
  SetOnClosedCallback(pid, cb_loop, custom_close1);
  SetOnStringMsgCallback(pid, cb_loop, onStringMsg1);
}

void custom_close2(int pid) {
  printf("\nCLOSE2 at %d of %d\n", getpid(), pid);
}

void onStringMsg2(int pid, const char* message) {
  /*printf("\nMSG2: %s\n", message);*/
}
void onDCCallback1(int pid, void* socket, cb_event_loop* cb_loop) {
  printf("\n=sock2===Got datachannel!=======%d\n", getpid());
  SetOnClosedCallback(pid, cb_loop, custom_close2);
  SetOnStringMsgCallback(pid, cb_loop, onStringMsg2);
  closeDataChannel(socket);
  //run_stress_test(socket); // spawns a thread as nothing should block in callbacks
}

int main() {
  bool running = false;

  struct RTCIceServer_C rtc_ice;
  rtc_ice.hostname = "stun3.l.google.com";
  rtc_ice.port = 19302;
  struct RTCConfiguration_C rtc_conf;
  GArray* ice_servers;
  ice_servers = g_array_new(false, false, sizeof(rtc_ice));
  g_array_append_val(ice_servers, rtc_ice);
  rtc_conf.ice_ufrag = NULL;
  rtc_conf.ice_pwd = NULL;
  rtc_conf.ice_servers = ice_servers;
  rtc_conf.ice_port_range1 = rtc_conf.ice_port_range2 = 0;
  struct RTCIceServer_C rtc_ice1;
  rtc_ice1.hostname = "stun3.l.google.com";
  rtc_ice1.port = 19302;
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
  pc_info_ret1.socket = pc_info_ret2.socket = NULL;
  pc_info_ret1 = newPeerConnection(rtc_conf, onIceCallback, onDCCallback1, cb_loop);
  pc_info_ret2 = newPeerConnection(rtc_conf1, onIceCallback, onDCCallback, cb_loop);

  sock1 = pc_info_ret1.socket;
  sock2 = pc_info_ret2.socket;

  ParseOffer(sock1, ""); //trigger ICE
  ParseOffer(sock2, ""); // ""

  CreateDataChannel(sock1, "testchannel", "", 0x00, 0);

  printf("\n====================++++%d++\n", getpid());
  char* offer = GenerateOffer(sock1);
  ParseOffer(sock2, offer);
  free(offer);
  char* answer = GenerateAnswer(sock2);
  ParseOffer(sock1, answer);
  free(answer);
  processWait();
  return 0;
}
