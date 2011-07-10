all: stlink-test

.PHONY: test load-kext

CFLAGS = -std=gnu99 -Wall -Werror
DGFLAGS = -MMD -MP -MT $@

-include config.mak

LIBSTLINK_SOURCES = stlink-libusb.c stlink-cmd.c

-include stlink-test.d

stlink-test: main.c $(addprefix libstlink/, $(LIBSTLINK_SOURCES)) Makefile
	$(CC) -o $@ $(CPPFLAGS) -I. -Ilibstlink $(DGFLAGS) $(CFLAGS) main.c $(addprefix libstlink/,stlink-libusb.c stlink-cmd.c) $(LDFLAGS) -lusb-1.0

test: stlink-test
	./stlink-test

DEST=/tmp
KEXT=STLink

load-kext:
	-sudo rm -rf $(DEST)/$(KEXT).kext
	mkdir -p $(DEST)/$(KEXT).kext/Contents
	cp darwin/Info.plist $(DEST)/$(KEXT).kext/Contents/
	sudo chown -R root:wheel $(DEST)/$(KEXT).kext
	sudo chmod -R 0644 $(DEST)/$(KEXT).kext
	sudo kextload -v $(DEST)/$(KEXT).kext
	sudo killall -HUP kextd
