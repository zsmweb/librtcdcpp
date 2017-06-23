#include <rtcdcpp/librtcdcpp.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
// extern C {

int main() {
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

  struct IceCandidate_C ice_cand; 
  
  void onIceCallback(ice_cand) {
    printf("\n Ice candidate cb\n");
  }

  struct DataChannel* dc; //heap init?
  
  void onDCCallback(dc) {
    printf("\n DC cb\n");
  }

  struct PeerConnection* pc = newPeerConnection(rtc_conf, onIceCallback, onDCCallback);
  printf("\nOffer: %s\n", GenerateOffer(pc));
  
  while(1) {
    usleep(1);
  }
  return 0;
}
