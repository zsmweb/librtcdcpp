from pyrtcdcpp import RTCConf, PeerConnection
from pprint import pprint
from time import sleep
from base64 import b64encode, b64decode

got_dc = False

### CALLBACKS ###
class Peer(PeerConnection):
    def onCandidate(self, ice):
        pass # We don't want trickle ICE now

    def onMessage(self, msg):
        #print("\nMESSAGE: " + msg + "\n")
        pass

    def onChannel(self, dc):
        global dataChan
        global got_dc
        dataChan = dc
        got_dc = True
        print("\n=======Got DC=======\n")

    def onClose(self):
        print("DC Closed")

pc1 = Peer()
pc1.ParseOffer('') # This is to trigger the collection of candidates in sdp (non trickle way to connect)

# Handshake part. Give our answer, take the offer
answer = pc1.GenerateAnswer()
answer = bytes(answer, 'utf-8')
print("Answer: ", b64encode(answer).decode('utf-8'))

offer = b64decode(input("Offer:"))
pc1.ParseOffer(offer.decode('utf-8'))

while True:
    if got_dc:
        #msg = input("Enter msg: ")
        #dataChan.SendString(msg)
        pass
    sleep(1)
