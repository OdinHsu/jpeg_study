.PHONY: dirs clean

SRC = *.c *.cpp **/*.c **/*.cpp

TEST = test/ut_rectangle.h test/ut_triangle.h test/ut_sort.h test/ut_mixed_shape.h \
				test/ut_iterator.h test/ut_visitor.h test/ut_string.h test/ut_scanner.h \
				test/ut_shape_builder.h test/ut_parser.h test/ut_filter.h
OBJ = libenc/simd/*.o

DEPS = evdi_ioctl.h
CFLAGS = -I/usr/local/include/libusb-1.0 -L/usr/local/lib -lusb-1.0 -I libenc/ \
		-I . -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib \
		-L/usr/local/lib -lX11 -lXext -lpng -lXrandr
LIBVER := 1.9.0
YEAR := 22
MON := $(shell date '+%m')
DAY := $(shell date '+%d')
PACK := deb
ARCH := $(shell uname -m)

ifeq ($(ARCH), x86_64)
	ASDEF = _SSE2_
	ARCH_NAME = amd64
else ifeq ($(ARCH), aarch64)
	ASDEF = _NEON_
	ARCH_NAME = arm64
endif

DEB_VERSION := $(YEAR)_$(MON)_$(DAY)_b_$(ARCH_NAME).deb #$(PACK) $(YEAR)_$(MON)_$(DAY)_b
RUN_VERSION := $(YEAR)_$(MON)_$(DAY)_b_$(ARCH_NAME).run 
#Version := $(YEAR).$(MON).$(DAY)
LIBABI := 0



PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib

all: dirs main
dts: libdisp dybin

main: ./main.c $(SRC) $(OBJ)
	g++ -o bin/usb-disp -I/usr/local/include/libusb-1.0 *.c *.cpp **/*.c **/*.cpp -L/usr/local/lib -lusb-1.0 -I libenc/ -I . libenc/simd/*.o -pthread -D _SSE2_ -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lpng -lXrandr 

jpegenc: clean dirs ./jpegenc.c 
	gcc -g -o bin/jpegenc jpegenc.c -lusb-1.0 -D JPGENC -D _SSE2_ -I libenc/ **/jpeg_enc.c libenc/simd/x86_64/j*.o

dybin: ./main.c $(SRC) $(OBJ)
	g++ main.c lib/libusbdisp.so -o bin/main_dynamic -I/usr/local/include/libusb-1.0  -lusb-1.0 -lX11 -lXext -lXrandr
	LD_LIBRARY_PATH=./lib bin/main_dynamic

dirs:
	mkdir -p bin
	mkdir -p obj
	mkdir -p lib

libdisp: clean dirs $(SRC)
	g++ -c -fPIC -I/usr/local/include/libusb-1.0 [!m]*.c *.cpp **/*.c **/*.cpp -L/usr/local/lib -lusb-1.0 -I libenc/ -I . -pthread -D $(ASDEF) -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lpng -lXrandr
	mv *.o obj/
	gcc -shared -Wl,-soname,libusbdisp.so.0 -o libusbdisp.so.0.0.0 obj/*.o libenc/simd/$(ARCH)/*.o
	cp libusbdisp.so.0.0.0 libusbdisp.so
	cp libusbdisp.so.0.0.0 libusbdisp.so.0
	mv libusbdisp* lib/

clean:
	rm -rf bin/* obj/* lib/* libusbdisp*

stat:
	wc -l src/* test/*.h test/*.cpp

env:
	rm -f /etc/udev/rules.d/90-my-rule.rules
	touch /etc/udev/rules.d/90-my-rule.rules
	echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"34c7\", ATTRS{idProduct}==\"2103\", MODE=\"0666\"">> /etc/udev/rules.d/90-my-rule.rules
	echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"34c7\", ATTRS{idProduct}==\"2104\", MODE=\"0666\"">> /etc/udev/rules.d/90-my-rule.rules
	echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"34c7\", ATTRS{idProduct}==\"2105\", MODE=\"0666\"">> /etc/udev/rules.d/90-my-rule.rules
	echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"34c7\", ATTRS{idProduct}==\"2113\", MODE=\"0666\"">> /etc/udev/rules.d/90-my-rule.rules
	echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"34c7\", ATTRS{idProduct}==\"2114\", MODE=\"0666\"">> /etc/udev/rules.d/90-my-rule.rules
	echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"34c7\", ATTRS{idProduct}==\"2115\", MODE=\"0666\"">> /etc/udev/rules.d/90-my-rule.rules
	rm -f /usr/share/X11/xorg.conf.d/20-evdidevice.conf
	touch /usr/share/X11/xorg.conf.d/20-evdidevice.conf
	echo "Section \"Device\"\n\tIdentifier \"UsbDisplay\"\n\tDriver \"modesetting\"\n\tOption \"PageFlip\" \"false\"\nEndSection">> /usr/share/X11/xorg.conf.d/20-evdidevice.conf

dep:
	./dep_check.sh
deb:
	bash ./dep_check.sh cp_stage
	sed -i 's/Standards-Version.*/Standards-Version: 1.0.0/' deb_install/com.racertech.usbdisp-1.0.0/debian/control
	sed -i 's/Architecture.*/Architecture: $(ARCH_NAME)/' deb_install/com.racertech.usbdisp-1.0.0/debian/control
	cp stage/bin/example deb_install/com.racertech.usbdisp-1.0.0/opt/apps/com.racertech.usbdisp/files/bin/usb-disp_bin
	chmod +x deb_install/com.racertech.usbdisp-1.0.0/opt/apps/com.racertech.usbdisp/files/bin/usb-*
	cp -P stage/lib/* deb_install/com.racertech.usbdisp-1.0.0/opt/apps/com.racertech.usbdisp/files/lib/
	rm -rf deb_install/com.racertech.usbdisp-1.0.0/opt/apps/com.racertech.usbdisp/files/xorg/
	unzip deb_install/resource/xorg.zip -d deb_install/com.racertech.usbdisp-1.0.0/opt/apps/com.racertech.usbdisp/files/
	rm -f deb_install/com.racertech.usbdisp_1.0.0-1_$(ARCH_NAME).*
	cd deb_install/com.racertech.usbdisp-1.0.0 && fakeroot dpkg-buildpackage -us -uc -b -tc
	rm -f deb_install/com.racertech.usbdisp_$(DEB_VERSION)
	cp -r deb_install/com.racertech.usbdisp_1.0.0-1_$(ARCH_NAME).deb deb_install/com.racertech.usbdisp_$(DEB_VERSION)
	rm -f deb_install/com.racertech.usbdisp_$(RUN_VERSION)
	sed -i "s/ARCH=.*/ARCH=$(ARCH)/g" deb_install/deb_install.sh
	cat deb_install/deb_install.sh deb_install/com.racertech.usbdisp_1.0.0-1_$(ARCH_NAME).deb > deb_install/com.racertech.usbdisp_$(RUN_VERSION)
	chmod +x deb_install/com.racertech.usbdisp_$(RUN_VERSION)
