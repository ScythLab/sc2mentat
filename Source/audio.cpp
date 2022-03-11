#include "audio.h"
#include "tools.h"
#include "log.h"

#pragma comment(lib, "libogg.lib")
#pragma comment(lib, "libvorbis.lib")
#pragma comment(lib, "libvorbisfile.lib")

//OGG handling code is from a tutorial @ www.flipcode.com

//Built with:
//OGG version 1.1.3
//Vorbis version 1.2.0

CAudio::CAudio(LPCWSTR path)
{
	pXAudio2 = NULL;
	pMasteringVoice = NULL;
	pSourceVoice = NULL;
	wcscpy(path_, path);

	resetParams();

	// В некоторых случаях этот флаг приводит к ошибкам в XAudio2Create.
//#ifdef _DEBUG
//	flags |= XAUDIO2_DEBUG_ENGINE;
//#endif

	CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

CAudio::~CAudio(void)
{
	if (pSourceVoice != NULL)
	{
		pSourceVoice->Stop(0);
		pSourceVoice->DestroyVoice();
	}

	if (pMasteringVoice != NULL)
		pMasteringVoice->DestroyVoice();

	SafeRelease(pXAudio2);

	if (bFileOpened)
		ov_clear(&vf);

	CoUninitialize();
}

void CAudio::resetParams()
{
	bFileOpened = false;
	isRunning = false;
	boolIsPaused = false;
	bLoop = false;
	bDone = false;
	currentDiskReadBuffer = 0;
	flags = 0;
}

void CAudio::initialiseAudio()
{
	HRESULT hr;
	if (FAILED(hr = XAudio2Create(&pXAudio2, flags)))
		throw std::runtime_error(std::format(lng::string(lng::LS_SOUND_XAUDIO2_CREATE_FAIL), hr));
	
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
		throw std::runtime_error(std::format(lng::string(lng::LS_SOUND_XAUDIO2_MASTERVOICE_CREATE_FAIL), hr));
}

int CAudio::readAndSubmintBuff()
{
	int pos = 0;
	int sec = 0;
	int ret = 1;

	bool eof = false;

	// Read in the bits
	while (ret && pos < STREAMING_BUFFER_SIZE)
	{
		ret = ov_read(&vf, buffers[currentDiskReadBuffer] + pos, STREAMING_BUFFER_SIZE - pos, 0, 2, 1, &sec);
		if (OV_EINVAL == ret)
		{
			// Закончился буфер.
			break;
		}
		if (ret < 0)
			return -1;

		pos += ret;
		// Если запись зациклена, то повторно заполняем буфер, пока он не закончится
		if (!ret && bLoop)
		{
			ov_pcm_seek(&vf, 0);
			ret = 1;
		}
	}
	if (!ret)
		eof = true;

	if (!pos)
		return -1;

	//Submit the wave sample data using an XAUDIO2_BUFFER structure
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.pAudioData = (BYTE*)&buffers[currentDiskReadBuffer];
	buffer.AudioBytes = pos;
	if (eof)
		buffer.Flags = XAUDIO2_END_OF_STREAM;	//Tell the source voice not to expect any data after this buffer

	currentDiskReadBuffer++;
	currentDiskReadBuffer %= MAX_BUFFER_COUNT;

	HRESULT hr = pSourceVoice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr))
		return -1;

	return (int)eof;
}

void CAudio::clearQueue()
{
	while (queue_.size())
	{
		char* value = queue_.front(); queue_.pop_front();
		free(value);
	}
}

void CAudio::addToQueue(LPCSTR fileName)
{
	if (!fileName)
		return;

	if (isPlaying())
	{
		queue_.push_back(createTrimString(fileName));
	}
	else
	{
		if (loadSound(fileName))
			play(false);
	}
}

