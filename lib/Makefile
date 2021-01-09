### Make libraries in subdirs ###

all: build copy

build:
	-$(MAKE) -C ldms all
	-$(MAKE) -C vcm all
	-$(MAKE) -C dls all
	-$(MAKE) -C shm all
	-$(MAKE) -C nm all

copy:
	rm -rf bin || true > /dev/null
	mkdir bin
	-cp */*.so bin/


clean:
	-$(MAKE) -C ldms clean
	-$(MAKE) -C vcm clean
	-$(MAKE) -C dls clean
	-$(MAKE) -C shm clean
	-$(MAKE) -C nm clean
	rm -r bin
