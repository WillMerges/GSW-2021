# Make all db subdirs

all:
	-$(MAKE) -C fwd_influx all
	-$(MAKE) -C run_influx all
	-$(MAKE) -C grafana all

clean:
	-$(MAKE) -C fwd_influx clean
	-$(MAKE) -C run_influx clean
	-$(MAKE) -C grafana clean
