#define JPGENC
#ifdef JPGENC

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

//#include "debug_trace.h"
#include "jpeg_enc.h"

#define JPEG_QUALITY 95 // 2
#define HEADER_QUA 2
#define VID 0x34C7
#define PID 0x2114


// Encoder Block
#define BLOCK_WIDTH 32
#define BLOCK_HEIGHT 8
#define COLOR_COMPONENTS 3
#define BLOCK_SIZE BLOCK_WIDTH *BLOCK_HEIGHT *COLOR_COMPONENTS

static int blk_idx=0;

void rgb2ycc(int stride,int width8, int height8, char *raw_buffer, char *y_buffer);
unsigned char *load_bmp(const char *bmp, int *width, int *height);
bool SendPicFrameUsbCmd(unsigned short cmd);
bool pictureFrameCmd(libusb_device_handle *handle, unsigned short cmd);
bool jpgSetQuantTable(libusb_device_handle *handle, unsigned char *table, unsigned long len, unsigned short index);
bool jpgSetResolution(libusb_device_handle *handle, unsigned short width, unsigned short height);
bool jpgGetStatus(libusb_device_handle *handle, unsigned short *status);
int sendBuf(unsigned char* data, int len, int width, int height);
unsigned char* getBufFormBmp(unsigned char* raw_buffer, int width, int height, int* plen);

typedef struct
{
    unsigned int quality : 2;
    unsigned int len : 8;
    unsigned int block_index : 16;
    unsigned int reserved0 : 1;
    unsigned int end : 1;
    unsigned int id : 4;
} BlockHeader_t;

// for bitmap
#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

cinfo_t cinfo;
unsigned int* oldBuffer = NULL;

