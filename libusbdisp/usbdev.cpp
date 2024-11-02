
#include <unistd.h>

//#include <binder/ProcessState.h>
#include "usbdev.h"
#include "debug_trace.h"

typedef struct {
	int Interface;
	int EndPoint;
} idtable_t;

static idtable_t ids[] = {
		{2,2},
		{0,1},
		{0,1}     //if necessary modify to {2,2}
};

// static int jni_init(libusb_device **device)
// {
//         libusb_device **list;
//         libusb_context *ctx;
//         struct libusb_device_descriptor dd;
//         int index = -1;
//         ssize_t cnt;
//         int status, i;

//         printf("%s ---- [xxxxx starting xxxxx]", __func__);
//         /*status = libusb_init2(&ctx, "/dev/bus/usb");
//         if (status != LIBUSB_SUCCESS)
//                 return status;
//         else if (ctx == NULL)
//                 return LIBUSB_ERROR_OTHER;*/

//         printf("%s ---- [xxxxx libusb_get_device_list xxxxx]", __func__);
//         cnt = libusb_get_device_list(ctx, &list);
//         for (i = 0; i < cnt; i++) {
//                 *device = list[i];
//                 libusb_get_device_descriptor(list[i], &dd);
//                 if( (dd.idVendor == 0x34c7) && ((dd.idProduct & 0xFFF0) == 0x8000 || dd.idProduct == 0x2104) ) {  //if necessary modify 0x8000 to 0x2100
//                         //index = dd.idProduct & 0x0F;
//                         index = 2;  //2020/04/10 Eddie
//                         printf("Detect : VID = 0x%0X/PID = 0x%0X  index=%d",dd.idVendor, dd.idProduct, index);
//                         break;
//                 }
//                 *device = NULL;
//         }
//         if (!*device) {
//         	if (ctx)
//         		libusb_exit(ctx);
//             return -1;
//         }

//         return index;
// }

usbdev::usbdev(int fd)
{/*
	struct 	libusb_device_descriptor desc;
	int 	rc;
	int     index = -1;
	libusb_device_handle *handle;
	int     interface,epaddress;

	m_fd = fd; // save fd

    index = jni_init(&m_dev);
	if(index<0){
		printf("%s ---- [libusb init failed!]", __func__);
		return;
    }

	printf("%s ---- [libusb_open2 fd=%08x]", __func__, fd);
	rc = libusb_open2(m_dev, &handle, fd);

	if (LIBUSB_SUCCESS != rc) {
		printf( "libusb: Fails on device open : %s\n",libusb_error_name(rc));
		m_dev = NULL;
		m_handle = NULL;
		m_connected = false;
	} else {
		SetHandle(handle);
		SetbcdUSB(desc.bcdUSB);
		// TODO : Get interface and endpoint information from dev
		//SetEndpoint(0,1);
		interface = ids[index].Interface;
		epaddress = ids[index].EndPoint;
		SetEndpoint(interface,epaddress);
		m_connected = true;

		printf("%s ---- [interface=%d, epaddress=%d, index=%d]", __func__, interface, epaddress, index);
		printf( "libusb: USB version = %3X\n",getUSBVersion());
	}
*/
	m_timeout = 500; // ms

};

usbdev::usbdev(libusb_device *dev)
{
	struct 	libusb_device_descriptor desc;
	int 	rc;
	int     index = -1;
	libusb_device_handle *handle;
	int     interface,epaddress;

	m_dev = dev;

	rc = libusb_get_device_descriptor(m_dev, &desc);
	if (LIBUSB_SUCCESS != rc) {
		printf( "libusb: Error getting device descriptor\n");
	}

	//DBGTRACE(DBG_USB,"usbdev: Construct %x\n",(int)this);

	// have been filter the VID and the PID in example.cpp check_devs function
	index = desc.idProduct & 0x0F;
	index=2;
	printf("Eddie Detect : VID = 0x%0X/PID = 0x%0X\n",desc.idVendor,desc.idProduct);

	if( index < 0) {
		m_dev = NULL;
		m_handle = NULL;
		m_connected = false;
		return;
	}

	rc = libusb_open (m_dev, &handle);
	if (LIBUSB_SUCCESS != rc) {
		printf( "libusb: Fails on device open : %s\n",libusb_error_name(rc));
		m_dev = NULL;
		m_handle = NULL;
		m_connected = false;
	} else {
		SetHandle(handle);
		SetbcdUSB(desc.bcdUSB);
		// TODO : Get interface and endpoint information from dev
		//SetEndpoint(0,1);
		interface = ids[index].Interface;
		epaddress = ids[index].EndPoint;
		SetEndpoint(interface,epaddress);
		m_connected = true;

		printf( "libusb: USB version = %3X\n",getUSBVersion());
	}

	m_timeout = 500; // ms

};

usbdev::~usbdev()
{
	if(m_handle) {
		libusb_clear_halt(m_handle,m_address);
//		libusb_release_interface( m_handle,m_interface );
		printf( "usbdev: libusb_clear_halt()\n");
		libusb_close (m_handle);
	}
	m_connected = false;
	m_handle = NULL;
	//DBGTRACE(DBG_USB,"usbdev: Destruct %x\n",(int)this);
};

void usbdev::SetHandle(libusb_device_handle *handle)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	m_handle = handle;
	m_connected = true;
};

void usbdev::SetEndpoint(short interface,int address)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	m_interface = interface;
	m_address   = address;
	libusb_claim_interface(m_handle,interface);
};

void usbdev::SetbcdUSB(short bcdUSB)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	m_bcdUSB = bcdUSB;
};


