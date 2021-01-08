# Make all process subdirs

all:
	-$(MAKE) -C lib all
	-$(MAKE) -C proc all
	-$(MAKE) -C app all

clean:
	-$(MAKE) -C lib clean
	-$(MAKE) -C proc clean
	-$(MAKE) -C app clean
