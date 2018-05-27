//
//  NSSound.h
//  NeilOS
//
//  Created by Neil Singh on 1/10/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSSOUND_H
#define NSSOUND_H

#include "NSTypes.h"

#include <string>

struct pthread;

class NSSound {
public:
	static NSSound* Create(const std::string& filename, bool dealloc=true);
	~NSSound();
	
	// Is valid sound
	bool IsValid();
	
	// Play (synchronous by default)
	void Play();
	void Play(bool sync);
	void Pause();
	void Stop();
	bool IsPlaying();
	
	// Volume (from 0.0 to 1.0)
	void SetVolume(double vol);
	double GetVolume();
	
	// Seeking (in seconds)
	void SetPosition(NSTimeInterval sec);
	NSTimeInterval GetPosition();
	NSTimeInterval GetLength();
	
private:
	NSSound(const std::string& filename, bool dealloc);

	std::string filename;
	volatile pthread* sound_thread = NULL;
	void* data = NULL;
	bool dealloc = false;
};

#endif /* NSSOUND_H */
