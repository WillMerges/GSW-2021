# Make all process subdirs

all:
	-$(MAKE) -C view_log all
	-$(MAKE) -C mem_view all
	-$(MAKE) -C val_view all
	-$(MAKE) -C InfluxDB all
	-$(MAKE) -C map all
	-$(MAKE) -C voice all

clean:
	-$(MAKE) -C view_log clean
	-$(MAKE) -C mem_view clean
	-$(MAKE) -C val_view clean
	-$(MAKE) -C InfluxDB clean
	-$(MAKE) -C map clean
	-$(MAKE) -C voice clean
