
#pragma once

#if defined WIN32 || defined WIN64
#include <Windows.h>
#include <winioctl.h>
#else
#include "mm_type.h"
#endif

class ClientUSB 
{
protected:
	BOOL          m_connected;

public:
	ClientUSB() {};
	virtual ~ClientUSB() {};

	BOOL IsConnected() 
	{
		return   m_connected;
	}

	virtual BOOL   GetStatus(USHORT* status) = 0;
	virtual BOOL   GetEDIDSize(USHORT* size) = 0;
	virtual BOOL   GetEDIDData(UCHAR *buff,int bank) = 0;
	virtual BOOL   GetTargetVersion(char *data,int len) = 0;
	virtual BOOL   SetResolution(ULONG width,ULONG height) = 0;
	virtual BOOL   SetQuantTable(UCHAR* table,ULONG len,USHORT index) = 0;
	virtual BOOL   SetCommand(USHORT cmd) = 0;
	virtual BOOL   Write(UCHAR* data,ULONG SendLen,ULONG* ActLen) = 0;
	virtual USHORT getUSBVersion() = 0;
	virtual BOOL   SetISPmode() = 0; // ben.tsai //
	virtual BOOL   SetReConfig(USHORT flag) = 0;
	virtual BOOL   GetPowerStatus(USHORT* status) = 0;

	virtual void   ResetDevice() = 0;
private:
};


