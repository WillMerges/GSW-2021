### Make libraries in subdirs ###

all: build copy

build:
	-$(MAKE) -C ldms all
	-$(MAKE) -C vcm all
	-$(MAKE) -C dls all

copy:
	rm -rf bin || true
	mkdir bin
	-cp */*.so bin/


clean:
	-$(MAKE) -C ldms clean
	-$(MAKE) -C vcm clean
	-$(MAKE) -C dls clean
	rm -r bin
