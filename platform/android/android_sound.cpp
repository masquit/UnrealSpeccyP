/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2015 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../platform.h"

#ifdef _ANDROID

#include "../../tools/sound_mixer.h"

namespace xPlatform
{

void InitSound()
{
}
void DoneSound()
{
}
static eSoundMixer sound_mixer;
int UpdateSound(byte* buf, bool skip_data)
{
	if(skip_data)
	{
		for(int s = Handler()->AudioSources(); --s >= 0;)
		{
			Handler()->AudioDataUse(s, Handler()->AudioDataReady(s));
		}
		return 0;
	}
	sound_mixer.Update(buf);
	dword size = sound_mixer.Ready();
	sound_mixer.Use(size, buf);
	return size;
}

}
//namespace xPlatform

#endif//_ANDROID