BOOL usbdev::GetStatus(USHORT* status)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int actlen = libusb_control_transfer(
		m_handle,		// handle
		0xC1,			// request_type
		0x40,			// bRequest,
		0,				// wValue,
		m_interface,	// wIndex,
		(unsigned char*) status, // data
		2, 				// wLength,
		m_timeout
		);

	*status &= 0xFF;

	return ( actlen > 0 );
};

BOOL usbdev::GetEDIDSize(USHORT* size)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int actlen = libusb_control_transfer(
		m_handle,		// handle
		0xC1,			// request_type
		0x41,			// bRequest,
		0,				// wValue,
		m_interface,	// wIndex,
		(unsigned char*) size, // data
		2, 				// wLength,
		m_timeout
		);

	return ( actlen > 0 );
};

BOOL usbdev::GetEDIDData(UCHAR *buff,int bank)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int actlen = libusb_control_transfer(
		m_handle,		// handle
		0xC1,			// request_type
		0x41,			// bRequest,
		(unsigned short) bank,			// wValue,
		m_interface,	// wIndex,
		(unsigned char*) buff, // data
		128, 			// wLength,
		m_timeout
		);

	return ( actlen > 0 );
};

BOOL usbdev::GetTargetVersion(char *data,int len)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int actlen = libusb_control_transfer(
		m_handle,		// handle
		0xC1,			// request_type
		0x43,			// bRequest,
		0,				// wValue,
		m_interface,	// wIndex,
		(unsigned char*) data, // data
		len, 			// wLength,
		m_timeout
		);

	return ( actlen > 0 );

};

BOOL usbdev::GetPowerStatus(USHORT* status)
{printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int actlen = libusb_control_transfer(
		m_handle,		// handle
		0xC1,			// request_type
		0x44,			// bRequest,
		0,				// wValue,
		m_interface,	// wIndex,
		(unsigned char*) status, // data
		2, 			// wLength,
		m_timeout
		);

	return ( actlen > 0 );

};

USHORT usbdev::getUSBVersion()
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	return (m_bcdUSB>>8);
};

BOOL usbdev::SetResolution(ULONG width,ULONG height)
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	char  buffer[4];
	int   actlen;
	unsigned short status;

	buffer[0] = width       & 0xFF;
	buffer[1] = (width>>8)  & 0xFF;
	buffer[2] = height      & 0xFF;
	buffer[3] = (height>>8) & 0xFF;

	status = 1;
	
	//printf( "usbdev: SetResolution %ld x %ld\n",width,height);

	while(status) {
		if(!GetStatus(&status)) {
			printf( "usbdev: Fails on GetStatus()\n" );
			return false;
		}
		if(status) usleep(100000); // 100ms delay
		printf( "usbdev: Wait source in\n" );
	}
//printf("Eddie %s ---- [ 111111111111111111111 ]", __func__);
	actlen = libusb_control_transfer(
		m_handle,		// handle
		0x41,			// request_type
		0x81,			// bRequest,
		0,				// wValue,
		m_interface,	// wIndex,
		(unsigned char*) buffer, // data
		4, 			    // wLength,
		m_timeout );

#if 1
	status = 1;
	while(status) {
		if(!GetStatus(&status)) return false;
		if(status) usleep(100000); // 100ms delay
	}
#endif

//	libusb_claim_interface( m_handle,m_interface );
	return ( actlen > 0 );

};

BOOL usbdev::SetCommand(USHORT cmd)
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int   actlen;
	unsigned short buf=cmd;

	actlen = libusb_control_transfer(
		m_handle,		// handle
		0x41,			// request_type
		0x82,			// bRequest,
		cmd,			// wValue,
		m_interface,	// wIndex,
		(unsigned char*) &buf, // data
		2, 			// wLength,
		m_timeout
		);

	return ( actlen > 0 );
};

BOOL usbdev::Write(UCHAR* data,ULONG SendLen,ULONG* ActLen)
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int   res;

	//printf( "usbdev: libusb_bulk_transfer(), entry\n");
	res = libusb_bulk_transfer(m_handle,m_address, data, SendLen+1,(int*)ActLen,m_timeout);
	//printf( "usbdev: libusb_bulk_transfer(), exit\n");
	if( res < 0 ) {
		printf( "usbdev: Fails on libusb_bulk_transfer()\n");
		libusb_clear_halt(m_handle,m_address);
		printf( "usbdev: Fails ,exit\n");
		return false;
	}
	return true;
}

BOOL usbdev::SetQuantTable(UCHAR* table,ULONG len,USHORT index)
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int   actlen;

	//printf("%s ---- [xxxxxxxxxxxxx m_interface=%d]", __func__, m_interface);
	actlen = libusb_control_transfer(
		m_handle,		// handle
		0x41,			// request_type
		0x83,			// bRequest,
		index,			// wValue,
		m_interface,	// wIndex,
		(unsigned char*) table, // data
		len, 			// wLength,
		m_timeout
		);

	return ( actlen > 0 );

};

BOOL usbdev::SetISPmode()
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int   actlen;
	unsigned short buf=0;

	actlen = libusb_control_transfer(
		m_handle,		// handle
		0x41,			// request_type
		0x84,			// bRequest,
		0,				// wValue,
		m_interface,	// wIndex,
		(unsigned char*) &buf, // data
		2, 				// wLength,
		m_timeout
		);

	return ( actlen > 0 );

}

BOOL usbdev::SetReConfig(USHORT flag)
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	int   actlen;
	unsigned short buf=0;

	actlen = libusb_control_transfer(
		m_handle,		// handle
		0x41,			// request_type
		0x8b,			// bRequest,
		flag,			// wValue,
		m_interface,	// wIndex,
		(unsigned char*) &buf, // data
		2, 				// wLength,
		m_timeout
		);

	return ( actlen > 0 );

}

void   usbdev::ResetDevice()
{//printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
	libusb_reset_device(m_handle);
};