int main(int argc, const char *argv[])
{
    if(argc == 1)
    {
        puts("the number of arguments incorrect");
        return 0;
    }

    // insert code here...
    FILE *fo;
    int width, height;
    int width8, height8;
    int imagesize;
    int len;
    SBYTE *y_buffer = NULL;
    unsigned char *raw_buffer;
    int mode = 0;

    cinfo.jpeg_stream.pbitstream = NULL;

    
    do{
        int i=1;
        if(strcmp(argv[i], "reset") == 0)
        {

            // load bmp file into 32 bits raw image
            //========== usb ==========
            libusb_device **devs;
            int r;
            ssize_t cnt;

            r = libusb_init(NULL);
            if (r < 0)
                return r;

            cnt = libusb_get_device_list(NULL, &devs);
            if (cnt < 0)
            {
                libusb_exit(NULL);
                return (int)cnt;
            }
            //========================
            libusb_device *dev;
            int i = 0, j = 0;
            uint8_t path[8];
            while ((dev = devs[i++]) != NULL)
            {
                struct libusb_device_descriptor desc;
                int r = libusb_get_device_descriptor(dev, &desc);
                if (r < 0)
                {
                    fprintf(stderr, "failed to get device descriptor");
                    break;
                }

                //----------------------------
                if ((desc.idVendor == VID) && (desc.idProduct == PID))
                {
                    libusb_device_handle *handle;
                    r = libusb_open(dev, &handle);
                    if (LIBUSB_SUCCESS != r)
                        printf("libusb: Fails on device open : %s\n", libusb_error_name(r));
                    
                    libusb_reset_device(handle);
                    usleep(500000);
                }
            }        

            libusb_free_device_list(devs, 1);

            libusb_exit(NULL);
            return 0;

        }else if(strcmp(argv[i], "-j") == 0)
        {
            if(argc != 4)
            {
                puts("the number of arguments incorrect");
                return 0;
            }
            mode=0;

        }
        else if( strcmp(argv[i], "-u") == 0 )
        {
            if(argc != 5)
            {
                puts("the number of arguments incorrect");
                return 0;
            }
            mode=1;
            width8 = atoi(argv[3]);
            height8 = atoi(argv[4]);
            width8 = ((width8 + 7) >> 3) << 3;
            height8 = ((height8 + 7) >> 3) << 3;
        }
        else if( strcmp(argv[i], "-b") == 0 )
        {
            if(argc != 4)
            {
                puts("the number of arguments incorrect");
                return 0;
            }
            mode=2;
        }
        else if( strcmp(argv[i], "-l") == 0 )
        {
            if(argc != 4)
            {
                puts("the number of arguments incorrect");
                return 0;
            }
            mode=3;
        }
        else if( strcmp(argv[i], "-t") == 0 )
        {
            if(argc != 3)
            {
                puts("the number of arguments incorrect");
                return 0;
            }
            mode=4;
        }
        else if( strcmp(argv[i], "-f") == 0 )
        {
            if(argc != 4)
            {
                puts("the number of arguments incorrect");
                return 0;
            }
            mode=5;
        }
        else if( strcmp(argv[i], "--next") == 0 )
        {
            SendPicFrameUsbCmd(1010);
            return 0;
        }
        else if( strcmp(argv[i], "--close") == 0 )
        {
            SendPicFrameUsbCmd(1111);
            return 0;
        }
        else if( strcmp(argv[i], "-h") == 0 )
        {
            puts("this program has 3 modes:");
            puts("\t'-j' save BMP to jpeg file, usage: \t\t\tjpegenc -j src.bmp dest.jpg");
            puts("\t'-u' transmit bin file to 880x USB device, usage: \tjpegenc -u src.bin {width} {height}");
            puts("\t'-b' save BMP to 880x image bin file, usage: \t\tjpegenc -b src.bmp dest.bin");
            puts("\t'-l' BMP to USB and compare BMP for dirty block, usage:\tjpegenc -l src.bmp out.bin");
            puts("\t'-f' save bin file to text file, usage: \t\tjpegenc -f src.bin dest.txt");
            return 0;
        }
        else{
            puts("mode argument error");
            return 0;
        }
    }while(0);


    if( mode == 0 ) //SAVEJPG  #jpegenc -j src.bmp dest.jpg
    {
        // load bmp file into 32 bits raw image
        raw_buffer = load_bmp((char *)argv[2], &width, &height);
        width8 = ((width + 7) >> 3) << 3;
        height8 = ((height + 7) >> 3) << 3;

        if (raw_buffer == NULL)
            return 0;
        fo = fopen(argv[3], "wb");
        if (fo == NULL)
        {
            printf("Can't open output file %s", argv[3]);
            if (raw_buffer)
                free(raw_buffer);
            return 0;
        }

        printf("%-48s, %4d x %4d \n", argv[2], width, height);

        Initialize(&cinfo, 95);

        imagesize = width8 * height8;
        y_buffer = (SBYTE *)malloc(imagesize * 3);

        cinfo.Y_buffer = y_buffer;
        cinfo.Cb_buffer = cinfo.Y_buffer + imagesize;
        cinfo.Cr_buffer = cinfo.Cb_buffer + imagesize;

        cinfo.Ximage = width8;
        cinfo.Yimage = height8;
        cinfo.jpeg_stream.pbitstream = (unsigned char *)malloc(width8 * height8 * 4);
        cinfo.jpeg_stream.pos = 0;

        WriteHeader(&cinfo, width, height);

        // Profile Start
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        rgb2ycc(width8, width8, height8, (char *)raw_buffer, (char *)y_buffer);

        // Profile Stop
        gettimeofday(&end_time, NULL);

        int total_time = end_time.tv_usec - start_time.tv_usec;
        if (total_time < 0)
            total_time += 1000000;

        printf("RGB2YCbCr time = %6.2f ms\n", total_time / 1000.);

        len = encode(&cinfo);

        // Profile Stop
        gettimeofday(&end_time, NULL);

        total_time = end_time.tv_usec - start_time.tv_usec;
        if (total_time < 0)
            total_time += 1000000;

        printf("Total Elaspise time = %6.2f ms\n", total_time / 1000.);

        if (len)
            fwrite(cinfo.jpeg_stream.pbitstream, 1, len, fo);

        // free system resource and exit
        fclose(fo);

        if (raw_buffer)
            free(raw_buffer);
        if (cinfo.jpeg_stream.pbitstream)
            free(cinfo.jpeg_stream.pbitstream);
        if (y_buffer)
            free(y_buffer);

        DeInitialize();
    }
    else if( mode == 1 ) //JPGTOUSB #jpegenc -u src.bin {width} {height}
    {
        Initialize(&cinfo, JPEG_QUALITY);
        unsigned char *pfile;

        fo = fopen(argv[2], "rb");
        if (fo == NULL)
        {
            printf("Error while open file : %s\n", argv[2]);
            free(raw_buffer);
            return 0;
        }

        fseek(fo, 0, SEEK_END);
        len = ftell(fo);
        pfile = (unsigned char *)malloc(len);
        fseek(fo, 0, SEEK_SET);

        fread(pfile, 1, len, fo);
        fclose(fo);
        //========== usb ==========
        libusb_device **devs;
        int r;
        ssize_t cnt;

        r = libusb_init(NULL);
        if (r < 0)
            return r;

        cnt = libusb_get_device_list(NULL, &devs);
        if (cnt < 0)
        {
            libusb_exit(NULL);
            return (int)cnt;
        }
        //========================
        libusb_device *dev;
        int i = 0, j = 0;
        uint8_t path[8];

        while ((dev = devs[i++]) != NULL)
        {
            struct libusb_device_descriptor desc;
            int r = libusb_get_device_descriptor(dev, &desc);
            if (r < 0)
            {
                fprintf(stderr, "failed to get device descriptor");
                break;
            }

            // printf("%04x:%04x (bus %d, device %d)",
            //        desc.idVendor, desc.idProduct,
            //        libusb_get_bus_number(dev), libusb_get_device_address(dev));

            // r = libusb_get_port_numbers(dev, path, sizeof(path));
            // if (r > 0)
            // {
            //     printf(" path: %d", path[0]);
            //     for (j = 1; j < r; j++)
            //         printf(".%d", path[j]);
            // }
            // printf("\n");

            //----------------------------
            if ((desc.idVendor == VID) && (desc.idProduct == PID))
            {
                libusb_device_handle *handle;
                r = libusb_open(dev, &handle);
                if (LIBUSB_SUCCESS != r)
                    printf("libusb: Fails on device open : %s\n", libusb_error_name(r));
            
                jpgSetResolution(handle, 0, 0);
                unsigned char Qtable[140];

                usleep(500000);

                for (int index = 1; index < 4; index++)
                {
                    // GetQuantTable(Qtable,138,index);
                    jpeg_stream_t qtInfo;
                    qtInfo.pbitstream = Qtable;
                    qtInfo.pos = 0;
                    GetDQTinfo(&cinfo, &qtInfo);
                    jpgSetQuantTable(handle, Qtable, 138, index) ? printf("SetQuantTable %d\n", index) : printf("SetQuantTable %d Fails!\n", index);
                }

                jpgSetResolution(handle, width8, height8);

                // send encode data to USB display
                // if (!Write(cxt->usb_handle, pfile, len, (int *)&offset))
                //     printf("USB write failed!\n");
                int ActLen = 0;
                int res = libusb_bulk_transfer(handle, 1, pfile, len + 1, &ActLen, 500);
                if (res < 0)
                    printf("usbdev: Fails on libusb_bulk_transfer()\n");
                free(pfile);
            }
        }

        usleep(1000000);

        libusb_free_device_list(devs, 1);

        libusb_exit(NULL);
        if (raw_buffer)
            free(raw_buffer);
    }
    else if( mode == 2 )  //BMP to FW BIN #jpegenc -b src.bmp dest.bin
    {
        // load bmp file into 32 bits raw image
        raw_buffer = load_bmp((char *)argv[2], &width, &height);
        width8 = ((width + 7) >> 3) << 3;
        height8 = ((height + 7) >> 3) << 3;

        if (raw_buffer == NULL)
            return 0;
        fo = fopen(argv[3], "wb");
        if (fo == NULL)
        {
            printf("Can't open output file %s", argv[3]);
            if (raw_buffer)
                free(raw_buffer);
            return 0;
        }

        printf("%-48s, %4d x %4d \n", argv[2], width, height);

        unsigned char *bitstream = (unsigned char *)malloc(width8 * height8 * 4);
        unsigned char *blk_buffer = (unsigned char *)malloc(BLOCK_SIZE);
        BlockHeader_t *pheader = (BlockHeader_t *)blk_buffer;
        unsigned char *getBufferPosition = raw_buffer;
        unsigned long streamlen = 0;

        // streamlen += 32;
        // unsigned int ID = 0x53525053;
        // unsigned int decoder_type = 0x80000000;
        // unsigned int count = 0;
        // unsigned int length = len/4;
        // for(int i=0; i<8; i++)
        // {
        //     *((unsigned int* )bitstream +i) =    0xD0000000;
        // }
        
        Initialize(&cinfo, JPEG_QUALITY);

        imagesize = BLOCK_WIDTH * BLOCK_HEIGHT;
        y_buffer = (SBYTE *)malloc(imagesize * 3);

        cinfo.Y_buffer = y_buffer;
        cinfo.Cb_buffer = cinfo.Y_buffer + imagesize;
        cinfo.Cr_buffer = cinfo.Cb_buffer + imagesize;

        cinfo.Ximage = BLOCK_WIDTH;
        cinfo.Yimage = BLOCK_HEIGHT;
        cinfo.jpeg_stream.pbitstream = blk_buffer + 4;
        cinfo.jpeg_stream.pos = 0;

        memset(pheader, 0, 4);
        pheader->id = 13; // 0xd
        pheader->quality = HEADER_QUA;
        pheader->end = 0;
        pheader->reserved0 = 0;

        for (int blk_cnt = 0, blk_y = 0; blk_y < height8 / BLOCK_HEIGHT; blk_y++)
        {
            for (int blk_x = 0; blk_x < width8 / BLOCK_WIDTH; blk_x++)
            {
                unsigned long block_offset = blk_x * BLOCK_WIDTH * 4;
                int compress_len;

                rgb2ycc(width8, BLOCK_WIDTH, BLOCK_HEIGHT, (char *)raw_buffer + block_offset, (char *)y_buffer);

                
                cinfo.jpeg_stream.pos = 0;
                compress_len = (encode(&cinfo) + 3) & 0xFFFFFFFC;
                // block header
                pheader->block_index = blk_cnt++;
                pheader->len = (compress_len / 4) - 1;
                

                if ((blk_x == (width8 / BLOCK_WIDTH - 1)) && (blk_y == (height8 / BLOCK_HEIGHT - 1)))
                    pheader->end = 1;

                // get bitstream
                compress_len += 4;
                memcpy(bitstream + streamlen, blk_buffer, compress_len);
                streamlen += compress_len;
            }
            raw_buffer += (width8 * 4 * BLOCK_HEIGHT);
        }

        // Padding 0xFFFFD9FF to 128 bytes alignment
        unsigned long *pading = (unsigned long *)(bitstream + streamlen);
        do
        {
            *pading++ = 0xFFFFD9FF; // end of JPEG and end of frame
            streamlen += 4;
        } while (streamlen % 128);

        // send encode data to USB display
        // if (!Write(cxt->usb_handle, cxt->bitstream, streamlen, (int *)&offset))
        //     printf("USB write failed!\n");

        if (streamlen)
            fwrite(bitstream, 1, streamlen, fo);
        // free system resource and exit
        fclose(fo);

        if (y_buffer)
            free(y_buffer);
        if (bitstream)
            free(bitstream);
        if (blk_buffer)
            free(blk_buffer);
        if( getBufferPosition) free(getBufferPosition);
        DeInitialize();
    }
    else if( mode == 3 )  //BMP to USB and compare BMP for dirty block #jpegenc -l src.bmp out.bin
    {
        // load bmp file into 32 bits raw image
        //========== usb ==========
        libusb_device **devs;
        int r;
        ssize_t cnt;

        r = libusb_init(NULL);
        if (r < 0)
            return r;

        cnt = libusb_get_device_list(NULL, &devs);
        if (cnt < 0)
        {
            libusb_exit(NULL);
            return (int)cnt;
        }
        //========================
        libusb_device *dev;
        int i = 0, j = 0;
        uint8_t path[8];
        while ((dev = devs[i++]) != NULL)
        {
            struct libusb_device_descriptor desc;
            int r = libusb_get_device_descriptor(dev, &desc);
            if (r < 0)
            {
                fprintf(stderr, "failed to get device descriptor");
                break;
            }

            //----------------------------
            if ((desc.idVendor == VID) && (desc.idProduct == PID))
            {
                libusb_device_handle *handle;
                r = libusb_open(dev, &handle);
                if (LIBUSB_SUCCESS != r)
                    printf("libusb: Fails on device open : %s\n", libusb_error_name(r));
                raw_buffer = load_bmp((char *)argv[2], &width, &height);
                int plen=0;
                unsigned char* data = (unsigned char*)getBufFormBmp(raw_buffer, width, height, &plen);

                // ================= save bitstream data to out.bin ===================
                fo = fopen(argv[3], "wb");
                if (fo == NULL)
                    printf("Can't open output file %s", argv[3]);

                fwrite(data, 1, plen, fo);
                fclose(fo);
                // =====================================================================

                printf("%-48s, %4d x %4d \n", argv[2], width, height);

                jpgSetResolution(handle, 0, 0);

                unsigned char Qtable[140];

                usleep(500000);

                for (int index = 1; index < 4; index++)
                {
                    // GetQuantTable(Qtable,138,index);
                    jpeg_stream_t qtInfo;
                    qtInfo.pbitstream = Qtable;
                    qtInfo.pos = 0;
                    GetDQTinfo(&cinfo, &qtInfo);
                    jpgSetQuantTable(handle, Qtable, 138, index) ? printf("SetQuantTable %d\n", index) : printf("SetQuantTable %d Fails!\n", index);
                }

                jpgSetResolution(handle, width, height);
                printf("jpgSetResolution: %d, %d\n", width, height);

                printf("bulk_transfer len: %d\n", plen);
                int ActLen = 0;
                int res = libusb_bulk_transfer(handle, 1, data, plen + 1, &ActLen, 500);
                usleep(1000000);
                free(data);
                unsigned char tmp[2];
                fgets(tmp, 2, stdin);
                if(tmp[0]='9')
                    blk_idx=1;
                else
                    blk_idx=0;
                while(1)
                {
                    raw_buffer = load_bmp((char *)argv[2], &width, &height);
                    plen=0;
                    data = (unsigned char*)getBufFormBmp(raw_buffer, width, height, &plen);
                    // send encode data to USB display
                    printf("bulk_transfer len: %d\n", plen);
                    // ================= save bitstream data to out.bin ===================
                    if(plen){
                        fo = fopen(argv[3], "wb");
                        if (fo == NULL)
                            printf("Can't open output file %s", argv[3]);

                        fwrite(data, 1, plen, fo);
                        fclose(fo);
                    }
                    // =====================================================================

                    ActLen = 0;
                    res = libusb_bulk_transfer(handle, 1, data, plen + 1, &ActLen, 500);
                    if (res < 0)
                        printf("usbdev: Fails on libusb_bulk_transfer()\n");
                    usleep(1000000);
                    free(data);
                    fgets(tmp, 2, stdin);
                    if(tmp[0]='9')
                        blk_idx=1;
                    else
                        blk_idx=0;
                }
            }
        if(oldBuffer)
            free(oldBuffer);
        usleep(1000000);

        libusb_free_device_list(devs, 1);

        libusb_exit(NULL);
        }
    }
    else if( mode == 4 )  //BIN to USB without qtable #jpegenc -t src.bin
    {
        // load bmp file into 32 bits raw image
        //========== usb ==========
        libusb_device **devs;
        int r;
        ssize_t cnt;

        r = libusb_init(NULL);
        if (r < 0)
            return r;

        cnt = libusb_get_device_list(NULL, &devs);
        if (cnt < 0)
        {
            libusb_exit(NULL);
            return (int)cnt;
        }
        //========================
        libusb_device *dev;
        int i = 0, j = 0;
        uint8_t path[8];
        while ((dev = devs[i++]) != NULL)
        {
            struct libusb_device_descriptor desc;
            int r = libusb_get_device_descriptor(dev, &desc);
            if (r < 0)
            {
                fprintf(stderr, "failed to get device descriptor");
                break;
            }

            //----------------------------
            if ((desc.idVendor == VID) && (desc.idProduct == PID))
            {
                libusb_device_handle *handle;
                r = libusb_open(dev, &handle);
                if (LIBUSB_SUCCESS != r)
                    printf("libusb: Fails on device open : %s\n", libusb_error_name(r));
                
                unsigned char *pfile;

                fo = fopen(argv[2], "rb");
                if (fo == NULL)
                {
                    printf("Error while open file : %s\n", argv[2]);
                    free(raw_buffer);
                    return 0;
                }

                fseek(fo, 0, SEEK_END);
                len = ftell(fo);
                pfile = (unsigned char *)malloc(len);
                fseek(fo, 0, SEEK_SET);

                fread(pfile, 1, len, fo);
                fclose(fo);
                
                usleep(500000);
                printf("bulk_transfer len: %d\n", len);
                int ActLen = 0;
                int res = libusb_bulk_transfer(handle, 1, pfile, len + 1, &ActLen, 500);
                free(pfile);
                usleep(500000);
            }
        }        

        libusb_free_device_list(devs, 1);

        libusb_exit(NULL);
    }
    else if( mode == 5 )  //BIN to TEXT FILE #jpegenc -f src.bin dest.txt
    {
        unsigned char *pfile;

        fo = fopen(argv[2], "rb");
        if (fo == NULL)
        {
            printf("Error while open file : %s\n", argv[2]);
            free(raw_buffer);
            return 0;
        }

        fseek(fo, 0, SEEK_END);
        len = ftell(fo);
        pfile = (unsigned char *)malloc(len);
        fseek(fo, 0, SEEK_SET);

        fread(pfile, 1, len, fo);
        fclose(fo);

        //========================

        // response_packet_t header;

        // header.ID = 0x53525053;
        // header.decoder_type = 0x80000000;
        // header.count = 0;
        // header.length = FrameImgSize[Num]/4 - 4; // base on size of unsigned int
        unsigned int ID = 0x53525053;
        unsigned int decoder_type = 0x80000000;
        unsigned int count = 0;
        unsigned int length = len/4;

        fo = fopen (argv[3], "w+");

        //ID
        for(int i=0; i<4; i++)
        {
            fprintf(fo, "0x%02x, ", ID & 0xFF);
            ID = ID >> 8;
        }

        //decoder_type
        for(int i=0; i<4; i++)
        {
            if(i == 3)
            {
                fprintf(fo, "0x%02x, ", 0x80);
                break;
            }
            fprintf(fo, "0x%02x, ", 0);
        }

        //count
        for(int i=0; i<4; i++)fprintf(fo, "0x%02x, ", 0);

        //length
        for(int i=0; i<4; i++)
        {
            fprintf(fo, "0x%02x, ", length & 0xFF);
            length = length >> 8;
        }
        fprintf(fo,"\n");

        for(int i=0; i < len; i++)
        {
            fprintf(fo, "0x%02x, ", *(pfile + i));

            if((i+1) % 16==0) 
                fprintf(fo,"\n");
        }
        
        fclose(fo);

        if (raw_buffer)
            free(raw_buffer);
    }
    return 0;
}

