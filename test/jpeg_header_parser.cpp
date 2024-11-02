#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
 
/* JPEG marker definitions. refer to itu-t81 Table B.1 */
 
/* Start Of Frame markers, non-differential, Huffman coding */
#define JPEG_MARKER_SOF0	0xc0	/* Baseline DCT */
#define JPEG_MARKER_SOF1	0xc1	/* Extended sequential DCT */
#define JPEG_MARKER_SOF2	0xc2	/* Progressive DCT */
#define JPEG_MARKER_SOF3	0xc3	/* Lossless(sequential) */
 
/* Start Of Frame markers, differential, Huffman coding */
#define JPEG_MARKER_SOF4	0xc5	/* Differential sequential DCT */
#define JPEG_MARKER_SOF5	0xc6	/* Differential progressive DCT */
#define JPEG_MARKER_SOF6	0xc7	/* Differential Lossless(sequential) */
 
/* Start Of Frame markers, non-differential, arithmetic coding */
#define JPEG_MARKER_JPG		0xc8	/* Reserved for JPEG extensions */
#define JPEG_MARKER_SOF9	0xc9	/* Extended sequential DCT */
#define JPEG_MARKER_SOF10	0xca	/* Progressive DCT */
#define JPEG_MARKER_SOF11	0xcb	/* Lossless(dequential) */
 
/* Start Of Frame markers, differential, arithmetic coding */
#define JPEG_MARKER_SOF13	0xcd	/* Differential sequential DCT */
#define JPEG_MARKER_SOF14	0xce	/* Differential progressive DCT */
#define JPEG_MARKER_SOF15	0xcf	/* Differential lossless(sequential) */
 
/* Restart interval termination */
#define JPEG_MARKER_RST0	0xd0	/* Restart ...*/
#define JPEG_MARKER_RST1	0xd1
#define JPEG_MARKER_RST2	0xd2
#define JPEG_MARKER_RST3	0xd3
#define JPEG_MARKER_RST4	0xd4
#define JPEG_MARKER_RST5	0xd5
#define JPEG_MARKER_RST6	0xd6
#define JPEG_MARKER_RST7	0xd7
#define JPEG_MARKER_RST8	0xd8
#define JPEG_MARKER_RST9	0xd9
 
/* Huffman table specification */
#define JPEG_MARKER_DHT		0xc4	/* Define Huffman table(s) */
 
/* Arithmetic coding conditioning specification */
#define JPEG_MARKER_DAC		0xcc	/* Define arithmetic coding conditioning(s) */
 
/* Other markers */
#define JPEG_MARKER_SOI		0xd8	/* Start of image */
#define JPEG_MARKER_EOI		0xd9	/* End of image */
#define JPEG_MARKER_SOS		0xda	/* Start of scan */
#define JPEG_MARKER_DQT		0xdb	/* Define quantization table(s) */
#define JPEG_MARKER_DNL		0xdc	/* Define number of lines */
#define JPEG_MARKER_DRI		0xdd	/* Define restart interval */
#define JPEG_MARKER_DHP		0xde	/* Define hierarchial progression */
#define JPEG_MARKER_EXP		0xdf	/* Expand reference component(s) */
#define JPEG_MARKER_APP0	0xe0	/* Application marker, JFIF/AVI1... */
#define JPEG_MARKER_APP1	0xe1	/* EXIF Metadata etc... */
#define JPEG_MARKER_APP2	0xe2	/* Not common... */
#define JPEG_MARKER_APP13	0xed	/* Photoshop Save As: IRB, 8BIM, IPTC */
#define JPEG_MARKER_APP14	0xee	/* Not common... */
#define JPEG_MARKER_APP15	0xef	/* Not common... */
 
