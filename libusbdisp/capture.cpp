
#include "capture.h"

Capture::Capture()
{
    printf("Capture::Capture()\n");

    pEncodeEngine = new AndroidEngine();

    init_event(&sync_event); // test
};

Capture::~Capture()
{
    pEncodeEngine->DeviceReset();

    /*if( pEncodeEngine )
    {
        delete pEncodeEngine;
        pEncodeEngine = NULL;
    };*/

    uninit_event(&sync_event);

    printf("Capture::~Capture()\n");
};

bool Capture::CreateDevice(int fd)
{
    if( pEncodeEngine->CreateDevice(fd) ) {
        printf("Capture::CreateDevice(fd), OK\n");
        return pEncodeEngine->Start();
    } else {
        printf("Capture::CreateDevice(%d), Fail\n",fd);
        return false;
    }
};

bool Capture::CreateDevice(libusb_device *dev)
{
    if( pEncodeEngine->CreateDevice(dev) ) {
        printf("Capture::CreateDevice(), ok\n");
        return pEncodeEngine->Start();
    } else {
        printf("Capture::CreateDevice(), Fail\n");
        return false;
    }
};

bool Capture::SetDevResolution(int width, int height) {
    printf("Capture::SetDevResolution(int width, int height), OK\n");
    return pEncodeEngine->SetDevResolution(width, height);
}
