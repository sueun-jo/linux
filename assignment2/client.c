#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/videodev2.h>

#define VIDEO_DEVICE        "/dev/video0"
#define FRAMEBUFFER_DEVICE  "/dev/fb0"
#define WIDTH               640
#define HEIGHT              480
//read의 반환값은 buffer의 갯수

#define SERVER_PORT 54321 //server port

/* 클라이언트의 카메라 데이터를 서버 쪽에 보내고 서버는 그걸 framebuffer로 출력 */
static struct fb_var_screeninfo vinfo;

/* server쪽으로 넘겨줌 //display_frame()
static struct fb_var_screeninfo vinfo;

void display_frame(uint16_t *fbp, uint8_t *data, int width, int height) 
{
  int x_offset = (vinfo.xres - width) / 2;
  int y_offset = (vinfo.yres - height) / 2;

  // YUYV -> RGB565 변환하여 프레임버퍼에 출력
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x += 2) {
      uint8_t Y1 = data[(y * width + x) * 2];
      uint8_t U = data[(y * width + x) * 2 + 1];
      uint8_t Y2 = data[(y * width + x + 1) * 2];
      uint8_t V = data[(y * width + x + 1) * 2 + 1];

      int R1 = Y1 + 1.402 * (V - 128);
      int G1 = Y1 - 0.344136 * (U - 128) - 0.714136 * (V - 128);
      int B1 = Y1 + 1.772 * (U - 128);

      int R2 = Y2 + 1.402 * (V - 128);
      int G2 = Y2 - 0.344136 * (U - 128) - 0.714136 * (V - 128);
      int B2 = Y2 + 1.772 * (U - 128);

      // RGB565 포맷으로 변환 (R: 5비트, G: 6비트, B: 5비트)
      uint16_t pixel1 = ((R1 & 0xF8) << 8) | ((G1 & 0xFC) << 3) | (B1 >> 3);
      uint16_t pixel2 = ((R2 & 0xF8) << 8) | ((G2 & 0xFC) << 3) | (B2 >> 3);

      fbp[(y + y_offset) * vinfo.xres + (x + x_offset)] = pixel1;
      fbp[(y + y_offset) * vinfo.xres + (x + x_offset + 1)] = pixel2;
    }
  }
}
*/

int main (int argc, char **argv){

    int my_socket;

    struct sockaddr_in server_addr;

    /* ./client.out 127.0.0.1 입력 예외 처리 */
    if (argc != 2){
        printf("Your Command Something Wrong\n");
        return -1;
    }

    /* socket 생성 */
    if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("socket error : ");
        return -1;
    }
    printf("My socket is established\n");
    
    /* 서버 주소 생성 */
    memset (&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons (SERVER_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    
    /* 서버에 connect 요청 */
    if (connect (my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror ("Failed to connect to server");
        return -1;
    }

    /* open camera */
    int cam_fd = open(VIDEO_DEVICE, O_RDWR);
    if (cam_fd == -1) {
        perror("Failed to open video device");
        return -1;
    }

    /* format struct */
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    
    /* set format */
    if (ioctl(cam_fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("Failed to set format");
        close(cam_fd);
        return 1;
    }

    /* 버퍼설정
    struct v4l2_requestbuffers req;
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("Failed to request buffer");
        close(fd);
        return 1;
    }
    */

    // buffer 동적 할당 및 실패 시 처리
    unsigned char *buffer = malloc(fmt.fmt.pix.sizeimage);
    if (!buffer) {
        perror("Failed to allocate buffer");
        close(cam_fd);
        return 1;
    }
    

    while (1) {
        int bytes_read = read(cam_fd, buffer, fmt.fmt.pix.sizeimage); // 과제 : read한 부분 서버로 보내기
        if (bytes_read == -1) {
        perror("Failed to read frame");
        break;
        }

        // buffer에 읽어온 프레임 데이터를 처리
        printf("Captured frame size: %d bytes\n", bytes_read);
        int send_data = send(my_socket, buffer, bytes_read, 0);
        if (send_data == -1){
            perror("Failed to send frame");
            break;
        }

    }

    free(buffer);
    close(cam_fd);
    close (my_socket);
    return 0;
}