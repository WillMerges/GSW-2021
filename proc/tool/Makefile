# Make all process subdirs

all:
	-$(MAKE) -C shmtest all
	-$(MAKE) -C mqueue_test all
	-$(MAKE) -C vcm_test all
	-$(MAKE) -C log2csv all
	-$(MAKE) -C mdns_publish all
	-$(MAKE) -C vlock_test all

clean:
	-$(MAKE) -C shmtest clean
	-$(MAKE) -C mqueue_test clean
	-$(MAKE) -C vcm_test clean
	-$(MAKE) -C log2csv clean
	-$(MAKE) -C mdns_publish clean
	-$(MAKE) -C vlock_test clean
