#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include "wavfile.h"

WAVHEADER wavheader;

int main(int argc, char** argv)
{
	int fd = -1; // WAV 파일을 위한 file descriptor
	int rc, buf_size, dir;
	long count, remain;
	int channels, format; //오디오 디바이스 설정을 위한 채널과 포맷
	unsigned int val;
	char *buffer;
	
	snd_pcm_t *handle; //ALSA 디바이스 설정을 위한 구조체
	snd_pcm_hw_params_t *params; //오디오 디바이스 설정을 위한 구조체
	snd_pcm_uframes_t frames; //오디오 프레임 설정을 위한 구조체

	if(argc <= 1) {
		printf("usage : %s filename\n", argv[0]);
		exit(-1);
	} else {
		printf("Playing file : %s\n", argv[1]);
	}

	/* wAV 파일 열기 */
	if((fd = open(argv[1], O_RDONLY)) == -1) {
		printf("Could not open the specified wave file : %s\n", argv[1]);
		exit(-1);
	}

	// Read Head of wave file
	if((count = read(fd, &wavheader, sizeof(wavheader))) < 1) {
		printf("Could not read wave data\n");
		exit(-1);
	}

	// 출력을 위한 ALSA PCM 디바이스 열기
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if(rc < 0){
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		return -1;
	}

	//오디오 디바이스에 오디오 디바이스의 매겨변수 설정을 위한 공간 확보
	snd_pcm_hw_params_alloca(&params); 

	//기본값으로 초기화
	snd_pcm_hw_params_any(handle, params);

	//헤더에서 채널에 대한 정보를 가져와서 출력하고 설정하기
	channels = wavheader.nChannels;
	printf("Wave Channel Mode : %s\n", (channels-1)? "Stereo" : "Mono");
	snd_pcm_hw_params_set_channels(handle, params, channels);
	
	//오디오 포맷 설정
	printf("Wave Bytes : %d\n", wavheader.nblockAlign);
	switch(wavheader.nblockAlign) {
		case 1: //모노 8비트
			format = SND_PCM_FORMAT_U8;
			break;
		case 2:
			format = (channels==1)? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U8;
			break;
		case 4:
			printf("Stereo Wave file\n");
			format = SND_PCM_FORMAT_S16_LE;
			break;
		default:
			printf("Unknown byte rate for sound\n");
			return -1;
	}
	
	//인터리브드 모드로 설정
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	
	//오디오 디바이스의 포맷 설정
	snd_pcm_hw_params_set_format(handle, params, format);
	 
	//오디오 디바이스 샘플링 레이트 설정
	printf("Wave Sampling Rate : 0x%d\n", wavheader.sampleRate);

	val = wavheader.sampleRate;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    rc = snd_pcm_hw_params(handle, params);
    if(rc < 0 ){
    	fprintf(stderr,"Unable to set hw parameters: %s\n", snd_strerror(rc));
        goto END;
    }

    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    buf_size = frames * channels * ((format == SND_PCM_FORMAT_S16_LE)?2:1);
	buffer = (char*)malloc(buf_size);

	// ALSA 주기 시간 가져오기
    snd_pcm_hw_params_get_period_time(params, &val, &dir);

    remain = wavheader.dataLen;
    do{
    buf_size = (remain > buf_size)? buf_size: remain;
    if((count = read(fd, buffer, buf_size))<=0) break;
    remain -= count;

    int frame_bytes = channels * ((format == SND_PCM_FORMAT_S16_LE) ? 2 : 1);
	int actual_frames = count / frame_bytes;
	rc = snd_pcm_writei(handle, buffer, actual_frames);
    if(rc==-EPIPE){
        fprintf(stderr, "Underrun occurred\n");
        snd_pcm_prepare(handle);
    }else if(rc<0){
        fprintf(stderr, "error form write: %s\n", snd_strerror(rc));
    }else if(rc != (int)frames){
        fprintf(stderr, "short write, write %d frames\n", rc);
    }
    }while(count == buf_size);

END:
    close(fd);
    sleep(1);
    snd_pcm_drain(handle);
    snd_pcm_close(handle); //사용 끝난 디바이스 닫기

    free(buffer);

    return 0;
}
