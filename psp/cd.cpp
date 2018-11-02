/*
Copyright (C) 2007 Peter Mackay and Chris Swindle.
Copyright (C) 2009 Crow_bar.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <cstddef>
#include <stdio.h>

#include <pspaudiolib.h>
#include <pspiofilemgr.h>

extern "C"
{
#include "../quakedef.h"
}

#include "mp3.h"

extern 	cvar_t bgmtype;
extern	cvar_t bgmvolume;

#define MAX_TRACKS 128

namespace quake
{
	namespace cd
	{
		struct Sample
		{
			short left;
			short right;
		};

		static int	 last_track = 4;
		static bool	 playing  = false;
		static bool	 paused   = false;
		static bool	 enabled  = false;
		static float cdvolume = 0;
	    static int   num_tracks = 0;
	    static char  tracks[MAX_TRACKS][MAX_QPATH];
	    static int   cd_loop = 0;
	}
}

using namespace quake;
using namespace quake::cd;

static void CD_f (void)
{
	char	*command;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("commands:");
		Con_Printf("on, off, reset, remap, \n");
		Con_Printf("play, next, prev, stop, loop, pause, resume\n");
		Con_Printf("eject, close, info, mlist\n");
		return;
	}

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "remap") == 0)
	{
        Con_Printf("not used\n");
		return;
	}

	if (Q_strcasecmp(command, "close") == 0)
	{
        Con_Printf("not used\n");
		return;
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), (qboolean) false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), (qboolean) true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "next") == 0)
	{
		CDAudio_Next();
		return;
	}

	if (Q_strcasecmp(command, "prev") == 0)
	{
		CDAudio_Prev();
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		if (playing)
			CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("MP3 Player By Crow_bar\n");
		Con_Printf("Based On sceMp3 Lib\n");
		Con_Printf("- 2009 -\n");
		Con_Printf("\n");
		return;
	}

	if (Q_strcasecmp(command, "mlist") == 0)
	{
        CDAudio_PrintMusicList();
		return;
	}
}

void CDAudio_VolumeChange(float bgmvolume)
{
    if(!enabled)
       return;

	int volume = (int) (bgmvolume * (float) PSP_VOLUME_MAX);
	mp3_volume = volume;
	changeMp3Volume=0;
	cdvolume = bgmvolume;
}

extern "C" int sceKernelDelayThread(int delay); 

void CDAudio_Play(byte track, qboolean looping)
{
    if(!enabled)
       return;

	CDAudio_Stop();

	if(track >= num_tracks || track <= 0)
	   track = 1;

	last_track = track;

    mp3_pause = 0;

	int ret;
	ret = mp3_start_play(tracks[track], 0);

	cd_loop = looping;
	if(ret != 2)
	{
		Con_Printf("Playing %s\n", tracks[track]);
		playing = true;
	}
	else
	{
		Con_Printf("Couldn't find %s\n", tracks[track]);
		playing = false;
		CDAudio_VolumeChange(0);
	}
	
	CDAudio_VolumeChange(bgmvolume.value);
}

void CDAudio_Stop(void)
{
	if(!enabled)
       return;

	mp3_job_started = 0;
	playing = false;
}

void CDAudio_Pause(void)
{
    if(!enabled)
       return;

	mp3_pause = 1;

	paused = true;
}

void CDAudio_Resume(void)
{
	if(!enabled)
       return;

    mp3_pause = 0;

	paused = false;
}

void CDAudio_Update(void)
{
    if(enabled == true)
    {
        if(strcmpi(bgmtype.string,"cd") == 0)
        {
			if(mp3_status == MP3_END)
		    {
			     if(cd_loop == 0)
					   last_track = last_track + 1;

				 CDAudio_Play(last_track, (qboolean)cd_loop);
		    }
		    else if(mp3_status == MP3_NEXT)
		    {
			    last_track = last_track + 1;
			    CDAudio_Play(last_track, (qboolean)cd_loop);
		    }

            if(changeMp3Volume)
               CDAudio_VolumeChange(bgmvolume.value);
		}
		else
		{
            if (playing == true)
			{
                if(paused)
	               CDAudio_Resume();

				CDAudio_Stop();
			}
		}
    }
}

void CDAudio_Next(void)
{
     if(!enabled)
        return;

	 if(paused)
	    CDAudio_Resume();

	 last_track = last_track + 1;
	 CDAudio_Play(last_track, (qboolean) false);
}

void CDAudio_Prev(void)
{
     if(!enabled)
        return;

	 if(paused)
	    CDAudio_Resume();

	 last_track = last_track - 1;
	 CDAudio_Play(last_track, (qboolean) false);
}

int CDAudio_Init(void)
{
	if(COM_CheckParm("-nomp3"))
	   return 0;

	mp3_init();

	sceKernelDelayThread(5*10000);

	num_tracks = 1;

	char path[256];

	sprintf(path, "%s/MP3/", host_parms.basedir);

	SceUID dir = sceIoDopen(path);
	if(dir < 0)
	{
		Con_Printf("/MP3/ Directory not found\n");
		return 0;
	}

	SceIoDirent dirent;

    memset(&dirent, 0, sizeof(SceIoDirent));

	while(sceIoDread(dir, &dirent) > 0)
	{
		if(dirent.d_name[0] == '.')
		{
			continue;
		}

		if(!strcasecmp(COM_FileExtension (dirent.d_name),"mp3"))
	    {
		  Con_Printf("music track: %s\n", dirent.d_name);
          sprintf(tracks[num_tracks],"%s%s",path, dirent.d_name);
          num_tracks++;
        }

        memset(&dirent, 0, sizeof(SceIoDirent));
    }
	sceIoDclose(dir);

	enabled = true;

	Cmd_AddCommand ("cd", CD_f);

	Con_Printf("CD Audio Initialized\n");

	return 0;
}

void CDAudio_PrintMusicList(void)
{
    Con_Printf("\n");
    Con_Printf("================== Music List ===================\n");
	for(int i = 1; i < num_tracks ; i++)
	{
	  Con_Printf("%i: %s\n",i, tracks[i]);
	}
    Con_Printf("=================================================\n");
    Con_Printf("\n");
}

void CDAudio_Shutdown(void)
{
	if(COM_CheckParm("-nomp3"))
	   return;

	Con_Printf("CDAudio_Shutdown\n");

	CDAudio_Stop();

	sceKernelDelayThread(5*10000);
	mp3_deinit();
}

