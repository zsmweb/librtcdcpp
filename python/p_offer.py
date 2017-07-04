from pyrtcdcpp import RTCConf, PeerConnection
from pprint import pprint
from time import sleep
from base64 import b64encode, b64decode

got_dc = False

### CALLBACKS ###
def onIceCallback(ice):
    pass # We don't want trickle ICE now

def onMessage(msg):
    print("\nMESSAGE: " + msg + "\n")

def gotDC(dc):
    global got_dc
    got_dc = True
    dc.SetOnStringMsgCallback(onMessage)
    print("\n=======Got DC=======\n")

rtc_conf = RTCConf([("stun3.l.google.com", 19302)]) # Config/setting for STUN server
pc1 = PeerConnection(rtc_conf, onIceCallback, gotDC)
pc1.ParseOffer('') # This is to trigger the collection of candidates in sdp (we want a non trickle way to connect)

# pc1 generates the offer, so we create the datachannel object
dataChan = pc1.CreateDataChannel("test")

# Handshake part. Give our offer, take the answer
offer = pc1.GenerateOffer()
offer = bytes(offer, 'utf-8')
print("Offer: ", b64encode(offer).decode('utf-8'))

answer = b64decode(input("Answer:"))
pc1.ParseOffer(answer.decode('utf-8'))

while True:
    if got_dc:
        msg = input("Enter msg: ")
        dataChan.SendString(msg)
    sleep(1)
