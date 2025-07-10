#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#define FBDEVICE "/dev/fb0"

int main (int argc, char **argv){
    
    int fbfd = 0;
    
    //프레임 버퍼 정보 처리를 위한 구조체
    struct fb_var_screeninfo vinfo, old_vinfo;
    struct fb_fix_screeninfo finfo; 

    //프레임 버퍼를 위한 디바이스 파일을 읽기와 쓰기 모드로 연다
    fbfd = open(FBDEVICE, O_RDWR);
    if(fbfd < 0){
        perror("Error: cannot open framebuffer device");
        return -1;
    }

    //현재 프레임 버퍼에 대한 화면 정보를 얻어온다
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) < 0){
        perror("Error reading fixed information");
        return -1;
    }

    //현재 프레임 버퍼에 대한 가상 화면 정보를 얻어온다
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0){
        perror("error reading variable information");
        return -1;
    }

    //현재 프레임 버퍼에 대한 정보를 추가한다
    printf("Resolution : %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    printf("Virtual Resolution : %dx%d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("Length of frame buffer memory: %d\n", finfo.smem_len);

    //이전 값 백업
    old_vinfo = vinfo;

    //프레임 버퍼에 새로운 해상도 설정

    vinfo.xres = 800;
    vinfo.yres = 600;

    if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) < 0){
        perror("fbdev ioctl(PUT)");
        return -1;
    }
    
    //설정한 프레임 버퍼에 대한 정보를 출력한다
    printf("New Resolution: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    getchar();
    ioctl(fbfd, FBIOPUT_VSCREENINFO, &old_vinfo);

    close(fbfd);

    return 0;



}