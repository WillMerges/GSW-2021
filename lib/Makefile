### Make libraries in subdirs ###

all: build copy python

build:
	-$(MAKE) -C vcm all
	-$(MAKE) -C dls all
	-$(MAKE) -C shm all
	-$(MAKE) -C nm all
	-$(MAKE) -C convert all
	-$(MAKE) -C telemetry all
	-$(MAKE) -C clock all
	-$(MAKE) -C vlock all
	-$(MAKE) -C trigger all

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
	-$(MAKE) -C python clean
	-$(MAKE) -C clock clean
	-$(MAKE) -C vlock clean 
	-$(MAKE) -C trigger clean 
	rm -r bin

# build special library used by Python ctypes
# library is every lib object module in a single lib with additional c bindings
python: build copy
	-$(MAKE) -C python
	g++ -shared python/bindings.o vcm/src/*.o dls/src/*.o nm/src/*.o shm/src/*.o convert/src/*.o telemetry/src/*.o clock/src/*.o vlock/src/*.o trigger/src/*.o -o bin/libpython.so -lrt
