from pyrtcdcpp import RTCConf, PeerConnection
from pprint import pprint
from time import sleep
from base64 import b64encode, b64decode
import time

got_dc = False

### CALLBACKS ###
class Peer(PeerConnection):
    def onCandidate(self, ice):
        pass # We don't want trickle ICE now

    def onMessage(self, msg):
        print("\nMESSAGE: " + msg + "\n")

    def onChannel(self, dc):
        global got_dc
        got_dc = True
        print("\n=======Got DC=======\n")

    def onClose(self):
        print("DC Closed")

def stress(dc):
   i = 1
   count = 0
   global running
   max_count = 2000
   byte_count = 0
   j = 32 # kB
   i = j * 1024
   print("===Testing fixed payloads of", j, "kB", max_count, "times===");
   time_start = time.time()
   while (running == True) and (count < max_count):
       count += 1
       try:
           dc.SendString('A' * i)
       except:
           print("ERR")
           break
   running = False
   time_end = time.time()
   total_data = (count * j) / 1024 # in MB
   total_time = time_end - time_start
   print("Total data sent:", total_data, "MB")
   print("Time taken in seconds:", total_time)
   print("Data rate:", total_data / total_time, "MB/s")


pc1 = Peer()
pc1.ParseOffer('') # This is to trigger the collection of candidates in sdp (we want a non trickle way to connect)

# pc1 generates the offer, so we create the datachannel object
dataChan = pc1.CreateDataChannel("test")

# Handshake part. Give our offer, take the answer
offer = pc1.GenerateOffer()
offer = bytes(offer, 'utf-8')
print("Offer: ", b64encode(offer).decode('utf-8'))

answer = b64decode(input("Answer:"))
pc1.ParseOffer(answer.decode('utf-8'))
running = True

while running == True:
    if got_dc:
        #msg = input("Enter msg: ")
        #dataChan.SendString(msg)
        stress(dataChan)
    sleep(1)
