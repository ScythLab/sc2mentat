// https://www.gamedev.net/forums/topic/496350-xaudio2-and-ogg/
// https://xiph.org/vorbis/
#pragma once
#include <xaudio2.h>
#include <vector>
#include <list>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// ƒлина буфера на 1 секунду (стерео, 44.1 к√ц, 16 бит на канал),
// не должна превышать XAUDIO2_MAX_BUFFER_BYTES
#define STREAMING_BUFFER_SIZE 44100 * 4 * 1
// Ќе должно превышать XAUDIO2_MAX_QUEUED_BUFFERS
#define MAX_BUFFER_COUNT 3

class CAudio //:
			 //public CBase
{
public:
	CAudio(LPCWSTR path);
	virtual ~CAudio(void);

	void initialiseAudio();
	bool isPlaying();
	void stop();
	bool play(bool loop = true);
	bool loadSound(const char* szSoundFilePath);
	void alterVolume(float fltVolume);
	void getVolume(float &fltVolume);
	void pause();
	bool update();
	void playAndWait();
	void waitPlaying();
	void clearQueue();
	void addToQueue(LPCSTR fileName);

private:
	IXAudio2* pXAudio2;
	IXAudio2MasteringVoice* pMasteringVoice;
	IXAudio2SourceVoice* pSourceVoice;
	std::list<char*> queue_;
	TCHAR path_[MAX_PATH];

	UINT32 flags;
	char buffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
	bool bFileOpened;
	OggVorbis_File vf;
	bool isRunning;
	bool boolIsPaused;
	bool bDone;
	bool bLoop;
	DWORD currentDiskReadBuffer;

	void resetParams();
	int readAndSubmintBuff();
};
