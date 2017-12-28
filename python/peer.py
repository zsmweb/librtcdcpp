from cent import Client, generate_token  # Cent.Client for publishing.

import signal

# Websocket centrifugo client (centrifuge.Client) for subscribing
from centrifuge import Client as clientpy

from centrifuge import Credentials
import sys
from uuid import uuid4
import json
import time
from time import sleep
from pyrtcdcpp import RTCConf, PeerConnection, \
    processWait, exitter, init_cb_event_loop, DataChannel
import asyncio
import os

signalling_server = os.getenv('CENTRIFUGO_SERVER', "localhost:8000")
secret_key = "secret"  # or use --insecure_api ?


class InvalidUUID(Exception):
    pass


global state

# Denotes "DC connected" state. 0 being disconnected and 1 being connected
state = 0
global dc
dc = None


class Peer(PeerConnection):
    def onCandidate(self, ice):
        pass  # We don't want trickle ICE now

    def onMessage(self, msg):
        print("\nReceived message: " + msg)
        print("Send msg:-")

    def onChannel(self, dcn):
        global dc
        global state
        state = 1
        dc = dcn
        print("\n=======Got DC=======\n")
        print("\nPress enter to get the prompt.")

    def onClose(self):
        print("DC Closed")
        os._exit(0)


url = "http://" + signalling_server
global cent_client
cent_client = Client(url, secret_key, timeout=1)  # Cent


async def run(evt_loop, user):
    global peer
    global dc
    dc = None
    peer = None
    timestamp = str(int(time.time()))
    info = json.dumps({"client_version": "0.1"})
    token = generate_token(secret_key, user, timestamp, info=info)

    async def input_validation(uinput):
        uinput = uinput[:-1]  # Remove '\n'
        global state
        global peer

        if state == 1:
            return

        if uinput not in cent_client.channels() or uinput == user:
            raise InvalidUUID
        else:
            peer = Peer(evt_loop)
            peer.ParseOffer('')  # gather candidates
            peer.CreateDataChannel("test", evt_loop, None,
                                   DataChannel.DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT_UNORDERED, 2000)
            offer_sdp = peer.GenerateOffer()
            payload = {
                "sdp_cand": offer_sdp,
                "sdp_type": "offer",
                "from": user
            }
            if cent_client.publish(uinput, payload) is None:
                pass
            return peer

    async def uuid_input_loop(peer):
        global state
        while True:
            if state == 0:
                prompt_user(user)
            else:
                break
            user_input = await q.get()
            try:
                peer = await input_validation(user_input)
            except InvalidUUID:
                print("\nInvalid UUID!")
    credentials = Credentials(user, timestamp, info, token)
    address = "ws://" + signalling_server + "/connection/websocket"

    async def message_handler(**kwargs):
        rsc = kwargs['data']
        global peer
        try:
            sdp = rsc['sdp_cand']
            sdp_type = rsc['sdp_type']
            from_client = rsc['from']
        except KeyError:
            print("key error")
            return
        if sdp_type == "offer":
            print("Got offer")
            peer = Peer(evt_loop)
            peer.ParseOffer('')  # cand
            peer.ParseOffer(sdp)
            answer = peer.GenerateAnswer()
            payload = {
                "sdp_cand": answer,
                "sdp_type": "answer",
                "from": None  # No need for them to contact us anymore
            }
            cent_client.publish(from_client, payload)
        elif sdp_type == "answer":
            peer.ParseOffer(sdp)
            print("SDP answer has been parsed, trying to establish a connection...")

    async def join_handler(**kwargs):
        print("Join:", kwargs)

    async def leave_handler(**kwargs):
        print("Leave:", kwargs)

    async def error_handler(**kwargs):
        print("Sub error:", kwargs)

    async def dc_input_loop():
        # TODO: Clear queue?
        global dc
        while state == 1:
            print("Send msg:-")
            user_input = await q.get()
            dc.SendString(user_input)
        pass

    async def connection_handler(**kwargs):
        os.system('clear')
        print("Connected", kwargs)
        sub = await clientpy1.subscribe(
            user,  # The channel is our UUID
            on_message=message_handler,
            on_join=join_handler,
            on_leave=leave_handler,
            on_error=error_handler
        )
        await uuid_input_loop(peer)
        await dc_input_loop()

    async def disconnected_handler(**kwargs):
        print("Disconnected:", kwargs)
        if dc is None:
            os._exit(0)
        else:
            dc.Close()

    async def connect_error_handler(**kwargs):
        print("Error:", kwargs)

    clientpy1 = clientpy(
        address, credentials,
        on_connect=connection_handler,
        on_disconnect=disconnected_handler,
        on_error=connect_error_handler
    )

    print("Connecting to centrifugo signalling server...")
    await clientpy1.connect()


def prompt_user(user):
    print("My ID: ", user)
    print("Enter the ID to call:-")


def uuid_input_cb(q):
    asyncio.async(q.put(sys.stdin.readline()))


if __name__ == '__main__':
    def signal_handler(signal, frame):
        if dc is not None:
            dc.Close()
        cent_client.disconnect(user)
    signal.signal(signal.SIGINT, signal_handler)

    evt_loop = init_cb_event_loop()
    user = "psl-" + uuid4().hex[:3]
    q = asyncio.Queue()
    loop = asyncio.get_event_loop()
    loop.add_reader(sys.stdin, uuid_input_cb, q)
    asyncio.ensure_future(run(evt_loop, user))
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        print("interrupted")
    finally:
        loop.close()
