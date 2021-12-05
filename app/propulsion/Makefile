# Make all propulsion applications 

all:
	-$(MAKE) -C ec_cmd all
	-$(MAKE) -C ec_program all
	chmod +x ec_server.py

clean:
	-$(MAKE) -C ec_cmd clean
	-$(MAKE) -C ec_program clean
	chmod -x ec_server.py
