#include <stdint.h>
#include <iostream>
#include <assert.h>
#include <string.h>
using namespace std;

#pragma pack(1) // 禁用結構體對齊，確保讀取的資料大小和 BMP 格式一致 (測試過如果沒使用，BITMAPFILEHEADER會是16bytes)

// 定義 BMP 文件頭結構
typedef struct {
    uint16_t bfType;        // 文件類型，應該為 'BM'，表示位圖
    uint32_t bfSize;        // 文件大小，以字節為單位
    uint16_t bfReserved1;   // 保留字段，通常設為 0
    uint16_t bfReserved2;   // 保留字段，通常設為 0
    uint32_t bfOffBits;     // 位圖數據的偏移量，即從文件頭開始到實際像素數據的距離
} BITMAPFILEHEADER;

// 定義 BMP 信息頭結構
typedef struct {
    uint32_t biSize;            // 信息頭的大小，以字節為單位
    int32_t biWidth;            // 位圖的寬度，以像素為單位
    int32_t biHeight;           // 位圖的高度，以像素為單位（正值表示從上到下的位圖，負值表示從下到上的位圖）
    uint16_t biPlanes;          // 色彩平面數，必須為 1
    uint16_t biBitCount;        // 每個像素的位數，常見值為 1（單色）、4（16 色）、8（256 色）、24（真彩色）
    uint32_t biCompression;     // 壓縮類型，0 表示不壓縮
    uint32_t biSizeImage;       // 位圖數據的大小，以字節為單位（對於不壓縮的位圖可設為 0）
    int32_t biXPelsPerMeter;     // 水平解析度，以像素每米為單位
    int32_t biYPelsPerMeter;     // 垂直解析度，以像素每米為單位
    uint32_t biClrUsed;         // 使用的顏色數，對於真彩色圖像可以設為 0
    uint32_t biClrImportant;    // 重要顏色數，對於真彩色圖像可以設為 0
} BITMAPINFOHEADER;

unsigned char* load_bmp_image(const char* file_name, int *width, int *height);

int main() {
    const char* bmp_name = "../src/testBMP.bmp";

    int width = 0, height = 0;
    unsigned char* bmp_data = load_bmp_image(bmp_name, &width, &height);

    

    return 0;
}

unsigned char* load_bmp_image(const char* file_name, int *width, int *height) {
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("無法開啟文件");
        return NULL;
    }

    BITMAPFILEHEADER fileHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    if (fileHeader.bfType != 0x4D42) { // 檢查是否為 BMP 格式（'BM' 標識）
        printf("此文件不是 BMP 圖片\n");
        fclose(file);
        return NULL;
    }

    BITMAPINFOHEADER infoHeader;
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);
    printf("寬度: %d\n", infoHeader.biWidth);
    printf("高度: %d\n", infoHeader.biHeight);
    printf("色深: %d 位元\n", infoHeader.biBitCount);
    *width = infoHeader.biWidth;  // set width & height
    *height = infoHeader.biHeight;

    // 移動到像素資料起始位置
    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    // 分配空間並讀取像素資料
    int imageSize = infoHeader.biWidth * abs(infoHeader.biHeight) * ((infoHeader.biBitCount + 7 ) / 8);  // width * height = 像素數量 => 在乘上每個像素的位元數（除8是從bits變成bytes)
    uint8_t *pixels = (uint8_t *)malloc(imageSize);
    assert(pixels);
    
    size_t result = fread(pixels, imageSize, 1, file);
    if (result != 1) {
        perror("Error reading pixel data");
        free(pixels); // 釋放內存
        fclose(file);
        return NULL;
    }

    fclose(file);

    return pixels;
}