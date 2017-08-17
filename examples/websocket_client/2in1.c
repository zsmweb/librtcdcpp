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

void stress1(DataChannel *dc) {
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
      if (_SendString(dc, test_str) == 0) {
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
    
    //std::cout << "\nWaiting " << wait_1 << " seconds.\n";
   // usleep(wait_1 * 1000000);
    /*
    std::cout << "===Testing throughput using single char spam===\n";
    while (running && count < 419) {
      count += 1;
      std::string test_str((size_t) i, 'A');
      try {
        //usleep(300000);
        if (count == 419) { gdbbreak = 1; }
        dc->SendString(test_str);
      } catch(std::runtime_error& e) {
        std::cout << "BROKE at count: " << count << "\n";
        count--;
        break;
      }
    }
    */
    //printf("\n%d bytes sent. \n\n", count);
    printf("Incremental throughput stops at: %d\n", inc_stop);
    printf("Single char spam stops at count: %d\n", count);
    printf("TOTAL successful send_string calls: %d\n", inc_stop + count);
    printf("TOTAL data sent: %d MB\n", bytecount1 / (1024 * 1024));
    printf("TOTAL time taken: %d seconds\n", elapsed_seconds);
    printf("Data rate: %d MB/s\n", (bytecount1 / elapsed_seconds) / (1024 * 1024));
    close_wait = 3;
    printf("\nWaiting %d seconds before closing DC.\n", close_wait);
    usleep(close_wait * 1000000); //5s
    _closeDataChannel(dc);
}

void run_stress_test(DataChannel* dc) {
    printf("\nRunning stress tests\n");
    pthread_t t1;
    pthread_create(&t1, NULL, (void *)&stress1, dc);
    pthread_detach(t1);
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

	void onDCCallback(struct DataChannel* dc, void *socket) {
		printf("\n=sock1===Got datachannel!=======\n", getpid());
	}
  void onDCCallback1(struct DataChannel* dc, void* socket) {
		printf("\n=sock2===Got datachannel!=======\n", getpid());
    run_stress_test(dc); // spawns a thread as nothing should block in callbacks
	}
  void* sock1;
  void* sock2;
	
	sock1 = newPeerConnection(rtc_conf, onIceCallback, onDCCallback);
	sock2 = newPeerConnection(rtc_conf1, onIceCallback, onDCCallback1);
  ParseOffer(sock1, ""); //trigger ICE
  ParseOffer(sock2, ""); // ""

  CreateDataChannel(sock1, "testchannel", "");

  int repeat = 0;
  while(repeat < 1) {
  printf("\n========================\n");
  
    char* offer = GenerateOffer(sock1);
    ParseOffer(sock2, offer);
    char* answer = GenerateAnswer(sock2);
    ParseOffer(sock1, answer);
    break;
  }
  processWait();
	return 0;
}
