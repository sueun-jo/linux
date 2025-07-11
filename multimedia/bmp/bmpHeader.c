#include <stdio.h>
#include <stdlib.h>

#include "bmpHeader.h"

int readBmp(char *filename, unsigned char **data, int *cols, int *rows){
    BITMAPFILEHEADER bmpHeader;
    BITMAPINFOHEADER bmpInfoHeader;
    FILE *fp;

    //bmp 파일을 오픈한다 
    fp = fopen(filename, "rb");
    if(fp == NULL){
        perror("ERROR\n");
        return -1;
    }

    //  BITMAPFILEHDEAR 구조체의 데이터
    fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);  

    // BITMAPINFOHEADER 구조체의 데이터 
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1,  fp); 

    //트루 컬러 지원 안하면 표시 못함

    if (bmpInfoHeader.biBitCount != 24){
        perror("This image file doesn't supports 24 bit color\n");
        fclose(fp);
        return -1;
    }

    //이미지에서 해상도 정보 가져옴 
    *cols = bmpInfoHeader.biWidth; // 가로 
    *rows = bmpInfoHeader.biHeight; // 세로

    // 이미지 해상도 (넓이*깊이)
    
    printf("resolution: %d x %d\n", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
    printf("Bit count: %d\n", bmpInfoHeader.biBitCount); //픽셀당 비트 수 (색상)

    //실제 이미지가 있는 데이터 위치를 계산해서 가져옴
    fseek(fp, bmpHeader.bfOffBits, SEEK_SET);
    fread(*data, 1, bmpHeader.bfSize-bmpHeader.bfOffBits, fp);

    fclose(fp); //사용 끝나면 닫아줌

    return 0;
}