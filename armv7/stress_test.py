from pyrtcdcpp import RTCConf, PeerConnection, processWait
from pprint import pprint
from threading import Thread
from time import sleep, time
import argparse

dc_stress_flag = False
parser = argparse.ArgumentParser()
parser.add_argument("--size", help="Size in kilobytes of each payload. Default is 32 kB.", default=32, type=int)
parser.add_argument("--count", help="Number of times each payload gets sent. Default is 2000.", default=2000, type=int)
parser.add_argument("--p2p-pairs", help="Number of p2p connections to make. Default is 2 (4 peers).", default=2, type=int)
args = parser.parse_args()

def stress(dc):
   i = 1
   count = 0
   global running
   max_count = args.count
   byte_count = 0
   j = args.size # kB
   i = j * 1024
   print("===Testing fixed payloads of", j, "kB", max_count, "times===");
   time_start = time()
   while (count < max_count):
       count += 1
       try:
           dc.SendString('A' * i)
       except:
           print("ERR")
           break
   running = False
   time_end = time()
   total_data = (count * j) / 1024 # in MB
   total_time = time_end - time_start
   print("Total data sent:", total_data, "MB")
   print("Time taken in seconds:", total_time)
   print("Data rate:", total_data / total_time, "MB/s")
   dc.Close()

class Peer1(PeerConnection):
    def onCandidate(self, ice):
        pass # We don't want trickle ICE now

    def onMessage(self, msg):
        #print("\n[PC1]MESSAGE: " + msg + "\n")
        pass

    def onChannel(self, dc):
        print("\n=======PC1 Got DC=======\n")

    def onClose(self):
        print("DC1 Closed")

class Peer2(PeerConnection):
    def onCandidate(self, ice):
        pass # We don't want trickle ICE now

    def onMessage(self, msg):
        #print("\n[PC2]MESSAGE: " + msg + "\n")
        pass

    def onChannel(self, dc):
        print("\n=======PC2 Got DC=======")
        Thread(None, stress, "Stress_test_t", (dc,)).start()

    def onClose(self):
        print("DC2 Closed")

for i in range(0, args.p2p_pairs):
    pc1 = Peer1()
    pc2 = Peer2()
    
    pc1.CreateDataChannel("test")

    pc1.ParseOffer('') # To make offer have ice candidates
    offer = pc1.GenerateOffer()
    pc2.ParseOffer(offer)

    answer = pc2.GenerateAnswer()
    pc2.ParseOffer('')
    pc1.ParseOffer(answer)

processWait()
