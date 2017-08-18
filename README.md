After a clone, do:

* `git submodule init && git submodule update`
* `docker build -f Dockerfile.amd64 -t librtcdcpp .` on AMD64/x86. `docker build -f Dockerfile.armv7 .` for ARMv7
* `docker run -it librtcdcpp:latest`
* Inside the container, `cd python/` and `./build.sh`
* Run the stress test script. See help: `python stress-test.py --help`