unsigned char* getBufFormBmp(unsigned char* raw_buffer, int width, int height, int* plen)
{
            int width8 = ((width + 7) >> 3) << 3;
            int height8 = ((height + 7) >> 3) << 3;
            

            if (raw_buffer == NULL)
                return NULL;
            
            static int first = false;
            if(oldBuffer == NULL)
            {
                oldBuffer = (unsigned int*)malloc(width*height*4+BLOCK_WIDTH*4);
                first = true;
                unsigned int* src;
                unsigned int* dst;
                src = (unsigned int*)raw_buffer;
                dst = oldBuffer;
                for(int i=0;i < height;i++) {
                    memcpy(dst, src, width*sizeof(int));
                    src  += width;
                    dst += width;
                }
            }

            unsigned char *bitstream = (unsigned char *)malloc(width8 * height8 * 4);
            unsigned char *blk_buffer = (unsigned char *)malloc(BLOCK_SIZE);
            BlockHeader_t *pheader = (BlockHeader_t *)blk_buffer;
            unsigned char *getBufferPosition = raw_buffer;
            unsigned long streamlen = 0;

            // streamlen += 32;
            // unsigned int ID = 0x53525053;
            // unsigned int decoder_type = 0x80000000;
            // unsigned int count = 0;
            // unsigned int length = len/4;
            // for(int i=0; i<8; i++)
            // {
            //     *((unsigned int* )bitstream +i) =    0xD0000000;
            // }
            
            Initialize(&cinfo, JPEG_QUALITY);

            int imagesize = BLOCK_WIDTH * BLOCK_HEIGHT;
            
            SBYTE *y_buffer = NULL;
            y_buffer = (SBYTE *)malloc(imagesize * 3);

            cinfo.Y_buffer = y_buffer;
            cinfo.Cb_buffer = cinfo.Y_buffer + imagesize;
            cinfo.Cr_buffer = cinfo.Cb_buffer + imagesize;

            cinfo.Ximage = BLOCK_WIDTH;
            cinfo.Yimage = BLOCK_HEIGHT;
            cinfo.jpeg_stream.pbitstream = blk_buffer + 4;
            cinfo.jpeg_stream.pos = 0;

            memset(pheader, 0, 4);
            pheader->id = 13; // 0xd
            pheader->quality = HEADER_QUA;
            pheader->end = 0;
            pheader->reserved0 = 0;
            int blk_cnt, blk_y;
            int dirty_blk = 0;
            unsigned int* poldBuffer = oldBuffer;
            BlockHeader_t *last_header = (BlockHeader_t *)bitstream;
            for (blk_cnt = 0, blk_y = 0; blk_y < height8 / BLOCK_HEIGHT; blk_y++)
            {
                for (int blk_x = 0; blk_x < width8 / BLOCK_WIDTH; blk_x++, blk_cnt++)
                {
                    unsigned long block_offset = blk_x * BLOCK_WIDTH * 4;
                    int compress_len;

                    if(!first && (memcmp((unsigned char* )poldBuffer + block_offset, raw_buffer + block_offset, BLOCK_SIZE) == 0))
                        continue;
                    
                    if(!first)
                        memcpy((unsigned char* )poldBuffer + block_offset, raw_buffer + block_offset, BLOCK_SIZE);

                    dirty_blk++;
                    if(blk_idx)
                    {
                        printf("dirty block index: %d\n",blk_cnt);
                    }
                    rgb2ycc(width8, BLOCK_WIDTH, BLOCK_HEIGHT, (char *)raw_buffer + block_offset, (char *)y_buffer);

                    
                    cinfo.jpeg_stream.pos = 0;
                    compress_len = (encode(&cinfo) + 3) & 0xFFFFFFFC;
                    // block header
                    pheader->block_index = blk_cnt;
                    pheader->len = (compress_len / 4) - 1;
                    

                    if ((blk_x == (width8 / BLOCK_WIDTH - 1)) && (blk_y == (height8 / BLOCK_HEIGHT - 1)))
                        pheader->end = 1;

                    // get bitstream
                    compress_len += 4;
                    last_header = (BlockHeader_t *)(bitstream + streamlen);
                    memcpy(bitstream + streamlen, blk_buffer, compress_len);
                    streamlen += compress_len;
                }
                raw_buffer += (width8 * 4 * BLOCK_HEIGHT);
                poldBuffer = (unsigned int* )((unsigned char* )poldBuffer + (width8 * 4 * BLOCK_HEIGHT));
            }

            last_header->end = 1;

            printf("dirty_blk: %d, blocks: %d, len: %ld\n", dirty_blk, blk_cnt+1, streamlen);

            // Padding 0xFFFFD9FF to 128 bytes alignment
            unsigned long *pading = (unsigned long *)(bitstream + streamlen);
            do
            {
                *pading++ = 0xFFFFD9FF; // end of JPEG and end of frame
                streamlen += 4;
            } while (streamlen % 128);

            // send encode data to USB display
            // if (!Write(cxt->usb_handle, cxt->bitstream, streamlen, (int *)&offset))
            //     printf("USB write failed!\n");


            // free system resource and exit

            if (y_buffer)
                free(y_buffer);
            if (blk_buffer)
                free(blk_buffer);
            if( getBufferPosition) free(getBufferPosition);
            DeInitialize();

            *plen = streamlen;
            first = false;
            if (streamlen) 
                return bitstream;
}