static const char *seg_name[] = {
	"Baseline DCT; Huffman",
	"Extended sequential DCT; Huffman",
	"Progressive DCT; Huffman",
	"Spatial lossless; Huffman",
	"Huffman table",
	"Differential sequential DCT; Huffman",
	"Differential progressive DCT; Huffman",
	"Differential spatial; Huffman",
	"[Reserved: JPEG extension]",
	"Extended sequential DCT; Arithmetic",
	"Progressive DCT; Arithmetic",
	"Spatial lossless; Arithmetic",
	"Arithmetic coding conditioning",
	"Differential sequential DCT; Arithmetic",
	"Differential progressive DCT; Arithmetic",
	"Differential spatial; Arithmetic",
	"Restart",
	"Restart",
	"Restart",
	"Restart",
	"Restart",
	"Restart",
	"Restart",
	"Restart",
	"Start of image",
	"End of image",
	"Start of scan",
	"Quantisation table",
	"Number of lines",
	"Restart interval",
	"Hierarchical progression",
	"Expand reference components",
	"JFIF header",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: application extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"[Reserved: JPEG extension]",
	"Comment",
	"[Invalid]",
};
 
void show_segment(uint32_t marker)
{
	int32_t index = marker - 0xc0;
 
	if (index < 0 || index >= sizeof(seg_name)/sizeof(char *))
		return;
 
	printf("%s\n", seg_name[index]);
}
 
void usage(void) {
	printf("usage: jpeg_parse file\n");
 
	return;
}
 
int parse_jpeg_header(uint8_t *data, uint32_t length, uint32_t *width, uint32_t *height, uint32_t *format)
{
	uint8_t *start, *end, *cur;
	int found = 0;
 
	start = data;
	end = data + length;
	cur = start;
 
	// printf("start - %p, cur - %p, end - %p\n", start, cur, end);
 
#define READ_U8(value) do \
	{ \
		(value) = *cur; \
		++cur; \
	} while (0)
 
#define READ_U16(value) do \
	{ \
		uint16_t w = *((uint16_t *)cur); \
		cur += 2; \
		(value) = (((w & 0xff) << 8) | ((w & 0xff00) >> 8)); \
		/* printf("w = 0x%x, value = 0x%x\n", w, value); */\
	} while (0)
 
	while (cur < end) {
		uint8_t marker;
 
		if (*cur++ != 0xff) {
			printf("%2x%2x->%2x%2x\n", *(cur-2), *(cur-1), *cur, *(cur+1));;
			printf("cur pos: 0x%lx\n", cur-start);
			break;
		}
 
		READ_U8(marker);
 
		if (marker == JPEG_MARKER_SOS)
			break;
 
		show_segment(marker);
 
		switch (marker) {
		case JPEG_MARKER_SOI:
			break;
		case JPEG_MARKER_DRI:
			cur += 4;	/* |length[0..1]||rst_interval[2..3]|*/
			break;
		case JPEG_MARKER_SOF2:
			fprintf(stderr, "progressive JPEGs not suppoted\n");
			break;
		case JPEG_MARKER_SOF0: {
			uint16_t length;
			uint8_t sample_precision;
 
			READ_U16(length);
			length -= 2;
 
			READ_U8(sample_precision);
			printf("sample_precision = %d\n", sample_precision);
 
			READ_U16(*height);
			READ_U16(*width);
			length -= 5;
 
			cur += length;
 
			found = 1;
			break;
		}
		default: {
			uint16_t length;
			READ_U16(length);
			// printf("cur: 0x%lx, length: 0x%x\n", cur-start, length);
			length -= 2;
			cur += length;
			break;
		}
		}
	}
 
	printf("parse jpeg header finish\n");
 
	return found;
}
 
int main(int argc, char *argv[])
{
	int fd, ret;
	uint32_t f_len;
	struct stat stat;
	uint8_t *bs_mem;
	uint32_t width, height, format;
 
	if (argc != 2)
		usage();
 
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open failed. error:%d-%m\n", errno);
		return -1;
	}
 
	ret = fstat(fd, &stat);
	if (ret < 0)
		goto err1;
 
	f_len = stat.st_size;
	printf("file length %d bytes\n", f_len);
 
	/* allocate mem for jpeg buffer */
	bs_mem = (uint8_t *)malloc(f_len);
	if (!bs_mem)
		goto err1;
 
	if (read(fd, bs_mem, f_len) < 0) {
		fprintf(stderr, "read %d data failed\n", f_len);
		goto err2;
	}
 
	/* parse jpeg header */
	if (parse_jpeg_header(bs_mem, f_len, &width, &height, &format))
		printf("picture width: %d, height: %d\n", width, height);
 
err2:
	free(bs_mem);
 
err1:
	close(fd);
 
	return 0;
}