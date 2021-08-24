### Make libraries in subdirs ###

all: build copy

build:
	-$(MAKE) -C vcm all
	-$(MAKE) -C dls all
	-$(MAKE) -C shm all
	-$(MAKE) -C nm all
	-$(MAKE) -C convert all
	-$(MAKE) -C telemetry all

copy:
	rm -rf bin || true > /dev/null
	mkdir bin
	-cp */*.so bin/


clean:
	-$(MAKE) -C vcm clean
	-$(MAKE) -C dls clean
	-$(MAKE) -C shm clean
	-$(MAKE) -C nm clean
	-$(MAKE) -C convert clean
	-$(MAKE) -C telemetry clean
	rm -r bin