void rgb2ycc(int stride, int width8, int height8, char *raw_buffer, char *y_buffer)
{
    // convert rgb to y cb cr using simd instruction;
    SBYTE **component[3];
    SBYTE *pRGBRow[8];
    SBYTE *pYRow[8];
    SBYTE *pURow[8];
    SBYTE *pVRow[8];
    SBYTE *p_src = (SBYTE *)raw_buffer;
    SBYTE *p_dest = (SBYTE *)y_buffer;
    int i, y;
    int imagesize = width8 * height8;

    component[0] = pYRow;
    component[1] = pURow;
    component[2] = pVRow;

    for (y = 0; y < height8; y += 8)
    {
        for (i = 0; i < 8; i++)
        {
            pRGBRow[i] = p_src;
            pYRow[i] = p_dest;
            pVRow[i] = pYRow[i] + imagesize;
            pURow[i] = pVRow[i] + imagesize;
            p_src += stride * 4;
            p_dest += width8;
        };
        jsimd_extbgrx_ycc_convert(width8, (JSAMPARRAY)pRGBRow, (JSAMPIMAGE)component, 0, 8);
    }
}

unsigned char *load_bmp(const char *bmp, int *width, int *height)
{
    FILE *fi;
    long len;

    unsigned char *pfile = NULL;
    unsigned char *input_buffer = NULL;
    unsigned char *psrc;
    unsigned int pitch;
    int _width;
    int _height;
    int i, j;

    fi = fopen(bmp, "rb");
    if (fi == NULL)
    {
        printf("Error while opening file: %s\n", bmp);
        return NULL;
    }

    // 确定文件大小
    if (fseek(fi, 0, SEEK_END) != 0)
    {
        printf("Error seeking end of file: %s\n", bmp);
        fclose(fi);
        return NULL;
    }

    len = ftell(fi);
    if (len < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
    {
        printf("File too small to be a valid BMP: %s\n", bmp);
        fclose(fi);
        return NULL;
    }

    if (fseek(fi, 0, SEEK_SET) != 0)
    {
        printf("Error seeking start of file: %s\n", bmp);
        fclose(fi);
        return NULL;
    }

    pfile = (unsigned char *)malloc(len);
    if (pfile == NULL)
    {
        printf("Memory allocation failed for pfile\n");
        fclose(fi);
        return NULL;
    }

    size_t read_len = fread(pfile, 1, len, fi);
    if (read_len != len)
    {
        printf("Error reading BMP file: %s\n", bmp);
        free(pfile);
        fclose(fi);
        return NULL;
    }

    fclose(fi);

    // 检查 BMP 签名
    BITMAPFILEHEADER *fileHeader = (BITMAPFILEHEADER *)pfile;
    if (fileHeader->bfType != 0x4D42) // 'BM' in little endian
    {
        printf("Not a BMP file: %s\n", bmp);
        free(pfile);
        return NULL;
    }

    // 读取 BMP 信息头
    BITMAPINFOHEADER *infoHeader = (BITMAPINFOHEADER *)(pfile + sizeof(BITMAPFILEHEADER));
    _width = infoHeader->biWidth;
    _height = infoHeader->biHeight;
    unsigned int biBitCount = infoHeader->biBitCount;
    unsigned int biCompression = infoHeader->biCompression;

    // 检查压缩类型
    if (biCompression != 0) // BI_RGB
    {
        printf("Unsupported BMP compression: %u\n", biCompression);
        free(pfile);
        return NULL;
    }

    // 仅支持 24 位和 32 位 BMP
    if (biBitCount != 24 && biBitCount != 32)
    {
        printf("Unsupported BMP bit count: %u\n", biBitCount);
        free(pfile);
        return NULL;
    }

    // 原始高度用于检查是顶向下还是底向上
    int mHeight = _height;

    // 获取绝对值
    _width = (_width >= 0) ? _width : -_width;
    _height = (_height >= 0) ? _height : -_height;

    // 分配 input_buffer 大小为 _width * _height * 4
    input_buffer = (unsigned char *)malloc(_width * _height * 4);
    if (input_buffer == NULL)
    {
        printf("Memory allocation failed for input_buffer\n");
        free(pfile);
        return NULL;
    }

    pitch = _width * 4;

    // 获取像素数据指针
    psrc = pfile + fileHeader->bfOffBits;

    if (biBitCount == 24)
    {
        printf("24 bit BMP file\n");
        int srcpitch = ((_width * 3 + 3) / 4) * 4; // 行填充到 4 字节的倍数

        for (i = 0; i < _height; i++)
        {
            if (mHeight < 0) // 顶向下
            {
                for (j = 0; j < _width; j++)
                {
                    input_buffer[i * pitch + j * 4] = psrc[i * srcpitch + j * 3];
                    input_buffer[i * pitch + j * 4 + 1] = psrc[i * srcpitch + j * 3 + 1];
                    input_buffer[i * pitch + j * 4 + 2] = psrc[i * srcpitch + j * 3 + 2];
                    input_buffer[i * pitch + j * 4 + 3] = 0; // Alpha 通道
                }
            }
            else // 底向上
            {
                for (j = 0; j < _width; j++)
                {
                    input_buffer[i * pitch + j * 4] = psrc[(_height - 1 - i) * srcpitch + j * 3];
                    input_buffer[i * pitch + j * 4 + 1] = psrc[(_height - 1 - i) * srcpitch + j * 3 + 1];
                    input_buffer[i * pitch + j * 4 + 2] = psrc[(_height - 1 - i) * srcpitch + j * 3 + 2];
                    input_buffer[i * pitch + j * 4 + 3] = 0; // Alpha 通道
                }
            }
        }
    }
    else if (biBitCount == 32)
    {
        printf("32 bit BMP file\n");
        int srcpitch = _width * 4; // 32 位 BMP 每行像素无填充

        for (i = 0; i < _height; i++)
        {
            if (mHeight < 0) // 顶向下
            {
                for (j = 0; j < _width; j++)
                {
                    input_buffer[i * pitch + j * 4] = psrc[i * srcpitch + j * 4];
                    input_buffer[i * pitch + j * 4 + 1] = psrc[i * srcpitch + j * 4 + 1];
                    input_buffer[i * pitch + j * 4 + 2] = psrc[i * srcpitch + j * 4 + 2];
                    input_buffer[i * pitch + j * 4 + 3] = psrc[i * srcpitch + j * 4 + 3];
                }
            }
            else // 底向上
            {
                for (j = 0; j < _width; j++)
                {
                    input_buffer[i * pitch + j * 4] = psrc[(_height - 1 - i) * srcpitch + j * 4];
                    input_buffer[i * pitch + j * 4 + 1] = psrc[(_height - 1 - i) * srcpitch + j * 4 + 1];
                    input_buffer[i * pitch + j * 4 + 2] = psrc[(_height - 1 - i) * srcpitch + j * 4 + 2];
                    input_buffer[i * pitch + j * 4 + 3] = psrc[(_height - 1 - i) * srcpitch + j * 4 + 3];
                }
            }
        }
    }

    free(pfile);

    *width = _width;
    *height = _height;

    return input_buffer;
}

int sendBuf(unsigned char* data, int len, int width, int height)
{
    

        return 0;
}

bool SendPicFrameUsbCmd(unsigned short cmd)
{
        //========== usb ==========
        libusb_device **devs;
        int r;
        ssize_t cnt;
        bool res;

        r = libusb_init(NULL);
        if (r < 0)
            return r;

        cnt = libusb_get_device_list(NULL, &devs);
        if (cnt < 0)
        {
            libusb_exit(NULL);
            return (int)cnt;
        }
        //========================
        libusb_device *dev;
        int i = 0, j = 0;
        uint8_t path[8];

        while ((dev = devs[i++]) != NULL)
        {
            struct libusb_device_descriptor desc;
            int r = libusb_get_device_descriptor(dev, &desc);
            if (r < 0)
            {
                fprintf(stderr, "failed to get device descriptor");
                break;
            }

            //----------------------------
            if ((desc.idVendor == VID) && (desc.idProduct == PID))
            {
                libusb_device_handle *handle;
                r = libusb_open(dev, &handle);
                if (LIBUSB_SUCCESS != r)
                    printf("libusb: Fails on device open : %s\n", libusb_error_name(r));
            
                //send USB command
                res = pictureFrameCmd( handle, cmd );
            }
        }

        usleep(500000);

        libusb_free_device_list(devs, 1);

        libusb_exit(NULL);
        return res;
}

bool pictureFrameCmd(libusb_device_handle *handle, unsigned short cmd)
{
    unsigned char buffer[2];
    int actlen;
    unsigned short status;

    buffer[0] = cmd & 0xFF;
    buffer[1] = (cmd >> 8) & 0xFF;

    // status = 1;
    // while (status)
    // {
    //     if (!jpgGetStatus(handle, &status))
    //     {
    //         printf("usbdev: Fails on GetStatus()\n");
    //         return false;
    //     }
    //     if (status)
    //     {
    //         printf("usbdev: Wait source in\n");
    //         usleep(100000); // 100ms delay
    //     }
    // }

    actlen = libusb_control_transfer(
        handle, // handle
        0x41,   // request_type
        0x90,   // bRequest,
        0,      // wValue,
        0,      // wIndex,
        buffer, // data
        2,      // wLength,
        500);

    // printf("SetResolution %d x %d, len: %d\n", width, height, actlen);

// #if 1
//     status = 1;
//     while (status)
//     {
//         if (!jpgGetStatus(handle, &status))
//             return false;
//         if (status)
//             usleep(100000); // 100ms delay
//     }
// #endif

    return (actlen > 0);
}

bool jpgSetQuantTable(libusb_device_handle *handle, unsigned char *table, unsigned long len, unsigned short index)
{
    int actlen;

    actlen = libusb_control_transfer(
        handle, // handle
        0x41,   // request_type
        0x83,   // bRequest,
        index,  // wValue,
        0,      // wIndex,
        table,  // data
        len,    // wLength,
        500);

    // printf("SetQuantTable buflen: %ld, len: %d\n", len, actlen);
    return (actlen > 0);
}

bool jpgSetResolution(libusb_device_handle *handle, unsigned short width, unsigned short height)
{
    unsigned char buffer[4];
    int actlen;
    unsigned short status;

    buffer[0] = width & 0xFF;
    buffer[1] = (width >> 8) & 0xFF;
    buffer[2] = height & 0xFF;
    buffer[3] = (height >> 8) & 0xFF;

    status = 1;
    while (status)
    {
        if (!jpgGetStatus(handle, &status))
        {
            printf("usbdev: Fails on GetStatus()\n");
            return false;
        }
        if (status)
        {
            printf("usbdev: Wait source in\n");
            usleep(100000); // 100ms delay
        }
    }

    actlen = libusb_control_transfer(
        handle, // handle
        0x41,   // request_type
        0x81,   // bRequest,
        0,      // wValue,
        0,      // wIndex,
        buffer, // data
        4,      // wLength,
        500);

    // printf("SetResolution %d x %d, len: %d\n", width, height, actlen);

#if 1
    status = 1;
    while (status)
    {
        if (!jpgGetStatus(handle, &status))
            return false;
        if (status)
            usleep(100000); // 100ms delay
    }
#endif

    return (actlen > 0);
}

bool jpgGetStatus(libusb_device_handle *handle, unsigned short *status)
{
    int actlen = libusb_control_transfer(
        handle,   // handle
        0xC1,     // request_type
        0x40,     // bRequest,
        0,        // wValue,
        0,        // wIndex,
        (unsigned char*)status, // data
        2,        // wLength,
        500);

    *status &= 0xFF;
    //printf("%s, status: 0x%02x, len: %d\n", __func__, *status, actlen);

    return (actlen > 0);
}

#endif
