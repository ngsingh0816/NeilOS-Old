//
//  NSSound.cpp
//  NeilOS
//
//  Created by Neil Singh on 1/10/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSSound.h"
#include "NSLog.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sched.h>
#include <mpg123/mpg123.h>
#include <sys/ioctl.h>
#include <string.h>

#define SAMPLE_RATE			44100
#define BYTES_PER_SAMPLE	2

#define MP3_TYPE 	1
#define WAV_TYPE	2

#define STATE_CANCELED	0
#define STATE_PLAYING	1
#define STATE_PAUSED	2

using std::string;

namespace {
	typedef struct {
		uint8_t* buffer;
		unsigned int size;
		unsigned int offset;
		pthread_mutex_t lock;
		NSSound* sound;
		volatile int state;
		int type;
		int fd;
		double volume;
		
		// MP3 things
		mpg123_handle* handle;
	} SoundData;

	// Whether we have inited the mp3 decoder
	bool nssound_setup = false;
};

void* soundFunc(void* sd) {
	SoundData* sound_data = reinterpret_cast<SoundData*>(sd);
	NSSound* sound = sound_data->sound;
	if (sound_data->type == WAV_TYPE) {
		int size = 0;
		
		for (;;) {
			size = read(sound_data->fd, &size, 0);
			pthread_mutex_lock(&sound_data->lock);
			if (sound_data->offset >= sound_data->size) {
				pthread_mutex_unlock(&sound_data->lock);
				break;
			}
			int min = ((sound_data->offset + size) >= sound_data->size) ? (sound_data->size - sound_data->offset) : size;
			write(sound_data->fd, &sound_data->buffer[sound_data->offset], min);
			sound_data->offset += size;
			pthread_mutex_unlock(&sound_data->lock);
			
			if (sound_data->state == STATE_CANCELED)
				break;
			while (sound_data->state == STATE_PAUSED)
				sched_yield();
		}
	} else {
		size_t tmp = 0;
		int size = read(sound_data->fd, &size, 0);
		uint8_t* out = new uint8_t[size];
		
		int ret = 0;
		while (ret != MPG123_ERR && ret != MPG123_NEED_MORE) {
			pthread_mutex_lock(&sound_data->lock);
			ret = mpg123_read(sound_data->handle, out, size, &tmp);
			pthread_mutex_unlock(&sound_data->lock);
			
			read(sound_data->fd, &size, 0);
			write(sound_data->fd, out, tmp);
			
			if (sound_data->state == STATE_CANCELED)
				break;
			while (sound_data->state == STATE_PAUSED)
				sched_yield();
		}
	
		delete[] out;
	}
	sound->Stop();
	
	return NULL;
}

NSSound::~NSSound() {
	SoundData* d = reinterpret_cast<SoundData*>(data);
	if (sound_thread) {
		d->state = STATE_CANCELED;
		while (sound_thread)
			sched_yield();
	}
	if (data) {
		if (d->fd != -1)
			close(d->fd);
		if (d->type == WAV_TYPE) {
			if (d->buffer)
				munmap(d->buffer, d->size);
		} else {
			if (d->handle) {
				mpg123_close(d->handle);
				mpg123_delete(d->handle);
			}
		}
		delete d;
	}
}

NSSound* NSSound::Create(const std::string& fn, bool d) {
	return new NSSound(fn, d);
}

NSSound::NSSound(const string& fn, bool de) {
	if (!nssound_setup) {
		nssound_setup = true;
		mpg123_init();
	}
	
	sound_thread = NULL;
	filename = fn;
	dealloc = de;
	
	data = new SoundData;
	SoundData* d = reinterpret_cast<SoundData*>(data);
	memset(d, 0, sizeof(SoundData));
	d->volume = 1.0;
	d->sound = this;
	d->lock = PTHREAD_MUTEX_INITIALIZER;
	
	const string mp3String = ".mp3";
	const string wavString = ".wav";
	if (fn.length() >= mp3String.length() &&
		fn.compare(fn.length() - mp3String.length(), mp3String.length(), mp3String) == 0) {
		d->type = MP3_TYPE;
		
		int ret = 0;
		d->handle = mpg123_new(NULL, &ret);
		mpg123_param(d->handle, static_cast<enum mpg123_parms>(MPG123_QUIET | MPG123_FORCE_STEREO), 2, 0);
		mpg123_open_feed(d->handle);
		
		FILE* file = fopen(filename.c_str(), "r");
		fseek(file, 0, SEEK_END);
		uint32_t end = ftell(file);
		rewind(file);
		
		uint8_t* buf = new uint8_t[end];
		fread(buf, 1, end, file);
		fclose(file);
		
		mpg123_format_none(d->handle);
		mpg123_format(d->handle, SAMPLE_RATE, MPG123_STEREO, MPG123_ENC_SIGNED_16);
		mpg123_feed(d->handle, buf, end);
		delete[] buf;
		
		d->buffer = NULL;
		d->size = end;
		
		if (ret == MPG123_ERR) {
			delete d;
			data = NULL;
			return;
		}
	} if (fn.length() >= wavString.length() &&
		  fn.compare(fn.length() - wavString.length(), wavString.length(), wavString) == 0) {
		d->type = WAV_TYPE;
		FILE* file = fopen(filename.c_str(), "r");
		if (!file) {
			delete d;
			data = NULL;
			return;
		}
		
		fseek(file, 0, SEEK_END);
		int s = ftell(file);
		fseek(file, 0xC, SEEK_SET);
		for (;;) {
			uint32_t desc = 0;
			fread(&desc, 1, sizeof(uint32_t), file);
			if (desc == 0x61746164) {
				fseek(file, 4, SEEK_CUR);
				s -= ftell(file);
				break;
			} else {
				uint32_t size = 0;
				fread(&size, 1, sizeof(uint32_t), file);
				fseek(file, size, SEEK_CUR);
			}
			
			if (ftell(file) >= s) {
				delete d;
				data = NULL;
				return;
			}
		}
		
		d->size = s;
		
		if (d->size == 0) {
			NSLog("NSSound - Invalid file.\n");
			delete d;
			data = NULL;
			return;
		}
		d->buffer = reinterpret_cast<uint8_t*>(mmap(NULL, d->size, PROT_READ, MAP_PRIVATE, fileno(file), 0));
		
		fclose(file);
	}
	
	d->fd = open("/dev/audio", O_RDWR);
}

