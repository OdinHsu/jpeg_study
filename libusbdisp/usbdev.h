#ifndef USB_DEV_H
#define USB_DEV_H

#include "ClientUSB.h"
#include <libusb-1.0/libusb.h>

class usbdev : public ClientUSB
{
public:
	usbdev(int fd);
	usbdev(libusb_device *dev);
	virtual ~usbdev();

	void SetHandle(libusb_device_handle *handle);
	libusb_device_handle *GetHandle(){ return m_handle; };
	void SetEndpoint(short interface,int address);
	void SetbcdUSB(short bcdUSB);
private:
	short  m_interface;
	short  m_address;
	int    m_bcdUSB;
	unsigned long m_timeout;

	libusb_device *m_dev;
	libusb_device_handle *m_handle;

    int    m_fd;

public:
	virtual BOOL   GetStatus(USHORT* status);
	virtual BOOL   GetEDIDSize(USHORT* size);
	virtual BOOL   GetEDIDData(UCHAR *buff,int bank);
	virtual BOOL   GetTargetVersion(char *data,int len);
	virtual BOOL   SetResolution(ULONG width,ULONG height);
	virtual BOOL   SetQuantTable(UCHAR* table,ULONG len,USHORT index);
	virtual BOOL   SetCommand(USHORT cmd);
	virtual BOOL   Write(UCHAR* data,ULONG SendLen,ULONG* ActLen);
	virtual USHORT getUSBVersion();
	virtual BOOL   SetISPmode(); // ben.tsai //
	virtual BOOL   SetReConfig(USHORT flag);
	virtual BOOL   GetPowerStatus(USHORT* status);

	virtual void   ResetDevice();

};

#define VID 0x04FC
#define PID 0x2103
#define INTERFACE 2
#define EPADDR 2
#endif
