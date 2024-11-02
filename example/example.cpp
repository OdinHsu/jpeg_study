#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

#include "driver.hpp"
#include "libevdipp/evdi.hpp"
#include "libevdipp/screen.hpp"
#include <unistd.h> 

std::shared_ptr<ScreenDriver> screen;
std::vector<unsigned char> edid_data_v;
namespace {

bool read_edid_from_file(const std::string& filename,
    std::vector<unsigned char>& edid)
{
    try {
        std::ifstream input(filename.c_str(), std::ios::binary | std::ios::ate);
        input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        if (input) {
            std::streamsize size = input.tellg();
            input.seekg(0, std::ios::beg);
            edid.resize(size);
            input.read((char*)edid.data(), size);
            return input.good();
        }
    } catch (const std::ios::failure&) {
        return false;
    }
    return false;
}

} // anonymous namespace

//============================= usb hotplug =============================

libusb_device_handle *handle = NULL;
static bool attached_DISP = false;
static bool detached_DISP = false;
libusb_device *temp_dev;
int rc;
void getEdid(){
    unsigned char * Edid_data =new unsigned char[256];
    libusb_device_handle *handles = NULL;

    rc = libusb_open (temp_dev, &handles);
    printf("Eddie %s ---- [xxxxx starting xxxxx]", __func__);
    for(int i = 1 ;i<= 2 ;i++){
       libusb_control_transfer(
        handles,       // handle
        0xC1,           // request_type
        0x41,           // bRequest,
        (unsigned short) i,          // wValue,
        0,    // wIndex,
        (unsigned char*) Edid_data+(i-1)*128, // data
        128,            // wLength,
        500
        );
    }
    for(int i =0; i<256;i++){
       edid_data_v.push_back(Edid_data[i]);
    }
    printf("size vector %ld \n",edid_data_v.size());
    if(handles) {
        libusb_clear_halt(handles,0);
//      libusb_release_interface( m_handle,m_interface );
        printf( "usbdev: libusb_clear_halt()\n");
        libusb_close (handles);
    }
    //m_connected = false;
    handle = NULL;
   //return ( actlen > 0 );

}


static bool check_devs(libusb_device **devs)
{
    int vendor_id = 0x34c7;
    int vendor_id_old= 0x04fc; 
    int product_id []= {0x2103, 0x2104, 0x2105, 0x2113, 0x2114, 0x2115};
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
        
        // should include all of product_id
        if(desc.idVendor == vendor_id && ( 0x2113 <= (desc.idProduct | 0x0010) && (desc.idProduct | 0x0010) <= 0x2115)) 
        {
            usleep(300000); // if usb has attached may need delay for initial
            temp_dev = dev;
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
    std::cout << "Qutting..." << std::endl;
    ev::get_default_loop().break_loop(ev::ALL);


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
    temp_dev = dev;
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
    {

    }
    attached_DISP = false;
    detached_DISP = true;
    //done++;
    screen->quit();
    screen.reset();
    return 0;
}
int usbDisplay_init(){
    //=========== usb init ================
    libusb_hotplug_callback_handle hp[2];
    int vendor_id, class_id ,vendor_id_old;
    int rc;

    vendor_id  = 0x34c7;
    vendor_id_old = 0x04fc;
    int product_id [] = {0x2103, 0x2104, 0x2105, 0x2113, 0x2114, 0x2115};
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
    
    for( int i=0; i < sizeof(product_id) / sizeof(int); i++)
    {
        rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_NO_FLAGS, vendor_id,
        	product_id[i], class_id, hotplug_callback, NULL, &hp[0]);
        if (LIBUSB_SUCCESS != rc) {
            fprintf (stderr, "Error registering attach callback 0\n");
            libusb_exit (NULL);
            return EXIT_FAILURE;
        }
    }

    for( int i=0; i < sizeof(product_id) / sizeof(int); i++)
    {
        rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_NO_FLAGS, vendor_id,
        	product_id[i], class_id, hotplug_callback_detach, NULL, &hp[0]);
        if (LIBUSB_SUCCESS != rc) {
            fprintf (stderr, "Error registering detach callback 0\n");
            libusb_exit (NULL);
            return EXIT_FAILURE;
        }
    }

    return 0;

}

int main(int argc, char* argv[])
{
    std::vector<unsigned char> edid;
    if (argc > 1) {
        if (!read_edid_from_file(argv[1], edid)) {
            std::cerr << "Reading the EDID file " << argv[1] << " failed."
                      << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Warning: no argument passed, using built-in sample EDID" << std::endl;
    }

    Evdi::log_handler = [](const std::string& message) {
        std::cout << "example: " << message << std::endl;
    };

    if(usbDisplay_init())return EXIT_FAILURE;
    static bool old_connected = false; //have to set static in virtscreen mode

	libusb_device **devs;
	ssize_t cnt;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0){
		libusb_exit(NULL);
		return (int) cnt;
	}


	if(check_devs(devs))attached_DISP = true;
	libusb_free_device_list(devs, 1);

    while(1){
        timeval tv = {0, 0};
        
        int rc = libusb_handle_events_timeout (NULL,&tv);
        if (rc < 0)printf("libusb_handle_events() failed: %s\n", libusb_error_name(rc));

        if (attached_DISP == true)
        {            
	        edid_data_v.clear();
            getEdid();
            Evdi evdi;
            if (!evdi) {
                std::cerr << "No usable EVDI found" << std::endl;
                return 2;
            }

            screen = std::make_shared<ScreenDriver>(evdi, edid_data_v, false);
            screen->add_screen(screen, temp_dev);
            screen->drive(20); // Hz
            //screen.reset();
        }
    };


    return 0;
}
