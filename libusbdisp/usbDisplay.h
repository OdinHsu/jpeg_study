#ifndef USBDISPLAY_H
#define USBDISPLAY_H

#include <libusb-1.0/libusb.h>
class Capture;

class UsbDisplay
{
public:
    int jxInit(libusb_device *dev);

    void jxDelete();

    void jxStartEncode(char * in,int width,int height,int pitch);

    void jxSetResolution(int width,int height);
private:
    Capture *pCapture = NULL;
};
#endif