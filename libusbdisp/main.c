/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>

#include "usbDisplay.h"
#include <malloc.h>
#include <cstring>
#include <string>
#include <iostream>
#include "xrandr.h"
#include <unistd.h> 

UsbDisplay *usbservice;

//============================= usb hotplug =============================

libusb_device_handle *handle = NULL;
static bool attached_DISP = false;
static bool detached_DISP = false;

static bool check_devs(libusb_device **devs)
{
    int vendor_id  = 0x34c7;
    int product_id = 0x2105;
	libusb_device *dev;
	int i = 0, j = 0;
	uint8_t path[8]; 

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return false;
		}

		printf("%04x:%04x (bus %d, device %d)",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));
        
        if(desc.idVendor == vendor_id && desc.idProduct == product_id) 
        {
            usleep(300000); // if usb has attached may need delay for initial
            usbservice->jxInit(dev);
            attached_DISP = true;
            detached_DISP = false;
            return true;
        }
	}
    return false;
}

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
    usbservice->jxInit(dev);
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
        usbservice->jxDelete();
    attached_DISP = false;
    detached_DISP = true;
    //done++;

    return 0;
}
int usbDisplay_init(){
    usbservice = new UsbDisplay();


    //=========== usb init ================
    libusb_hotplug_callback_handle hp[2];
    int product_id, vendor_id, class_id;
    int rc;

    vendor_id  = 0x34c7;
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
#ifdef XDEBUG
#define NAME  "screenshot"
#define BPP   4
#include <png.h>

void initpngimage( png_image * pi, struct shmimage * image )
{
    bzero( pi, sizeof( png_image ) ) ;
    pi->version = PNG_IMAGE_VERSION ;
    pi->width = image->ximage->width ;
    pi->height = image->ximage->height ;
    pi->format = PNG_FORMAT_BGRA ;
}

int savepng( struct shmimage * image, char * path )
{
    FILE * f = fopen( path, "w" ) ;
    if( !f )
    {
        perror( NAME ) ;
        return false ;
    }
    png_image pi ;
    initpngimage( &pi, image ) ;
    unsigned int scanline = pi.width * BPP ;
    if( !png_image_write_to_stdio( &pi, f, 0, image->data, scanline, NULL) )
    {
        fclose( f ) ;
        printf( NAME ": could not save the png image\n" ) ;
        return false ;
    }
    fclose( f ) ;
    return true ;
}
#endif
int
main (int argc, char **argv)
{   
    bool extend = false;
    #ifdef XDEBUG
        struct timeval start_time,end_time;
        X11_Screen *test = new X11_Screen();
        test->init_dsp();
        test->informationCheck();

        if(argc > 1)
            if (!strcmp ("-sec", argv[1])) extend = true;

        while(1){
            test->cursorPosition();
            printf("\nCursor position X: %d, Y: %d \n", test->root_x, test->root_y);
            gettimeofday(&start_time,NULL);

            struct shmimage image = test->screenshot(extend);
            test->combineCursorBuffer(&image);
            //(char *)image.data,image.ximage->width ,image.ximage->height,image.ximage->width);

            gettimeofday(&end_time,NULL);
            int total_time = end_time.tv_usec - start_time.tv_usec;
            if(total_time < 0) total_time += 1000000;
            printf("\n test time : %d \n\n",total_time/1000);

            savepng(&image,(char*)"bin/qqqq.png");
            usleep(2000000);
        }
    #endif
    //=========== usb init ================

    timeval tv = {0, 0};
    if(usbDisplay_init())return EXIT_FAILURE;
    static bool old_connected = false; //have to set static in virtscreen mode


    if(argc > 1)
        if (!strcmp ("-sec", argv[1])) extend = true;
    X11_Screen *UsbDisplay = new X11_Screen();
    UsbDisplay->init_dsp();
    UsbDisplay->informationCheck();


	libusb_device **devs;
	ssize_t cnt;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0){
		libusb_exit(NULL);
		return (int) cnt;
	}

	if(check_devs(devs))attached_DISP = true;
	libusb_free_device_list(devs, 1);
    while (1) {

        int rc = libusb_handle_events_timeout (NULL,&tv);

        if (rc < 0)printf("libusb_handle_events() failed: %s\n", libusb_error_name(rc));

        //===================== usb attached_DISP ========================
        if (attached_DISP == true)
        {
               
            UsbDisplay->cursorPosition();
            //printf("\nCursor position X: %d, Y: %d \n", UsbDisplay->root_x, UsbDisplay->root_y);

            struct shmimage image = UsbDisplay->screenshot(extend);
            UsbDisplay->combineCursorBuffer(&image);

            usbservice->jxStartEncode((char *)image.data,image.ximage->width ,image.ximage->height,image.ximage->width);

            UsbDisplay->destroyimage( UsbDisplay->dsp_share, &image ) ;
            usleep(30000);
        }
    }
return 0;
}
