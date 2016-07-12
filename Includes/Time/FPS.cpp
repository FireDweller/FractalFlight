#include "FPS.h"

	void Fps::update_fps()
	{
		// increase the counter by one
		m_fpscount++;

		// one second elapsed? (= 1000 milliseconds)
		if (m_fpsinterval.value() > 1000)
		{
			// save the current counter value to m_fps
			m_fps = m_fpscount;

			// reset the counter and the interval
			m_fpscount = 0;
			m_fpsinterval = Interval();
		}

	}

	void Fps::update_frames()
	{
		// increase the counter by one
		m_framescount++;

		// one second elapsed? (= 1000 milliseconds)
		if (m_framesinterval.value() > 10)
		{
			// save the current counter value to m_fps
			m_frames = m_framescount;

			// reset the counter and the interval
			m_framescount = 0;
			m_framesinterval = Interval();
		}

	}

	void Fps::update()
	{
		update_frames();
		update_fps();
	}

	unsigned int Fps::get_fps() const
	{
		return m_fps;
	}

	unsigned int Fps::get_frames() const
	{
		return m_frames;
	}