// Is valid sound
bool NSSound::IsValid() {
	return (data != NULL);
}

void NSSound::Play() {
	Play(false);
}

void NSSound::Play(bool sync) {
	if (!data)
		return;
	
	SoundData* d = reinterpret_cast<SoundData*>(data);
	d->state = STATE_PLAYING;
	
	if (!sound_thread)
		pthread_create((pthread_t*)&sound_thread, NULL, soundFunc, data);
	
	if (sync) {
		while (sound_thread) {
			sched_yield();
		}
	}
}

void NSSound::Pause() {
	if (!data || !sound_thread)
		return;
	
	SoundData* d = reinterpret_cast<SoundData*>(data);
	if (d->state == STATE_PLAYING)
		d->state = STATE_PAUSED;
}

void NSSound::Stop() {
	if (!data || !sound_thread)
		return;
	
	SoundData* d = reinterpret_cast<SoundData*>(data);
	d->state = STATE_CANCELED;
	sound_thread = NULL;
	
	if (dealloc)
		delete this;
}

bool NSSound::IsPlaying() {
	if (!data)
		return false;
	
	SoundData* sd = reinterpret_cast<SoundData*>(data);
	return (sd->state == STATE_PLAYING);
}

// Volume (from 0.0 to 1.0)
void NSSound::SetVolume(double vol) {
	if (!data)
		return;
	
	SoundData* sd = reinterpret_cast<SoundData*>(data);
	sd->volume = vol;
	ioctl(sd->fd, 0, vol);
	pthread_mutex_unlock(&sd->lock);
}

double NSSound::GetVolume() {
	if (!data)
		return 0.0;
	
	SoundData* sd = reinterpret_cast<SoundData*>(data);
	return sd->volume;
}

// Seeking (in seconds)
void NSSound::SetPosition(NSTimeInterval sec) {
	if (!data)
		return;
	
	SoundData* sd = reinterpret_cast<SoundData*>(data);
	if (sd->type == WAV_TYPE) {
		uint32_t off = sec * SAMPLE_RATE * BYTES_PER_SAMPLE;
		pthread_mutex_lock(&sd->lock);
		sd->offset = off;
		pthread_mutex_unlock(&sd->lock);
	} else {
		pthread_mutex_lock(&sd->lock);
		mpg123_seek_frame(sd->handle, mpg123_timeframe(sd->handle, sec), SEEK_SET);
		pthread_mutex_unlock(&sd->lock);
	}
}

NSTimeInterval NSSound::GetPosition() {
	if (!data)
		return 0.0;
	
	SoundData* sd = reinterpret_cast<SoundData*>(data);
	if (sd->type == WAV_TYPE) {
		return (double)sd->offset / SAMPLE_RATE / BYTES_PER_SAMPLE;
	} else {
		pthread_mutex_lock(&sd->lock);
		NSTimeInterval frame_length = mpg123_tpf(sd->handle);
		uint32_t frame = mpg123_tellframe(sd->handle);
		pthread_mutex_unlock(&sd->lock);
		return frame_length * frame;
	}
}

NSTimeInterval NSSound::GetLength() {
	if (!data)
		return 0.0;
	
	SoundData* sd = reinterpret_cast<SoundData*>(data);
	if (sd->type == WAV_TYPE) {
		return (double)sd->size / SAMPLE_RATE / BYTES_PER_SAMPLE;
	} else {
		pthread_mutex_lock(&sd->lock);
		NSTimeInterval frame_length = mpg123_tpf(sd->handle);
		uint32_t num_frames = mpg123_framelength(sd->handle);
		pthread_mutex_unlock(&sd->lock);
		
		return num_frames * frame_length;
	}
}
