After a clone, do:

* `git submodule init && git submodule update`
* `docker build -f Dockerfile-debian.armv7 -t librtcdcpp .` for ARMv7
* `docker run -it librtcdcpp:latest`
* Inside the container, `cd python/` and `./build.sh`
* `export LD_LIBRARY_PATH=../` (Since our built .so file will be in project root)
* Run the stress test script. See help: `python3 stress-test.py --help`

Note: Each time the library is made to create new 'peers', an IPC socket file will be created which has the path "/tmp/librtcdcpp{pid}". If not closed properly, these files will be left back.
Do `rm /tmp/librtcdcpp*` to make sure the inodes don't get full.
