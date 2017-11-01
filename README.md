After a clone, do:

* `git submodule init && git submodule update`
* `docker build -f Dockerfile-debian.armv7 -t librtcdcpp .` for ARMv7
* `docker run -it librtcdcpp:latest`
* Inside the container, `cd python/` and `./build.sh`
* `export LD_LIBRARY_PATH=../` (Since our built .so file will be in project root)
* Run the stress test script. See help: `python3 stress-test.py --help`

Note: Each time the library is made to create new 'peers', an IPC socket file will be created which has the path "/tmp/librtcdcpp{pid}". If not closed properly, these files will be left back.
Do `rm /tmp/librtcdcpp*` to make sure the inodes don't get full.

## Signalling server

From project root, 

* Run the centrifugo real-time messaging server container: `docker run --ulimit nofile=65536:65536 -v ./:/centrifugo -p 8000:8000 centrifugo/centrifugo centrifugo -c cent_config.json`

* Set env var `CENTRIFUGO_SERVER` to hostname:ip of the server started from above. (else it will use the default "localhost:8000")

* Install dependencies: `pip3 install cent centrifuge-python`

* Run `python3 python/peer.py` on the nodes (Python 3.5+ needed). Peer.py assigns a random UUID and you can 'call' any other peer just by typing in their UUID.
