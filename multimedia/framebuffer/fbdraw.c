#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>

#define FBDEVICE "/dev/fb0"

typedef unsigned char ubyte;
struct fb_var_screeninfo vinfo;/* 프레임 버퍼 정보 처리를 위한 구조체 */

/* 색깔 지정하는 거 */
unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}
#if 1
static int drawpoint(int fd, int x, int y, unsigned short color)
{
    /* 색상 출력을 위한 위치 계산 : offset  = (X의_위치+Y의_위치x해상도의_넓이)x2  */
    int offset = (x + y*vinfo.xres)*2;
    lseek(fd, offset, SEEK_SET);
    write(fd, &color, 2); //2바이트 = 16비트 (1byte = 8 bit)
    return 0;
}

static void drawline(int fd, int start_x, int end_x, int y, unsigned short color){

    //for 루프로 점을 이어 선을 그린다
    for (int x = start_x; x < end_x ; x++){
        drawpoint(fd, x, y, color);
    }
}

static void drawface(int fd, int start_x, int end_x, int start_y, int end_y, unsigned short color){
    for (int y = start_y; y < end_y ; y++){
        drawline(fd, start_x, end_x, y, color);
    }
}

/* mmap() 사용해서 drawshpae하는 거 */
static void drawfacemmap(int fd, int start_x, int start_y, int end_x, int end_y,
unsigned short color){
    unsigned short* pfb;
    unsigned short byte_per_pixel = vinfo.bits_per_pixel/8.; //비트를 바이트로 바꿔주세요

    if (end_x == 0) end_x = vinfo.xres; 
    if (end_y == 0) end_y = vinfo.yres;

    // mmap() 함수를 이용해서 메모리맵 할당
    pfb = (unsigned short *)mmap(0, vinfo.xres * vinfo.yres * byte_per_pixel, PROT_READ | PROT_WRITE,
    MAP_SHARED, fd, 0);

    for (int x = start_x; x < end_x; x ++){ //어차피 포인터 계산이니까 걍 지가 알아서 넘김
        for (int y = start_y; y < end_y; y++){
            *(pfb + x + y*vinfo.xres) =  color;
        }
    }

    munmap(pfb, vinfo.xres * vinfo.yres * byte_per_pixel);
}

static void drawcircle(int fd, int center_x, int center_y, int radius, unsigned short color){
    
    int x = radius;
    int y = 0;
    int radiusError = 1-x;

    while (x>=y) {
        drawpoint(fd, x+center_x, y+center_y, color);
        drawpoint(fd, y+center_x, x+center_y, color);
        drawpoint(fd, -x+center_x, y+center_y, color);
        drawpoint(fd, -y+center_x, x+center_y, color);
        drawpoint(fd, -x+center_x, -y+center_y, color);
        drawpoint(fd, -y+center_x, -x+center_y, color);
        drawpoint(fd, x+center_x, -y+center_y, color);
        drawpoint(fd, y+center_x, -x+center_y, color);
        
        y++;
        if (radiusError<0){
            radiusError += 2*y+1;
        } else {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }
}
#else
/* 점을 그린다. */
static void drawpoint(int fd, int x, int y, ubyte r, ubyte g, ubyte b)
{
    ubyte a = 0xFF;

    /* 색상 출력을 위한 위치를 구한다. */
    /* offset = (X의_위치 + Y의_위치 × 해상도의_넓이) × 색상의_바이트_수 */
    int offset = (x + y*vinfo.xres)*vinfo.bits_per_pixel/8.; 
    lseek(fd, offset, SEEK_SET);
    write(fd, &b, 1);
    write(fd, &g, 1);
    write(fd, &r, 1);
    write(fd, &a, 1);
}
#endif

int main(int argc, char **argv)
{
    int fbfd, status, offset;

    fbfd = open(FBDEVICE, O_RDWR); /* 사용할 프레임 버퍼 디바이스를 연다. */
    if(fbfd < 0) {
        perror("Error: cannot open framebuffer device");
        return -1;
    }

    /* 현재 프레임 버퍼에 대한 화면 정보를 얻어온다. */
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error reading fixed information");
        return -1;
    }
#if 1
    // drawpoint(fbfd, 50, 50, makepixel(255, 0, 0));            /*  Red 점을 출력 */
    // drawpoint(fbfd, 100, 100, makepixel(0, 255, 0));        /*  Green 점을 출력 */
    // drawpoint(fbfd, 150, 150, makepixel(0, 0, 255));        /*  Blue 점을 출력 */

   // drawline(fbfd, 50, 100, 50, makepixel(255, 0, 0));            /*  Red 선을 출력 */
   // drawline(fbfd, 100, 150, 100, makepixel(0, 255, 0));        /*  Green 선을 출력 */
   // drawline(fbfd, 150, 200, 150, makepixel(0, 0, 255));        /*  Blue 선을 출력 */

    /* 프랑스 국기*/
    //  drawface (fbfd, 0, vinfo.xres/3, 0, vinfo.yres, makepixel(0, 0, 255)); /* Blue 면 출력 */
    //  drawface (fbfd, vinfo.xres/3, vinfo.xres/3*2, 0, vinfo.yres, makepixel(255, 255, 255)); /* white 면 출력 */
    //  drawface (fbfd, vinfo.xres/3*2, vinfo.xres, 0, vinfo.yres, makepixel(255, 0, 0)); /* Red 면 출력 */

    
     /* 우크라이나 국기 */
    // drawface(fbfd, 0, vinfo.xres, 0, vinfo.yres/2, makepixel(255, 215, 0));
    // drawface(fbfd, 0, vinfo.xres, vinfo.yres/2, vinfo.yres, makepixel(0, 87, 183));

    // drawcircle(fbfd, 200, 200, 100, makepixel(255, 0, 255));
    
    // printf("%d %d", vinfo.xres, vinfo.yres );
    drawfacemmap (fbfd, 50, 100, 100, 200, makepixel(255, 0, 255));


#else
    drawpoint(fbfd, 50, 50, 255, 0, 0); /* 빨간색(Red) 점을 출력 */
    drawpoint(fbfd, 100, 100, 0, 255, 0); /* 초록색(Green) 점을 출력 */
    drawpoint(fbfd, 150, 150, 0, 0, 255); /* 파란색(Blue) 점을 출력 */
#endif

    close(fbfd); /* 사용이 끝난 프레임 버퍼 디바이스를 닫는다. */

    return 0;
}
