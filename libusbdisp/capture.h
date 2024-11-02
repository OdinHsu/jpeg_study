
#ifndef CAPTURE_H
#define CAPTURE_H

#include "ThreadBase.h"
#include "event.h"
#include "usbdev.h"
#include "EncodeEngine.h"

//namespace android{

class AndroidEngine : public EncodeEngine
{
public:
	AndroidEngine()
	{

	};

	virtual ~AndroidEngine()
	{

	};

	bool CreateDevice(int fd)
	{
		m_device = new usbdev(fd);
		return m_device->IsConnected();
	};

	bool CreateDevice(libusb_device *dev)
	{
		m_device = new usbdev(dev);
		return m_device->IsConnected();

	};

	bool SetDevResolution(int width, int height) {
        return m_device->SetResolution(width, height);
    };

};

class Capture
{
public:
	Capture();
	~Capture();

private:

	AndroidEngine *pEncodeEngine;

public:
	bool CreateDevice(int fd);
	bool CreateDevice(libusb_device *dev);
	inline void StartEncode(int width,int height,int pitch,unsigned char *framebuffer)
	{
	    pEncodeEngine->StartEncode(width,height,pitch,framebuffer);
	}
	bool SetDevResolution(int width, int height);
};


#endif
