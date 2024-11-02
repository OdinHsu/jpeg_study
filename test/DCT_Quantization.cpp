#include <stdio.h>
#include <math.h>

#define N 8 // DCT 块的大小

void dct(int input[N][N], int output[N][N]);

int main() {
    int block[N][N] = {
        {139, 148, 150, 149, 155, 164, 165, 168},
        { 98, 115, 130, 135, 143, 146, 142, 147},
        { 89, 110, 125, 128, 129, 121, 104, 106},
        { 96, 116, 128, 132, 134, 132, 113, 109},
        {111, 125, 127, 131, 137, 137, 120, 110},
        {122, 126, 126, 131, 133, 131, 126, 112},
        {133, 134, 136, 138, 140, 144, 141, 139},
        {138, 139, 139, 139, 140, 146, 148, 147}
    };

    int dct_output[N][N];
    dct(block, dct_output);

    // 打印 DCT 输出
    printf("DCT 结果:\n");
    for (int u = 0; u < N; u++) {
        for (int v = 0; v < N; v++) {
            printf("%4d ", dct_output[u][v]);
        }
        printf("\n");
    }

    return 0;
}

// 计算 DCT
void dct(int input[N][N], int output[N][N]) {
    for (int u = 0; u < N; u++) {
        for (int v = 0; v < N; v++) {
            double sum = 0.0;

            // 遍歷原始圖像的每個像素
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < N; y++) {
                    sum += (input[x][y]-128) * cos(((2 * x + 1) * u * M_PI) / (2 * N)) * cos(((2 * y + 1) * v * M_PI) / (2 * N));
                }
            }

            // 計算 C_u 和 C_v
            double Cu = (u == 0) ? 1.0 / sqrt(2) : 1.0;
            double Cv = (v == 0) ? 1.0 / sqrt(2) : 1.0;

            // 最終的 DCT 係數
            output[u][v] = 0.25 * Cu * Cv * sum;
        }
    }
}
