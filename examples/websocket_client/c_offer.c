#include <rtcdcpp/librtcdcpp.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Custom multiline stdin scanning function */
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
/* end of custom multiline stdin scanning function */

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

	void onIceCallback(struct IceCandidate_C ice_cand) { }

	struct DataChannel* dc;

	void onDCCallback(struct DataChannel* dc) {
		printf("\n========Got datachannel!=========\n");
	}

	void* sock1;
	sock1 = newPeerConnection(rtc_conf, onIceCallback, onDCCallback);
	ParseOffer(sock1, ""); // Leads to ICE candidates being saved in next Generate SDP call
  CreateDataChannel(sock1, "testchannel", ""); // This is the offer client, so it should create a datachannel object
  char* offer = GenerateOffer(sock1);
  gchar* offer_e = g_base64_encode(offer, strlen(offer));
  printf("\nOffer:\n%s\n", offer_e);
  printf("\n\nEnter answer SDP:\n");
  gchar *received_sdp = getlines();
  gsize decoded_sdp_len;
  received_sdp = g_base64_decode(received_sdp, &decoded_sdp_len);
  ParseOffer(sock1, received_sdp);
  free(offer);
	while(1) {
		usleep(1);
	}
	return 0;
}
