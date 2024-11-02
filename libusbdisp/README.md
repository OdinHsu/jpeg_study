# Usb_display_Ubuntu

sudo g++ -o usb-disp -I/usr/local/include/libusb-1.0 *.c *.cpp **/*.c **/*.cpp -L/usr/local/lib -lusb-1.0 -I libenc/ -I . libenc/simd/*.o -pthread -D _SSE2_ -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lpng


git merge --no-ff dev

git log --pretty=format: '%h : %s' --graph