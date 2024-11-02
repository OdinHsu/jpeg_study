

#include <stdio.h>
#include <stdlib.h>
#include "debug_trace.h"
#include "EncodeEngine.h"
#include "usbDisplay.h"
#include "capture.h"
//=================== jx display =====================


///////////////////


int UsbDisplay::jxInit(libusb_device *dev)
{

    debug_mask = ( DBG_PROFILE );
//    debug_mask = ( DBG_ALL );

    printf("JNI jxInit\n");
    if( pCapture != NULL) {
        delete pCapture;
    }

    // initialize
    pCapture = new Capture();
    if(! pCapture->CreateDevice(dev)) {
        printf("Create Capture Device fail!\n");
        return 0;
    }
    printf("JNI jxInit finish\n");
    return 1;
};

void UsbDisplay::jxDelete()
{
    if( pCapture ) {
        delete pCapture;
        pCapture = NULL;
        printf("JNI deinit!\n");
    }
};

void UsbDisplay::jxStartEncode(char * in,int width,int height,int pitch)
{

//==========================================
    char *framebuffer = in;//(char*) env->GetDirectBufferAddress(in);
//    long sz = env->GetDirectBufferCapacity(in);
//    DBGTRACE(DBG_JNI,"%08p,sz = %ld\n",framebuffer,sz);
    pCapture->StartEncode(width,height,pitch,(unsigned char*)framebuffer);
    WaitEvent(sync_event);
};

void UsbDisplay::jxSetResolution(int width,int height) {
    printf("jxSetResolution(JNIEnv *env,jobject thiz,jint width,jint height)\n");
    pCapture->SetDevResolution(width, height);
    //pEncodeEngine->SetResolution(width,height);
};


//============================= usb hotplug =============================
/*
libusb_device_handle *handle = NULL;
static bool attached_DISP = false;
static bool detached_DISP = false;


static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    int rc;

    (void)ctx;
    (void)dev;
    (void)event;
    (void)user_data;

    rc = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error getting device descriptor\n");
    }

    printf ("Device attached: %04x:%04x\n", desc.idVendor, desc.idProduct);

    if (handle) {
        libusb_close (handle);
        handle = NULL;
    }
    printf("find disp devices\n");
    usleep(600000);
    jxInit(dev);
    attached_DISP = true;
    detached_DISP = false;
    //done++;

    return 0;
}

static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    (void)ctx;
    (void)dev;
    (void)event;
    (void)user_data;

    printf ("Device detached\n");

    if (handle) {
        libusb_close (handle);
        handle = NULL;
    }
    if (attached_DISP == true)
        jxDelete();
    attached_DISP = false;
    detached_DISP = true;
    //done++;

    return 0;
}*/
/*
int usbDisplay_init(){
    //=========== usb init ================
    libusb_hotplug_callback_handle hp[2];
    int product_id, vendor_id, class_id;
    int rc;

    vendor_id  = 0x04fc;
    product_id = 0x2105;
    class_id   = LIBUSB_HOTPLUG_MATCH_ANY;



//============================== libusb hotplug ===============================
    rc = libusb_init (NULL);
    if (rc < 0)
    {
        printf("failed to initialise libusb: %s\n", libusb_error_name(rc));
        return EXIT_FAILURE;
    }

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
        printf ("Hotplug capabilites are not supported on this platform\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_NO_FLAGS, vendor_id,
        product_id, class_id, hotplug_callback, NULL, &hp[0]);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error registering callback 0\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_NO_FLAGS, vendor_id,
        product_id,class_id, hotplug_callback_detach, NULL, &hp[1]);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error registering callback 1\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }
    return 0;

}
*/