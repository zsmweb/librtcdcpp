# Introduction

This is a fork of the librtcdcpp project which was written in C++. Changes include a C wrapper which is used by the python wrapper (implemented using CFFI) and the ability to run multiple peer connections within the same process. A stress test script and a CLI python client is bundled.

## Quick start

After a clone, do:

* `git submodule init && git submodule update`
* `docker build -f Dockerfile-debian.armv7 -t librtcdcpp .` for ARMv7
* `docker run -it librtcdcpp:latest`

### Stress test

* Inside the container, `cd python/` and `./build.sh`
* `export LD_LIBRARY_PATH=../` (Since our built .so file will be in project root)
* Run the stress test script. See help: `python3 stress-test.py --help`

Note: Each time the library is made to create new 'peers', an IPC socket file will be created which has the path "/tmp/librtcdcpp{pid}". If not closed properly, these files will be left back.
Do `rm /tmp/librtcdcpp*` to make sure the inodes don't get full.

Stress test results on AMD A8 7410 with the ZMQ/protocol buffers fixes-:

![](http://image.ibb.co/m3j2qm/AMD_librtcdcpp.png)

### Signalling server

From project root, on the host machine or a different machine on the network, set up the signalling server:

* Run the centrifugo real-time messaging server container: `docker run --ulimit nofile=65536:65536 -v ./:/centrifugo -p 8000:8000 centrifugo/centrifugo centrifugo -c cent_config.json`

### Signalling client

Go inside the debian container created from the quick start step above and do the following:

* Set env var `CENTRIFUGO_SERVER` to hostname:ip of the server started from above. (else it will use the default "localhost:8000")

* Run `python3 python/peer.py` on the nodes (Python 3.5+ needed). Peer.py assigns a random UUID and you can 'call' any other peer just by typing in their UUID.
