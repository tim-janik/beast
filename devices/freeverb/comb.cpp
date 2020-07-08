// Comb filter implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#include "comb.hpp"

comb::comb()
{
	filterstore = 0;
	bufidx = 0;
}

void comb::setbuffer(float *buf, int size)
{
	buffer = buf;
	bufsize = size;
}

void comb::mute()
{
	for (int i=0; i<bufsize; i++)
		buffer[i]=0;
}

void comb::setdamp(float val, int mode)
{
        damp1 = val;            // original May 2000 damping
        damp2 = 1-val;
	if (mode == -1)         // STK: https://github.com/thestk/stk/blob/master/src/FreeVerb.cpp
          damp1 = -damp1;
	else if (mode == 0)     // VLC: https://github.com/videolan/vlc/blob/master/modules/audio_filter/spatializer/comb.hpp#L49
          damp1 = 0;
}

float comb::getdamp()
{
	return damp1;
}

void comb::setfeedback(float val)
{
	feedback = val;
}

float comb::getfeedback()
{
	return feedback;
}

// ends
