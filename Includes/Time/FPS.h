#include "Interval.h"

class Fps
{
protected:

	unsigned int m_frames;
	unsigned int m_framescount;
	unsigned int m_fps;
	unsigned int m_fpscount;
	bool start = false;
	Interval m_fpsinterval, m_framesinterval;

public:
	
	Fps() : m_fps(0), m_fpscount(0), m_framescount(0), m_frames(0)
	{
	}

	void update();
	void update_fps();
	void update_frames();

	unsigned int get_fps() const;
	unsigned int get_frames() const;
};
#pragma once
