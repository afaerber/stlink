all: stlink

.PHONY: test load-kext

-include config.mak

stlink: main.c stlink-libusb.c stlink-libusb.h stlink-cmd.c stlink.h bswap.h Makefile
	$(CC) -o $@ $(CPPFLAGS) -std=gnu99 main.c stlink-libusb.c stlink-cmd.c $(LDFLAGS) -lusb-1.0

test: stlink
	./stlink

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
