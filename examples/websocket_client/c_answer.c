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

    cb_event_loop* cb_loop;
    cb_loop = init_cb_event_loop();

	void onIceCallback(struct IceCandidate_C ice_cand) {}
	struct DataChannel* dc;
	void onDCCallback(struct DataChannel* dc) {
		printf("\n========Got datachannel!=========\n");
	}
	pc_info pc_info_ret1;
	pc_info_ret1 = newPeerConnection(rtc_conf, onIceCallback, onDCCallback, cb_loop);
	void* sock1 = pc_info_ret1.socket;
	ParseOffer(sock1, ""); // Leads to ICE candidates being saved in next Generate SDP call
  char* answer = GenerateAnswer(sock1);
  gchar* answer_e = g_base64_encode(answer, strlen(answer));
  printf("\nAnswer:\n%s\n", answer_e);
  printf("\n\nEnter offer SDP:\n");
  gchar *received_sdp = getlines();
  gsize decoded_sdp_len;
  received_sdp = g_base64_decode(received_sdp, &decoded_sdp_len);
  ParseOffer(sock1, received_sdp);
  free(answer);
	while(1) {
		usleep(1);
	}
	return 0;
}
