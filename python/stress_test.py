from pyrtcdcpp import RTCConf, PeerConnection, \
    processWait, exitter, init_cb_event_loop, DataChannel
from pprint import pprint
from threading import Thread
from time import sleep, time
import json
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt

dc_stress_flag = False
parser = argparse.ArgumentParser()
parser.add_argument("--size", help="Size in kilobytes of each payload. \
        Default is 32 kB.", default=32, type=int)
parser.add_argument("--count", help="Number of times each payload gets sent. \
        Default is 2000.", default=100, type=int)
parser.add_argument("--p2p-pairs", help="Number of p2p connections to make. \
        Default is 2 (4 peers).", default=2, type=int)
args = parser.parse_args()


def autolabel(rects, ax):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., 1.01 * height,
                '%d' % int(height),
                ha='center', va='bottom')


class graph():
    speed_for_packet = []  # List of (packet size, value)
    speed_for_packet_concurrent = {}  # Same, but under concurrent load

    def __init__(self):
        pass

    def add_speed(self, packet_size, speed):
        speed = round(speed, 2)
        print("Adding speed: ", speed)
        self.speed_for_packet.insert(0, (packet_size, speed))
        print(self.speed_for_packet)

    def add_concurrent_speed(self, packet_size, speed):
        print("Adding {} and speed {}".format(packet_size, speed))
        speed = round(speed, 2)
        try:
            old_speed = self.speed_for_packet_concurrent[packet_size]
            self.speed_for_packet_concurrent[packet_size] \
                = (old_speed + speed) / 2
        except KeyError:
            self.speed_for_packet_concurrent[packet_size] = speed

    def plot(self):
        N = len(self.speed_for_packet)
        fig, ax = plt.subplots()
        width = 0.15
        ind = np.arange(N)
        height_1 = [speed[1] for speed in self.speed_for_packet]
        height_2 = [speed for speed in
                    self.speed_for_packet_concurrent.values()]
        bottom_1 = [speed[0] for speed in self.speed_for_packet]
        rect1 = ax.bar(ind, height_1, width=width,
                       color='y', label='Sequential test')
        if len(height_2) > 0:
            N = len(self.speed_for_packet_concurrent)
            ind = np.arange(N)
            rect2 = ax.bar(ind + width, height_2, width=width, color='r',
                           label='Concurrent test')
            autolabel(rect2, ax)
        autolabel(rect1, ax)
        ax.set_ylabel('MB/s')
        ax.set_xlabel('Individual packet size (in KBs)')
        ax.set_xticks(ind)
        ax.set_xticklabels([speed[0] for speed in self.speed_for_packet])
        ax.legend()
        fig.tight_layout()
        plt.show()


def handshake_peers(evt_loop, pkt_size, graph_obj, concurrent=False):
    pc1 = Peer1(evt_loop)
    pc2 = Peer2(evt_loop)
    pc1.concurrent = concurrent
    pc2.pkt_size = pkt_size
    pc1.CreateDataChannel("test", evt_loop, None,
                          DataChannel.DATA_CHANNEL_RELIABLE, 3000000)
    pc2.graph_obj = graph_obj

    pc1.graph_obj = graph_obj

    pc1.ParseOffer('')  # To make offer have ice candidates
    pc2.ParseOffer('')
    offer = pc1.GenerateOffer()
    if offer is not None:
        pc2.ParseOffer(offer)
    else:
        return
    answer = pc2.GenerateAnswer()
    if answer is not None:
        pc1.ParseOffer(answer)


global pkt_size
pkt_size = None


def stress(dc, pc_obj):
    i = 1
    count = 0
    global running
    max_count = args.count
    byte_count = 0
    if pc_obj.pkt_size is None:
        j = args.size  # kB
    else:
        j = pc_obj.pkt_size
    i = j * 1024
    print("===Testing fixed payloads of", j, "kB", max_count, "times===")
    time_start = time()
    while (count < max_count):
        count += 1
        dc.SendString('A' * i)
    time_end = time()
    total_data = (count * j) / 1024  # in MB
    total_time = time_end - time_start
    last_payload = {'t': total_time, 'd': total_data, 'jj': j}
    dc.SendString('EOL' + json.dumps(last_payload))


class Peer1(PeerConnection):
    def onCandidate(self, ice):
        # print("Python peer1 got candidates: ", ice['candidate'])
        pass  # We don't want trickle ICE now

    def onMessage(self, msg):
        # print("\n[PC1]MESSAGE: " + msg + "\n")
        if (msg[:3] == 'EOL'):
            payload = json.loads(msg[3:])
            total_time = payload['t']
            total_data = payload['d']
            j = payload['jj']
            print("Total data sent:", total_data, "MB")
            print("Time taken in seconds:", total_time)
            print("Data rate:", total_data / total_time, "MB/s")
            if self.concurrent:
                self.graph_obj.add_concurrent_speed(j, total_data / total_time)
            else:
                self.graph_obj.add_speed(j, total_data / total_time)
            self.Close()  # dc Close

    def onChannel(self, dc):
        print("\n=======PC1 Got DC=======\n")

    def onClose(self):
        print("DC1 Closed")


class Peer2(PeerConnection):
    def onCandidate(self, ice):
        # print("Python peer2 got candidates: ", ice['candidate'])
        pass  # We don't want trickle ICE now

    def onMessage(self, msg):
        # print("\n[PC2]MESSAGE: " + msg + "\n")
        pass

    def onChannel(self, dc):
        print("\n=======PC2 Got DC=======")
        Thread(None, stress, "Stress_test_t", (dc, self)).start()

    def onClose(self):
        print("DC2 Closed")


stress_graph = graph()
evt_loop = init_cb_event_loop()  # 1 loop may not be so performant
if (len(sys.argv) > 1):
    for i in range(0, args.p2p_pairs):
        handshake_peers(evt_loop, args.size, stress_graph)
    processWait()
else:
    # Run a series of tests and plot em after test finishes
    print("##### Stress testing localhost IPC (single pair) ####")
    for i in range(1, 7):
        pkt_size = 2 ** i
        handshake_peers(evt_loop, pkt_size, stress_graph)
        processWait()

    concurrent_count = 2
    print("Stress testing localhost IPC in concurrent pairs \
            ({} at a time with same packet sizes)".format(concurrent_count))
    for packet_size in range(1, 7):
        for con_cnt in range(0, concurrent_count):
            pkt_size = 2 ** packet_size
            handshake_peers(evt_loop, pkt_size, stress_graph, True)
        processWait()
    stress_graph.plot()