bool CAudio::loadSound(const char* szSoundFileName)
{
	//If we already have a file open then kill the current voice setup
	if (bFileOpened)
	{
		pSourceVoice->Stop(0);
		pSourceVoice->DestroyVoice();
		pSourceVoice = NULL;

		ov_clear(&vf);

		resetParams();
	}

	TCHAR fileName[MAX_PATH];

	wsprintf(fileName, L"%s%S", path_, szSoundFileName);

	//Convert the path to unicode.
	//MultiByteToWideChar(CP_ACP, 0, strSoundPath, -1, wstrSoundPath, MAX_PATH);

	FILE *f;
	errno_t err;
	HRESULT hr;

	if ((err = _wfopen_s(&f, fileName, L"rb")) != 0)
	{
		// TODO: Выводить только 1 раз на файл.
		char szReason[MAX_PATH];
		_strerror_s(szReason, MAX_PATH, NULL);
		log_printidf(lng::LS_SOUND_FILE_OPEN_FAIL, szSoundFileName, szReason);

		return false;
	}

	//ov_open(f, &vf, NULL, 0);	//Windows does not like this function so we use ov_open_callbacks() instead
	if (FAILED(hr = ov_open_callbacks(f, &vf, NULL, 0, OV_CALLBACKS_DEFAULT)))
	{
		fclose(f);
		log_printidf(lng::LS_SOUND_SOURCE_OPEN_FAIL, szSoundFileName, hr);
		return false;
	}
	else
		bFileOpened = true;

	//The vorbis_info struct keeps the most of the interesting format info
	vorbis_info *vi = ov_info(&vf, -1);

	//Set the wave format
	WAVEFORMATEX wfm;
	memset(&wfm, 0, sizeof(wfm));

	wfm.cbSize = sizeof(wfm);
	wfm.nChannels = vi->channels;
	wfm.wBitsPerSample = 16;                    //Ogg vorbis is always 16 bit
	wfm.nSamplesPerSec = vi->rate;
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nChannels * 2;
	wfm.nBlockAlign = 2 * wfm.nChannels;
	wfm.wFormatTag = WAVE_FORMAT_PCM;

	memset(&buffers[currentDiskReadBuffer], 0, sizeof(buffers[currentDiskReadBuffer]));

	//Create the source voice
	if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &wfm)))
	{
		log_printidf(lng::LS_SOUND_SOURCE_CREATE_FAIL, hr);
		return false;
	}

	return true;
}

bool CAudio::play(bool loop)
{
	if (pSourceVoice == NULL)
	{
		return false;
	}

	HRESULT hr;

	bLoop = loop;

	// Подготовим звуковой буфер
	int res = 0;
	currentDiskReadBuffer = 0;
	for (int i = 0; i < MAX_BUFFER_COUNT && 1 != res; i++)
	{
		res = readAndSubmintBuff();
		if (-1 == res)
			return false;
	}

	if (FAILED(hr = pSourceVoice->Start(0)))
	{
		return false;
	}

	XAUDIO2_VOICE_STATE state;
	pSourceVoice->GetState(&state);
	isRunning = (state.BuffersQueued > 0) != 0;

	bDone = (1 == res);
	boolIsPaused = false;

	return isRunning;
}

void CAudio::stop()
{
	if (pSourceVoice == NULL)
		return;

	//XAUDIO2_FLUSH_BUFFERS according to MSDN is meant to flush the buffers after the voice is stopped
	//unfortunately the March 2008 release of the SDK does not include this parameter in the xaudio files
	//and I have been unable to ascertain what its value is
	//pSourceVoice->Stop(XAUDIO2_FLUSH_BUFFERS);
	pSourceVoice->Stop(0);

	boolIsPaused = false;
	isRunning = false;
}

bool CAudio::isPlaying()
{
	/*XAUDIO2_VOICE_STATE state;
	pSourceVoice->GetState(&state);
	return (state.BuffersQueued > 0) != 0;*/

	return isRunning;
}


//Alter the volume up and down
void CAudio::alterVolume(float fltVolume)
{
	if (pSourceVoice == NULL)
		return;

	pSourceVoice->SetVolume(fltVolume);			//Current voice volume
    //pMasteringVoice->SetVolume(fltVolume);	//Playback device volume
}

//Return the current volume
void CAudio::getVolume(float &fltVolume)
{
	if (pSourceVoice == NULL)
		return;

	pSourceVoice->GetVolume(&fltVolume);
	//pMasteringVoice->GetVolume(&fltVolume);
}

void CAudio::pause()
{
	if (pSourceVoice == NULL)
		return;

	if (boolIsPaused)
	{
		pSourceVoice->Start(0);	//Unless we tell it otherwise the voice resumes playback from its last position
		boolIsPaused = false;
	}
	else
	{
		pSourceVoice->Stop(0);
		boolIsPaused = true;
	}
}

bool CAudio::update()
{
	if (pSourceVoice == NULL)
		return false;

	if (!isRunning)
	{
		std::list<LPCSTR> test;

		bool ret = false;
		if (queue_.size())
		{
			char* fileName = queue_.front(); queue_.pop_front();
			ret = loadSound(fileName);
			free(fileName);
			if (ret)
				ret = play(false);
		}
		return ret;
	}

	XAUDIO2_VOICE_STATE state;
	pSourceVoice->GetState(&state);
	//log_dbg_printf("BuffersQueued: %d; SamplesPlayed: %d", state.BuffersQueued, state.SamplesPlayed);
	isRunning = (state.BuffersQueued > 0);
	// Если освободился буфер, то заполним его
	while (state.BuffersQueued < MAX_BUFFER_COUNT && !bDone)
	{
		int res = readAndSubmintBuff();
		bDone = (1 == res);
		pSourceVoice->GetState(&state);
		isRunning = (state.BuffersQueued > 0);
	}

	return isRunning;
}

void CAudio::playAndWait()
{
	play(false);
	waitPlaying();
}

void CAudio::waitPlaying()
{
	while (update())
		Sleep(100);
}