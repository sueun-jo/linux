#include <wiringPi.h>
#include <softTone.h>

#define SPKR 6

#define NOTE_C 261
#define NOTE_D 294
#define NOTE_E 330
#define NOTE_F 349
#define NOTE_G 392
#define NOTE_A 440

int notes[] = {
  NOTE_C, NOTE_E, NOTE_F,       // 한 번 더
  NOTE_C, NOTE_E, NOTE_F,       // 나 에 게
  NOTE_E, NOTE_G, NOTE_A, NOTE_G, NOTE_F, NOTE_E, NOTE_C, 
  NOTE_E, NOTE_F, NOTE_G,       // 거친 파
  NOTE_A, NOTE_G, NOTE_F, NOTE_E, NOTE_D, NOTE_E, NOTE_F, NOTE_G, 0
};

int noteDuration[] = {
  4, 4, 2,    // 한 번 더
  8, 8, 4,    // 나 에 게
  8, 8, 8, 8, 4, 4, 4,  // 질풍같은 용기를
  4, 4, 4,    // 거친 파
  4, 8, 8, 4, 4, 4, 4, 4, 4  // 도에도 굴하지 않게
};

int musicPlay()
{
	softToneCreate(SPKR);
	int length = sizeof(notes)/sizeof(notes[0]);
	
	for (int i=0; i<length; i++){
		softToneWrite(SPKR, notes[i]);
		int duration = 1000/ noteDuration[i];
		delay(duration*1.3); 
	}
	
	return 0;
}

int main (){
	wiringPiSetup();
	musicPlay();
	return 0;
}
