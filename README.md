# Introduction

This is a fork of the librtcdcpp project which was written in C++. Changes include a C wrapper which is used by the python wrapper (implemented using CFFI) and the ability to run multiple peer connections within the same process. A stress test script and a CLI python client is bundled.

## Quick start

After a clone, do:

* `git submodule init && git submodule update`
* `docker build -f Dockerfile-debian.armv7 -t librtcdcpp .` for ARMv7
> :warning: It may take several couple of hours to build the ARMv7 container, especially if it has matplotlib, numpy and its dependencies. Comment out [those lines](https://github.com/hamon-in/librtcdcpp/blob/8b8f373c828078c985824b45876b40c5b6648fcc/Dockerfile-debian.armv7#L22-#L24) to build a more lightweight container. The resulting container may not be able to run `stress_test.py`, but should be able to run the signalling client `peer.py`.
* `docker run -it librtcdcpp:latest`
> :warning: Make sure that the containers that hold the client *and* centrifugo server are configured for proper [networking](https://docs.docker.com/engine/userguide/networking/). You can set `--network=host` to let the containers use the host network's stack for easier networking. Otherwise it'll use the default docker bridge network.

### Stress test

Running the stress test script without arguments will run a series of tests, first of which is a sequential test for each packet sizes of 2, 4, 8 ... 64. Secondly, it will do a concurrent test (2 peers at a time) for each of those packet sizes. At the end of this, it will plot a graph using matplotlib.

* Inside the container, `cd python/` and `./build.sh`
* `export LD_LIBRARY_PATH=../` (Since our built .so file will be in project root)
* Run the stress test script. See help: `python3 stress-test.py --help`

> :recycle: Note: Each time the library is made to create new 'peers', an IPC socket file will be created which has the path "/tmp/librtcdcpp{pid}". If not closed properly, these files will be left back.
Do check for `/tmp/librtcdcpp*` files and `rm /tmp/librtcdcpp*` to make sure the inodes don't get full.

Stress test [results](https://github.com/hamon-in/librtcdcpp/wiki/Performance-evaluation-(AMD-A8-7410-CPU)#python-concurrent-test-protobuf--zmq) on AMD A8 7410 with the ZMQ/protocol buffers fixes-:

![](http://image.ibb.co/m3j2qm/AMD_librtcdcpp.png)
_\*concurrent test was done with 2 peers at a time with the same packet size for each packet size._

### Signalling server

From project root, on the host machine or a different machine on the network, set up the signalling server:

* Run the centrifugo real-time messaging server container: `docker run --ulimit nofile=65536:65536 -v ./:/centrifugo -p 8000:8000 centrifugo/centrifugo centrifugo -c cent_config.json`

### Signalling client

Go inside the debian container created from the quick start step above and do the following:

* Set env var `CENTRIFUGO_SERVER` to hostname:ip of the server started from above. (else it will use the default "localhost:8000")

* Do `export LD_LIBRARY_PATH=./` to let the client be aware of the `librtcdcpp.so` shared object that will be made in the project root. Modify as necessary depending on where you are, and relative to the where you have the .so file.

* Run `python3 python/peer.py` on the nodes (Python 3.5+ needed). Peer.py assigns a random UUID and you can 'call' any other peer just by typing in their UUID
