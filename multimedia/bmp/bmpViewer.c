#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "bmpHeader.h"

#define FBDEVFILE        "/dev/fb0"
#define LIMIT_UBYTE(n) (n>UCHAR_MAX)?UCHAR_MAX:(n<0)?:n
typedef unsigned char ubyte;

extern int readBmp(char *filename, ubyte **pData, int *cols, int *rows);

int main(int argc, char **argv){
    int cols, rows, color = 24;
    ubyte r,g,b,a = 255;
    ubyte *pData;
    unsigned short *pBmpData, *pFbMap;
    struct fb_var_screeninfo vinfo;
    int fbfd;

    if(argc!=2){
        printf("Usage : ./%s xxx.bpm\n",argv[0]);
        return -1;
    }

    fbfd = open(FBDEVFILE, O_RDWR);
    if(fbfd<0){
        perror("open()");
        return -1;
    }

    if(ioctl(fbfd,FBIOGET_VSCREENINFO,&vinfo)<0){
        perror("ioctl() fail");
        return -1;
    }
    pBmpData = (unsigned short*)malloc(vinfo.xres*vinfo.yres*sizeof(unsigned short)*2);
    pData = (ubyte*)malloc(vinfo.xres*vinfo.yres*sizeof(ubyte)*3);
    pFbMap = (unsigned short*)mmap(0,vinfo.xres*vinfo.yres*sizeof(unsigned short)*2,PROT_READ|PROT_WRITE, MAP_SHARED,fbfd,0);
    if((unsigned)pFbMap == (unsigned)-1){
        perror("mmap()");
        return -1;
    }

    if(readBmp(argv[1],&pData,&cols,&rows)<0){
        perror("readBmp()");
        return -1;
    }

    for(int y=0, k,total_y; y<rows; y++){
        k = (rows-y-1)*cols*color/8;
        for(int x = 0; x<cols; x++){
            b = LIMIT_UBYTE(pData[k+x*color/8+0]);
            g = LIMIT_UBYTE(pData[k+x*color/8+1]);
            r = LIMIT_UBYTE(pData[k+x*color/8+2]);

            unsigned short color_make = ((r>>3)<<11|((g>>2)<<5|(b>>3)));

            *(pBmpData + x + y*vinfo.xres)= color_make;
        }
    }

    memcpy(pFbMap,pBmpData,vinfo.xres*vinfo.yres*2);

    munmap(pFbMap,vinfo.xres*vinfo.yres*2);

    free(pBmpData);
    free(pData);

    close(fbfd);

    return 0;
}