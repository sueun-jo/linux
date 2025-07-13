#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/videodev2.h>

#define VIDEO_DEVICE        "/dev/video0"
#define FRAMEBUFFER_DEVICE  "/dev/fb0"
#define WIDTH               640
#define HEIGHT              480
#define FRAME_SIZE          (WIDTH * HEIGHT * 2)
#define SERVER_PORT 54321 // server port num
#define MAX_CLIENT 2

#define CLAMP(x) ((x)<0?0:((x)>255?255:(x)))

int client_socket;

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

      R1 = CLAMP(R1);
      G1 = CLAMP(G1);
      B1 = CLAMP(B1);
      R2 = CLAMP(R2);
      G2 = CLAMP(G2);
      B2 = CLAMP(B2);

      // RGB565 포맷으로 변환 (R: 5비트, G: 6비트, B: 5비트)
      uint16_t pixel1 = ((R1 & 0xF8) << 8) | ((G1 & 0xFC) << 3) | (B1 >> 3);
      uint16_t pixel2 = ((R2 & 0xF8) << 8) | ((G2 & 0xFC) << 3) | (B2 >> 3);

      fbp[(y + y_offset) * vinfo.xres + (x + x_offset)] = pixel1;
      fbp[(y + y_offset) * vinfo.xres + (x + x_offset + 1)] = pixel2;
    }
  }
}

int main (int argc, char **argv){

    /* listen_socket에 socket 열고 bind, listen accept 처리 */
    int listen_socket; //listen_socket은 연결대기용, client_socket(전역)은 client와 통신용
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    listen_socket = socket (AF_INET, SOCK_STREAM, 0); //tcp socket 통신 할게요
    if (listen_socket < 0){
        perror ("socket error : ");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* bind로 주소 설정 */
    if (bind (listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error : ");
        return -1;
    }

    /* 동시 접속 클라이언트 처리 대기 큐 listen */
    listen (listen_socket, MAX_CLIENT); 
    printf("[Server Info] Server is Listening on Port : %d ... \n", SERVER_PORT);

    
    /* accept : 클라이언트가 요청해서 생긴 socket이기에 client_socket이라 명명 */
    client_socket = accept (listen_socket, (struct sockaddr*)&client_addr, &addrlen);
    if (client_socket < 0)
        perror ("accept error");

    /* server쪽 framebuffer에 그려야 되니까 framebuffer_fd는 erver쪽에서 열어야 됨 */
    int fb_fd = open(FRAMEBUFFER_DEVICE, O_RDWR);
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        exit(1);
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb_fd);
        exit(1);
    }

    uint32_t fb_width = vinfo.xres;
    uint32_t fb_height = vinfo.yres;
    uint32_t screensize = fb_width * fb_height * vinfo.bits_per_pixel / 8;
    uint16_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    
    if ((intptr_t)fbp == -1) {
        perror("Error mapping framebuffer device to memory");
        close(fb_fd);
        exit(1);
    }
    /* ---------------------------------------------- */

    unsigned char *buffer = malloc(FRAME_SIZE);
    
    while (1) {
        int bytes_received = 0;
        unsigned char *bufp = buffer;

        while (bytes_received < FRAME_SIZE) {
            int n = recv(client_socket, bufp, FRAME_SIZE - bytes_received, 0);
            if (n < 0) { // recv한 값이 음수면 오류
                
                perror("Failed to recv");
                break;
            } else if (n==0){ // recv한 값이 0이면 connection 종료
                perror ("Connection closed");
                goto out; //단순 break로는 루프문 한개밖에 못나감
            }
            bytes_received += n; // 몇바이트 받았는지
            bufp += n; // 포인터 이동
        }
        if (bytes_received == FRAME_SIZE) {
            // 모든 데이터 제대로 받음
            display_frame(fbp, buffer, WIDTH, HEIGHT);
        }
    }
    out: //나가면 buffer 정리 및 소켓 닫아줌
    free(buffer);
    close (fb_fd);
    close (client_socket);
    return 0;

}