#include <rtcdcpp/librtcdcpp.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
//#include "/usr/include/linux/fcntl.h"

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


	void onIceCallback(struct IceCandidate_C ice_cand) { }

	struct DataChannel* dc;

	void onDCCallback(dc) {
		printf("\n========Got datachannel!=========\n");
	}

  struct f_descriptors fdes1, fdes2;
	
	fdes1 = newPeerConnection(rtc_conf, onIceCallback, onDCCallback);
	fdes2 = newPeerConnection(rtc_conf1, onIceCallback, onDCCallback);
  //CreateDataChannel(pc1, "testchannel", "");

  int repeat = 0;
  while(repeat < 1) {
  printf("\n========================\n");
  
  //int pipe_size = fcntl(fdes1.first, F_GETPIPE_SZ);
  //printf("\nPipe size of fdes1 read end:%d\n", pipe_size);
  
  usleep(1000000);
    char* offer = GenerateOffer(fdes1);
    //gchar* offer_e = g_base64_encode(offer, strlen(offer));
    //printf("\nOffer:\n%s\n", offer_e);
    //printf("\n\nEnter answer SDP:\n");
    //gchar *received_sdp = getlines();
    //gchar *decoded_sdp_len;
    //received_sdp = g_base64_decode(received_sdp, &decoded_sdp_len);
    //ParseOffer(pc, received_sdp);
    
    //ParseOffer(pc2, offer);
    //char* answer = GenerateAnswer(pc2);
    //ParseOffer(pc1, answer);
    
    printf("\nOffer: %s", offer);
    //free(offer);
    //free(answer);
    repeat += 1;
    if (repeat == 1) {
      //printf("\n Let's do that exact dance again to get ICE candidates exchanged the non trickle way\n");
    }
  }
	while(1) {
		usleep(1);
	}
	return 0;
}
