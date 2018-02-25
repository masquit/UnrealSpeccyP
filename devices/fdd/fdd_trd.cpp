/*
 Portable ZX-Spectrum emulator.
 Copyright (C) 2001-2018 SMT, Dexus, Alone Coder, deathsoft, djdron, scor
 
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

#include "../../std.h"

#include "fdd.h"

//=============================================================================
//	eFdd::CreateTrd
//-----------------------------------------------------------------------------
void eFdd::CreateTrd(int max_cyl)
{
	SAFE_DELETE(disk);
	disk = new eUdi(max_cyl, eUdi::MAX_SIDE);
	for(int i = 0; i < disk->Cyls(); ++i)
	{
		for(int j = 0; j < disk->Sides(); ++j)
		{
			Seek(i, j);
			
			int id_len = Track().data_len / 8 + ((Track().data_len & 7) ? 1 : 0);
			memset(Track().data, 0, Track().data_len + id_len);
			
			int pos = 0;
			WriteBlock(pos, 0x4e, 80);		//gap4a
			WriteBlock(pos, 0, 12);			//sync
			WriteBlock(pos, 0xc2, 3, true);	//iam
			Write(pos++, 0xfc);
			
			const int max_trd_sectors = 16;
			static const byte lv[3][max_trd_sectors] =
			{
				{ 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 },
				{ 1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16 },
				{ 1,12,7,2,13,8,3,14,9,4,15,10,5,16,11,6 }
			};
			Track().sectors_amount = max_trd_sectors;
			for(int i = 0; i < max_trd_sectors; ++i)
			{
				WriteBlock(pos, 0x4e, 40);		//gap1 50 fixme: recalculate gap1 only for non standard formats
				WriteBlock(pos, 0, 12);			//sync
				WriteBlock(pos, 0xa1, 3, true);	//id am
				Write(pos++, 0xfe);
				eUdi::eTrack::eSector& sec = Sector(i);
				sec.id = Track().data + pos;
				Write(pos++, cyl);
				Write(pos++, side);
				const int trdos_interleave = 1;
				Write(pos++, lv[trdos_interleave][i]);
				Write(pos++, 1); //256byte
				word crc = Crc(Track().data + pos - 5, 5);
				Write(pos++, crc >> 8);
				Write(pos++, (byte)crc);
				
				WriteBlock(pos, 0x4e, 22);		//gap2
				WriteBlock(pos, 0, 12);			//sync
				WriteBlock(pos, 0xa1, 3, true);	//data am
				Write(pos++, 0xfb);
				sec.data = Track().data + pos;
				int len = sec.Len();
				crc = Crc(Track().data + pos - 1, len + 1);
				pos += len;
				Write(pos++, crc >> 8);
				Write(pos++, (byte)crc);
			}
			assert(pos <= Track().data_len);
			WriteBlock(pos, 0x4e, Track().data_len - pos - 1); //gap3
		}
	}
	eUdi::eTrack::eSector* s = GetSector(0, 0, 9);
	if(!s)
		return;
	s->data[0xe2] = 1;					// first free track
	s->data[0xe3] = 0x16;				// 80T,DS
	s->DataW(0xe5, 2544);				// free sec
	s->data[0xe7] = 0x10;				// trdos flag
	UpdateCRC(s);
}
//=============================================================================
//	eFdd::AddFile
//-----------------------------------------------------------------------------
bool eFdd::AddFile(const byte* hdr, const byte* data)
{
	eUdi::eTrack::eSector* s = GetSector(0, 0, 9);
	if(!s)
		return false;
	int len = hdr[13];
	int pos = s->data[0xe4] * 0x10;
	eUdi::eTrack::eSector* dir = GetSector(0, 0, 1 + pos / 0x100);
	if(!dir)
		return false;
	if(s->DataW(0xe5) < len) //disk full
		return false;
	memcpy(dir->data + (pos & 0xff), hdr, 14);
	dir->DataW((pos & 0xff) + 14, s->DataW(0xe1));
	UpdateCRC(dir);
	
	pos = s->data[0xe1] + 16 * s->data[0xe2];
	s->data[0xe1] = (pos + len) & 0x0f, s->data[0xe2] = (pos + len) >> 4;
	s->data[0xe4]++;
	s->DataW(0xe5, s->DataW(0xe5) - len);
	UpdateCRC(s);
	
	// goto next track. s8 become invalid
	for(int i = 0; i < len; ++i, ++pos)
	{
		int cyl = pos / 32;
		int side = (pos / 16) & 1;
		if(!WriteSector(cyl, side, (pos & 0x0f) + 1, data + i * 0x100))
			return false;
	}
	return true;
}
//=============================================================================
//	eFdd::ReadScl
//-----------------------------------------------------------------------------
bool eFdd::ReadScl(const void* data, size_t data_size)
{
	if(data_size < 9)
		return false;
	const byte* buf = (const byte*)data;
	if(memcmp(data, "SINCLAIR", 8) || int(data_size) < 9 + (0x100 + 14)*buf[8])
		return false;
	
	CreateTrd(eUdi::MAX_CYL);
	int size = 0;
	for(int i = 0; i < buf[8]; ++i)
	{
		size += buf[9 + 14 * i + 13];
	}
	if(size > 2544)
	{
		eUdi::eTrack::eSector* s = GetSector(0, 0, 9);
		s->DataW(0xe5, size);			// free sec
		UpdateCRC(s);
	}
	const byte* d = buf + 9 + 14 * buf[8];
	for(int i = 0; i < buf[8]; ++i)
	{
		if(!AddFile(buf + 9 + 14*i, d))
			return false;
		d += buf[9 + 14*i + 13]*0x100;
	}
	return true;
}
//=============================================================================
//	eFdd::ReadTrd
//-----------------------------------------------------------------------------
bool eFdd::ReadTrd(const void* data, size_t data_size)
{
	int max_cyl = data_size / (256 * 16 * 2);
	if(max_cyl > eUdi::MAX_CYL)
		max_cyl = eUdi::MAX_CYL;
	CreateTrd(max_cyl);
	for(size_t i = 0; i < data_size; i += 0x100)
	{
		WriteSector(i >> 13, (i >> 12) & 1, ((i >> 8) & 0x0f) + 1, (const byte*)data + i);
	}
	return true;
}
//=============================================================================
//	eFdd::WriteTrd
//-----------------------------------------------------------------------------
bool eFdd::WriteTrd(FILE* file) const
{
	byte zerosec[256];
	memset(zerosec, 0, 256);
	for(int i = 0; i < disk->Cyls(); ++i)
	{
		for(int j = 0; j < disk->Sides(); ++j)
		{
			const eUdi::eTrack& t = disk->Track(i, j);
			for(int se = 0; se < 16; ++se)
			{
				const byte* data = zerosec;
				for(int k = 0; k < 16; ++k)
				{
					const eUdi::eTrack::eSector& s = t.sectors[k];
					if(s.id && (s.Sec() == se + 1) && s.Len() == 256)
					{
						data = s.data;
						break;
					}
				}
				if(fwrite(data, 1, 256, file) != 256)
					return false;
			}
		}
	}
	return true;
}
