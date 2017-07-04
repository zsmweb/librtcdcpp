from pyrtcdcpp import RTCConf, PeerConnection
from pprint import pprint
from time import sleep
from base64 import b64encode, b64decode

got_dc = False

### CALLBACKS ###
def onIceCallback(ice):
    pass # as-is

def onMessage(msg):
    print("\nMESSAGE: " + msg + "\n")

def gotDC(dc):
    global dataChan
    global got_dc
    dataChan = dc
    dc.SetOnStringMsgCallback(onMessage)
    print("\n=======Got DC=======\n")
    got_dc = True

rtc_conf = RTCConf([("stun3.l.google.com", 19302)])
pc1 = PeerConnection(rtc_conf, onIceCallback, gotDC)

pc1.ParseOffer('') # This is to trigger the collection of candidates in sdp (non trickle way to connect)

# Handshake part. Give our answer, take the offer
answer = pc1.GenerateAnswer()
answer = bytes(answer, 'utf-8')
print("Answer: ", b64encode(answer).decode('utf-8'))

offer = b64decode(input("Offer:"))
pc1.ParseOffer(offer.decode('utf-8'))

while True:
    if got_dc:
        msg = input("Enter msg: ")
        dataChan.SendString(msg)
    sleep(1)
