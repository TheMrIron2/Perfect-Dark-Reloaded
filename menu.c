/*
Copyright (C) 1996-1997 Id Software, Inc.

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
#include "quakedef.h"

#ifdef WIN32
#include "winquake.h"
#endif

#ifdef PSP
#include <pspkernel.h>
#include <psputility.h>
//#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <ctype.h>
#include "net_dgrm.h"
#include "cl_slist.h"
#include "crypter.h"


extern cvar_t	accesspoint;
extern cvar_t	r_wateralpha;
extern cvar_t	r_vsync;
extern cvar_t	in_disable_analog;
extern cvar_t	in_analog_strafe;
extern cvar_t	in_x_axis_adjust;
extern cvar_t	in_y_axis_adjust;
extern cvar_t	crosshair;
extern cvar_t	r_dithering;
#endif

#ifdef PSP_MP3_HWDECODE
extern int changeMp3Volume;
#endif

extern qboolean bmg_type_changed;
void CL_RemoveGIPFiles (char *path);
void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

enum
{
m_none,
m_main,
m_singleplayer,
m_load,
m_save,
m_maps, //Crow_bar. maplist
m_multiplayer,
m_setup,
m_net,
m_options,
m_video,
m_keys,
m_help,
m_quit,
m_serialconfig,
m_modemconfig,
m_lanconfig,
m_gameoptions,
m_search,
m_slist,
m_sedit,
m_osk
} m_state;

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
		void M_Menu_Maps_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_Net_f (void);
        void M_Menu_ServerList_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Video_f (void);
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);
void M_Menu_SerialConfig_f (void);
	void M_Menu_ModemConfig_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
		void M_Maps_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
		void M_Net_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		void M_Video_Draw (void);
	void M_Help_Draw (void);
	void M_Quit_Draw (void);
void M_SerialConfig_Draw (void);
	void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
void M_SinglePlayer_Key (int key);
	void M_Load_Key (int key);
	void M_Save_Key (int key);
	void M_Maps_Key (int key);
void M_MultiPlayer_Key (int key);
	void M_Setup_Key (int key);
	void M_Net_Key (int key);
void M_Options_Key (int key);
	void M_Keys_Key (int key);
	void M_Video_Key (int key);
void M_Help_Key (int key);
void M_Quit_Key (int key);
void M_SerialConfig_Key (int key);
void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);

void Con_SetOSKActive(qboolean active);
void M_Menu_OSK_f (char *input, char *output, int outlen);


qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define	IPXConfig		(m_net_cursor == 2)
#define	TCPIPConfig		(m_net_cursor == 3)

void M_ConfigureNetSubsystem(void);

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line, num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

int	M_ColorForMap (int m)
{
	return m < 128 ? m + 8 : m + 8;
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320)>>1), y, pic);
}

byte identityTable[256];
byte translationTable[256];

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	memcpy (dest, source, 256);

	if (top < 128)	// the artists made some backwards ranges.  sigh.
		memcpy (dest + TOP_RANGE, source + top, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[TOP_RANGE+j] = source[top+15-j];

	if (bottom < 128)
		memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[BOTTOM_RANGE+j] = source[bottom+15-j];
}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y, pic, translationTable);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

//=============================================================================

int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f ();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		M_Menu_Main_f ();
	}
}


//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
#define	MAIN_ITEMS	5


void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
}


void M_Main_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_main.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mainmenu.lmp") );

	f = (int)(host_time * 10)%6;

	M_DrawTransPic (54, 32 + m_main_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f ();
			break;

		case 1:
			M_Menu_MultiPlayer_f ();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Help_f ();
			break;

		case 4:
#if 0
		    M_Exit_f();
#endif
			M_Menu_Quit_f ();
			break;
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS  4


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_sgl.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/sp_menu.lmp") );

	f = (int)(host_time * 10)%6;
	M_Print (74, 100, "Maps");
	M_DrawTransPic (54, 32 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			/*
			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
					break;
			*/
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			CL_RemoveGIPFiles(NULL);
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("map start\n");
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;	
		case 3:
			M_Menu_Maps_f ();
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/s%i.sav", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose (f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_load.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_save.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}
//=============================================================================
/* MAPS MENU by Crow_bar */
#include <pspiofilemgr.h>

#define MAPS_INPAGE  25
#define MAPS_MAX     256
#define MAPS_MAXNAME 32

int		maps_cursor;
int     maps_stage;
int     maps_num;
int     maps_maxlen;
char    maps_list[MAPS_MAX][MAPS_MAXNAME];


void M_ScanMaps ()
{
    maps_num = 0;
	SceUID dir = sceIoDopen(va("%s/maps/usermaps", com_gamedir));
	if(dir < 0)
	{
		return;
	}

	SceIoDirent dirent;
    memset(&dirent, 0, sizeof(SceIoDirent));
	while(sceIoDread(dir, &dirent) > 0)
	{
		if(dirent.d_name[0] == '.')
		{
			continue;
		}
		if(!strcasecmp(COM_FileExtension (dirent.d_name),"bsp"))
	    {
		  COM_StripExtension (dirent.d_name, maps_list[maps_num]);

		  int cur_len = strlen(maps_list[maps_num]);
		  if(maps_maxlen < cur_len)
			 maps_maxlen = cur_len;

          maps_num++;
        }
        memset(&dirent, 0, sizeof(SceIoDirent));
    }
	sceIoDclose(dir);

}

void M_Menu_Maps_f (void)
{
	m_entersound = true;
	m_state = m_maps;
	key_dest = key_menu;
	M_ScanMaps ();
}

void M_Maps_Draw (void)
{
	int		i;

    M_DrawTextBox (150, 8, 4, 1);
    M_Print(158, 16, "Maps");

    if((!maps_list[0]) && (maps_num == 0))
    {
       M_Print (32, 32 + 8,"Maps not found");
       return;
	}

    M_DrawTextBox (24, 24, maps_maxlen, MAPS_INPAGE);

	for (i = 0 ; i < MAPS_INPAGE; i++)
	{
		M_Print (32, 32 + 8*(i), maps_list[i+maps_stage]);
    }

// line cursor
	M_DrawCharacter (24, 32 + maps_cursor * 8, 12+((int)(realtime*4)&1));
}

void M_Maps_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("map usermaps/%s\n", maps_list[maps_cursor+maps_stage]));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");

		maps_cursor--;
	    if(maps_cursor < 0)//hh
	    {
			maps_cursor = 0;
			if(maps_stage > 0)
		       maps_stage--;
        }

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");

	    if(maps_cursor >= maps_num-1)
		   break;

	    maps_cursor++;
	    if(maps_cursor > MAPS_INPAGE-1)
	    {
		   maps_cursor = MAPS_INPAGE-1;
		   if(maps_stage + maps_cursor < maps_num-1)
		      maps_stage++;
        }
		break;
	}
}
//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;

#ifdef PSP
#define	MULTIPLAYER_ITEMS   6
#else
#define	MULTIPLAYER_ITEMS	3
#endif

void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
}

#include <pspwlan.h>
void M_MultiPlayer_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	f = (int)(host_time * 10)%6;

	M_DrawTransPic (54, 32 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );

    M_Print (72, 97,  "Slist          ");
#ifdef PSP
    M_Print (72, 117, "Infrastructure ");
	M_DrawCheckbox (220, 117, tcpipAvailable && !tcpipAdhoc);

	M_Print (72, 137, "Adhoc          ");
	M_DrawCheckbox (220, 137, tcpipAvailable && tcpipAdhoc);
#endif

    if (serialAvailable || ipxAvailable || tcpipAvailable)
	    	return;

	M_PrintWhite ((320/2) - ((27*8)/2), 207, "No Communications Available");

}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;
	    
	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if ((serialAvailable || ipxAvailable || tcpipAvailable) && sceWlanDevIsPowerOn())
				M_Menu_Net_f ();
			break;

		case 1:
			if ((serialAvailable || ipxAvailable || tcpipAvailable) && sceWlanDevIsPowerOn())
				M_Menu_Net_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;

	    case 3:
			M_Menu_ServerList_f();
			break;
#ifdef PSP
		case 4:
            Datagram_Shutdown();

			tcpipAvailable = !tcpipAvailable;

			if(tcpipAvailable && sceWlanDevIsPowerOn())
			{
				tcpipAdhoc = false;
				net_driver_to_use = 0;
				Datagram_Init();
			}
			break;
		case 5:
			Datagram_Shutdown();

			tcpipAvailable = !tcpipAvailable;

			if(tcpipAvailable && sceWlanDevIsPowerOn())
			{
				tcpipAdhoc = true;
				net_driver_to_use = 1;
				Datagram_Init();
			}
			break;

#endif
		}
	}
}

//=============================================================================
/* SETUP MENU */

#ifndef PSP
int		setup_cursor = 4;
int		setup_cursor_table[] = {40, 56, 80, 104, 140};
#else
int		setup_cursor = 5;
int		setup_cursor_table[] = {40, 56, 72, 96, 120, 156};
#endif

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#ifndef PSP
#define	NUM_SETUP_CMDS	5
#else
// Define PSP specific variables
#define	NUM_SETUP_CMDS	6
extern int totalAccessPoints;
extern int accessPointNumber[100];
char    setup_accesspoint[64];
#endif

void M_Menu_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	Q_strcpy(setup_myname, cl_name.string);
	Q_strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
	
#ifdef PSP
	if(totalAccessPoints)
	{
	    sceUtilityGetNetParam(accessPointNumber[(int)accesspoint.value], 0, (netData*)setup_accesspoint);
	}
#endif
}

void M_Setup_Draw (void)
{
	qpic_t	*p;
	int offset = 0;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

#ifdef PSP
	offset = 16;

	M_Print (64, 40, "Access Point");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_accesspoint);
#endif

	M_Print (64, 40+offset, "Hostname");
	M_DrawTextBox (160, 32+offset, 16, 1);
	M_Print (168, 56, setup_hostname);

	M_Print (64, 56+offset, "Your name");
	M_DrawTextBox (160, 48+offset, 16, 1);
	M_Print (168, 56+offset, setup_myname);

	M_Print (64, 80+offset, "Shirt color");
	M_Print (64, 104+offset, "Pants color");

	M_DrawTextBox (64, 140+offset-8, 14, 1);
	M_Print (72, 140+offset, "Accept Changes");

	p = Draw_CachePic ("gfx/bigbox.lmp");
	M_DrawTransPic (160, 64+offset, p);
	p = Draw_CachePic ("gfx/menuplyr.lmp");
	M_BuildTranslationTable(setup_top*16, setup_bottom*16);
	M_DrawTransPicTranslate (172, 72+offset, p);

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

#ifndef PSP
	offset = 0;
#else
	offset = 1;
#endif

	if (setup_cursor == 0+offset)
		M_DrawCharacter (168 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1+offset)
		M_DrawCharacter (168 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}


void M_Setup_Key (int k)
{
	int	l;
	int	offset = 0;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
#ifdef PSP
		if (setup_cursor == 0)
		{
			S_LocalSound ("misc/menu3.wav");
			if(accesspoint.value > 1)
			{
				Cvar_SetValue("accesspoint", accesspoint.value-1);
				sceUtilityGetNetParam(accessPointNumber[(int)accesspoint.value], 0, (netData*)setup_accesspoint);			
			}
		}
		offset = 1;
#endif
		if (setup_cursor < 2+offset)
			return;
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2+offset)
			setup_top = setup_top - 1;
		if (setup_cursor == 3+offset)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
#ifdef PSP
		if (setup_cursor == 0)
		{
			S_LocalSound ("misc/menu3.wav");
			if(accesspoint.value < totalAccessPoints)
			{
				Cvar_SetValue("accesspoint", accesspoint.value+1);
				sceUtilityGetNetParam(accessPointNumber[(int)accesspoint.value], 0, (netData*)setup_accesspoint);
			}
		}

		offset = 1;
#endif

		if (setup_cursor < 2+offset)
			return;
forward:
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2+offset)
			setup_top = setup_top + 1;
		if (setup_cursor == 3+offset)
			setup_bottom = setup_bottom + 1;
		break;

	case K_INS:
#ifdef PSP
		offset = 1;
#endif
		if (setup_cursor == 0+offset)
		{
			M_Menu_OSK_f(setup_hostname, setup_hostname, 16);
			break;
		}

		if (setup_cursor == 1+offset)
		{
			M_Menu_OSK_f(setup_myname, setup_myname,16);
			break;
		}
		break;

	case K_ENTER:
#ifdef PSP
		offset = 1;
#endif
		if (setup_cursor == 0+offset || setup_cursor == 1+offset)
			return;

		if (setup_cursor == 2+offset || setup_cursor == 3+offset)
			goto forward;

		if (setup_cursor < 4+offset)
			break;

		// setup_cursor == 4 (OK)
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (Q_strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
#ifdef PSP
		offset = 1;
#endif
		if (setup_cursor == 0+offset)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1+offset)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;

	default:
		if (k < 32 || k > 127)
			break;

#ifdef PSP
		offset = 1;
#endif

		if (setup_cursor == 0+offset)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 1+offset)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	if (setup_top > 13)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 13;
	if (setup_bottom > 13)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 13;
}

//=============================================================================
/* SERVER LIST MENU */
void M_Menu_SEdit_f (void);

#define	MENU_X	50
#define	MENU_Y	21
#define TITLE_Y 4
#define	STAT_Y	166

int	slist_cursor = 0, slist_mins = 0, slist_maxs = 15, slist_state;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;

	slist_state = 0;
}

void M_ServerList_Draw (void)
{
	int	serv, line;

	M_DrawTransPic (16, 4, Draw_CachePic("gfx/qplaque.lmp"));
	M_DrawTextBox (MENU_X, TITLE_Y, 23, 1);
	M_PrintWhite (MENU_X + 60, TITLE_Y + 8, "Server List");

	if (!slist[0].server)
	{
		M_PrintWhite (84, MENU_Y + 16 + 16, "Empty server list");
		M_Print (60, MENU_Y + 16 + 32, "Press TRIANGLE to add a server");
		M_Print (60, MENU_Y + 16 + 40, "Press SQUARE to edit a server");
        M_Print (60, MENU_Y + 16 + 48, "Press CIRCLE to exit");
		return;
	}

	M_DrawTextBox (MENU_X, STAT_Y, 23, 1);
	M_DrawTextBox (MENU_X, MENU_Y, 23, slist_maxs - slist_mins + 1);
	for (serv = slist_mins, line = 1 ; serv <= slist_maxs && serv < MAX_SERVER_LIST && slist[serv].server ; serv++, line++)
		M_Print (MENU_X + 18, line * 8 + MENU_Y, va("%1.21s", slist[serv].description));
	M_PrintWhite (MENU_X, STAT_Y - 4, "TRIANGLE = add server, SQUARE = edit");
	M_PrintWhite (MENU_X + 18, STAT_Y + 8, va("%1.22s", slist[slist_cursor].server));
	M_DrawCharacter (MENU_X + 8, (slist_cursor - slist_mins + 1) * 8 + MENU_Y, 12+((int)(realtime*4)&1));
}

void M_ServerList_Key (key)
{
	int	slist_length;

	if (!slist[0].server && key != K_ESCAPE && key != K_DEL)
		return;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (slist_cursor > 0)
		{
			if (keydown[K_CTRL])
				SList_Switch (slist_cursor, slist_cursor - 1);
			slist_cursor--;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (keydown[K_CTRL])
		{
			if (slist_cursor != SList_Length() - 1)
			{
				SList_Switch (slist_cursor, slist_cursor + 1);
				slist_cursor++;
			}
		}
		else if (slist_cursor < MAX_SERVER_LIST - 1 && slist[slist_cursor+1].server)
			slist_cursor++;
		break;

	case K_HOME:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor = 0;
		break;

	case K_END:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor = SList_Length() - 1;
		break;

	case K_PGUP:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor -= (slist_maxs - slist_mins);
		if (slist_cursor < 0)
			slist_cursor = 0;
		break;

	case K_PGDN:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor += (slist_maxs - slist_mins);
		if (slist_cursor >= MAX_SERVER_LIST)
			slist_cursor = MAX_SERVER_LIST - 1;
		while (!slist[slist_cursor].server)
			slist_cursor--;
		break;

	case K_ENTER:
		if (keydown[K_CTRL])
		{
			M_Menu_SEdit_f ();
			break;
		}
		m_state = m_main;
		M_ToggleMenu_f ();
		Cbuf_AddText (va("connect \"%s\"\n", slist[slist_cursor].server));
		break;

	//case 'e':
	//case 'E':
	case K_INS:
		M_Menu_SEdit_f ();
		break;

	case K_DEL:
		S_LocalSound ("misc/menu2.wav");
		if ((slist_length = SList_Length()) < MAX_SERVER_LIST)
		{
			if (keydown[K_CTRL] && slist_length > 0)
			{
				if (slist_cursor < slist_length - 1)
					memmove (&slist[slist_cursor+2], &slist[slist_cursor+1], (slist_length - slist_cursor - 1) * sizeof(slist[0]));
				SList_Reset_NoFree (slist_cursor + 1);
				SList_Set (slist_cursor + 1, "127.0.0.1", "<BLANK>");
				if (slist_length)
					slist_cursor++;
			}
			else
			{
				memmove (&slist[slist_cursor+1], &slist[slist_cursor], (slist_length - slist_cursor) * sizeof(slist[0]));
				SList_Reset_NoFree (slist_cursor);
				SList_Set (slist_cursor, "127.0.0.1", "<BLANK>");
			}
		}
		break;

	case K_LEFTARROW:
		S_LocalSound("misc/menu2.wav");
		if ((slist_length = SList_Length()) > 0)
		{
			SList_Reset (slist_cursor);
			if (slist_cursor > 0 && slist_length - 1 == slist_cursor)
			{
				slist_cursor--;
			}
			else
			{
				memmove (&slist[slist_cursor], &slist[slist_cursor+1], (slist_length - slist_cursor - 1) * sizeof(slist[0]));
				SList_Reset_NoFree (slist_length - 1);
			}
		}
		break;
	}

	if (slist_cursor < slist_mins)
	{
		slist_maxs -= (slist_mins - slist_cursor);
		slist_mins = slist_cursor;
	}
	if (slist_cursor > slist_maxs)
	{
		slist_mins += (slist_cursor - slist_maxs);
		slist_maxs = slist_cursor;
	}
}

#define	SERV_X	60
#define	SERV_Y	64
#define	DESC_X	60
#define	DESC_Y	40
#define	SERV_L	23
#define	DESC_L	23

#define	SLIST_BUFSIZE	128

static	char	slist_serv[SLIST_BUFSIZE], slist_desc[SLIST_BUFSIZE];
static	int	slist_serv_max, slist_serv_min, slist_desc_max, slist_desc_min, sedit_state;

void M_Menu_SEdit_f (void)
{
	int	size;

	key_dest = key_menu;
	m_state = m_sedit;
	m_entersound = true;

	sedit_state = 0;
	Q_strncpyz (slist_serv, slist[slist_cursor].server, sizeof(slist_serv));
	Q_strncpyz (slist_desc, slist[slist_cursor].description, sizeof(slist_desc));
	slist_serv_max = (size = strlen(slist_serv)) > SERV_L ? size : SERV_L;
	slist_serv_min = slist_serv_max - SERV_L;
	slist_desc_max = (size = strlen(slist_desc)) > DESC_L ? size : DESC_L;
	slist_desc_min = slist_desc_max - DESC_L;
}

void M_SEdit_Draw (void)
{
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic("gfx/qplaque.lmp"));
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ((320 - p->width) >> 1, 4, p);

	M_DrawTextBox (SERV_X, SERV_Y, 23, 1);
	M_DrawTextBox (DESC_X, DESC_Y, 23, 1);
	M_PrintWhite (SERV_X, SERV_Y - 4, "Hostname/IP:");
	M_PrintWhite (DESC_X, DESC_Y - 4, "Description:");
	M_Print (SERV_X + 9, SERV_Y + 8, va("%1.23s", slist_serv + slist_serv_min));
	M_Print (DESC_X + 9, DESC_Y + 8, va("%1.23s", slist_desc + slist_desc_min));
	if (sedit_state == 0)
		M_DrawCharacter (SERV_X + 9 + 8*(strlen(slist_serv) - slist_serv_min), SERV_Y + 8, 10+((int)(realtime*4)&1));
	else
		M_DrawCharacter (DESC_X + 9 + 8*(strlen(slist_desc) - slist_desc_min), DESC_Y + 8, 10+((int)(realtime*4)&1));
}

void M_SEdit_Key (int key)
{
	int	l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_ServerList_f ();
		break;

	case K_ENTER:
		SList_Set (slist_cursor, slist_serv, slist_desc);
		M_Menu_ServerList_f ();
		break;

	case K_UPARROW:
	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		sedit_state = !sedit_state;
		break;

	case K_INS:
        if (sedit_state == 0)
		   M_Menu_OSK_f(slist_serv, slist_serv, 16);
		else
		   M_Menu_OSK_f(slist_desc, slist_desc, 16);
		break;

	case K_BACKSPACE:
		if (sedit_state == 0)
		{
			if ((l = strlen(slist_serv)))
				slist_serv[--l] = 0;
			if (strlen(slist_serv) - 6 < slist_serv_min && slist_serv_min)
			{
				slist_serv_min--;
				slist_serv_max--;
			}
		}
		else
		{
			if ((l = strlen(slist_desc)))
				slist_desc[--l] = 0;
			if (strlen(slist_desc) - 6 < slist_desc_min && slist_desc_min)
			{
				slist_desc_min--;
				slist_desc_max--;
			}
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (sedit_state == 0)
		{
			l = strlen (slist_serv);
			if (l < SLIST_BUFSIZE - 1)
			{
				slist_serv[l+1] = 0;
				slist_serv[l] = key;
				l++;
			}
			if (l > slist_serv_max)
			{
				slist_serv_min++;
				slist_serv_max++;
			}
		}
		else
		{
			l = strlen (slist_desc);
			if (l < SLIST_BUFSIZE - 1)
			{
				slist_desc[l+1] = 0;
				slist_desc[l] = key;
				l++;
			}
			if (l > slist_desc_max)
			{
				slist_desc_min++;
				slist_desc_max++;
			}
		}
		break;
	}
}

//=============================================================================
/* NET MENU */

int	m_net_cursor;
int m_net_items;
int m_net_saveHeight;

char *net_helpMessage [] =
{
/* .........1.........2.... */
  "                        ",
  " Two computers connected",
  "   through two modems.  ",
  "                        ",

  "                        ",
  " Two computers connected",
  " by a null-modem cable. ",
  "                        ",

  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	f = 32;

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen1.lmp");
	}
	else
	{
#ifdef WIN32
		p = NULL;
#else
		p = Draw_CachePic ("gfx/dim_modm.lmp");
#endif
	}

	if (p)
		M_DrawTransPic (72, f, p);

	f += 19;

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen2.lmp");
	}
	else
	{
#ifdef WIN32
		p = NULL;
#else
		p = Draw_CachePic ("gfx/dim_drct.lmp");
#endif
	}

	if (p)
		M_DrawTransPic (72, f, p);

	f += 19;
	if (ipxAvailable)
		p = Draw_CachePic ("gfx/netmen3.lmp");
	else
		p = Draw_CachePic ("gfx/dim_ipx.lmp");
	M_DrawTransPic (72, f, p);

	f += 19;
	if (tcpipAvailable)
		p = Draw_CachePic ("gfx/netmen4.lmp");
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
	M_DrawTransPic (72, f, p);

	if (m_net_items == 5)	// JDC, could just be removed
	{
		f += 19;
		p = Draw_CachePic ("gfx/netmen5.lmp");
		M_DrawTransPic (72, f, p);
	}

	f = (320-26*8)/2;
	M_DrawTextBox (f, 134, 24, 4);
	f += 8;
	M_Print (f, 142, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 150, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 158, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 166, net_helpMessage[m_net_cursor*4+3]);

	f = (int)(host_time * 10)%6;
	M_DrawTransPic (54, 32 + m_net_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 0:
			M_Menu_SerialConfig_f ();
			break;

		case 1:
			M_Menu_SerialConfig_f ();
			break;

		case 2:
			M_Menu_LanConfig_f ();
			break;

		case 3:
			M_Menu_LanConfig_f ();
			break;

		case 4:
// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0 && !serialAvailable)
		goto again;
	if (m_net_cursor == 1 && !serialAvailable)
		goto again;
	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;
}

//=============================================================================
/* OPTIONS MENU */
#define	SLIDER_RANGE	10
#define NUM_SUBMENU 4
#ifdef PSP_HARDWARE_VIDEO
enum 
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_SUBMENU,
	OPTIONS_ITEMS
};
enum 
{
	OPT_SUBMENU_0 = OPT_SUBMENU,
    //OPT_GAP_0,
	OPT_SCRSIZE,	
	OPT_GAMMA,		
	OPT_VSYNC,	
    OPT_MIPMAPS,
    OPT_MIPMAP_BIAS,
	//OPT_GAP_0_1,
	OPT_MUSICTYPE,
	OPT_MUSICVOL,
	OPT_SNDVOL,		
	OPT_CROSSHAIR,
    //OPT_GAP_0_2,
	OPT_WATERTRANS,
	OPT_TEX_SCALEDOWN,
	OPT_SIMPLE_PART,
	OPT_DITHERING,
	OPTIONS_ITEMS_0
};
enum 
{
	OPT_SUBMENU_1 = OPT_SUBMENU,
    //OPT_GAP_1,
	OPT_ALWAYRUN,
	OPT_IN_SPEED,
	OPT_IN_TOLERANCE,
	OPT_IN_ACCELERATION,	
	OPT_INVMOUSE,	
	OPT_NOMOUSE,
	OPT_MOUSELOOK,
	OPT_MOUSESTAFE,	
	OPT_IN_X_ADJUST,
	OPT_IN_Y_ADJUST,
    OPTIONS_ITEMS_1
};
/*
enum
{
	OPT_SUBMENU_X = OPT_SUBMENU,
    OPT_DETAIL,
	OPT_DETAIL_MIPMAPS,
    OPT_DETAIL_MIPMAP_BIAS,
    OPTIONS_ITEMS_X
};
*/
enum
{
	OPT_SUBMENU_2 = OPT_SUBMENU,
    OPT_DECAL_BLOOD,
    OPT_DECAL_BULLETS,
    OPT_DECAL_SPARKS,
    OPT_DECAL_EXPLOSIONS,
    OPT_DECAL_ALL,
    OPTIONS_ITEMS_2
};
enum
{
	OPT_SUBMENU_3 = OPT_SUBMENU,
	OPT_PART_EXPLOSIONS,
	OPT_PART_TRAILS,
	OPT_PART_SPARKS,
	OPT_PART_SPIKES,
	OPT_PART_GUNSHOTS,
	OPT_PART_BLOOD,
	OPT_PART_TELESPLASH,
	OPT_PART_BLOBS,
	OPT_PART_LAVASPLASH,
	OPT_PART_FLAMES,
	OPT_PART_LIGHTNING,
	OPT_PART_FLIES,
	OPT_PART_MUZZLEFLASH,
	OPT_PART_ALL,
    OPTIONS_ITEMS_3
};
#else
enum 
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_SUBMENU,
	OPTIONS_ITEMS
};
enum 
{
	OPT_SUBMENU_0 = OPT_SUBMENU,
    //OPT_GAP_0,
	OPT_SCRSIZE,	
	OPT_GAMMA,		
	OPT_VSYNC,	
    //OPT_GAP_0_1,
	OPT_MUSICTYPE,
	OPT_MUSICVOL,
	OPT_SNDVOL,		
	OPT_CROSSHAIR,	
    OPTIONS_ITEMS_0
};
enum 
{
	OPT_SUBMENU_1 = OPT_SUBMENU,
    //OPT_GAP_1,
	OPT_ALWAYRUN,
	OPT_IN_SPEED,
	OPT_IN_TOLERANCE,
	OPT_IN_ACCELERATION,	
	OPT_INVMOUSE,	
	OPT_NOMOUSE,
	OPT_MOUSELOOK,
	OPT_MOUSESTAFE,
	OPT_IN_X_ADJUST,
	OPT_IN_Y_ADJUST,	
    OPTIONS_ITEMS_1
};
#endif

int	options_cursor;
int m_submenu = 0;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;
}

//Crow_bar.
void MenuQMBSwitch(int value, char *name, int dir)
{
	char full_name[32];
	sprintf(full_name, "r_part_%s", name);

	int chvalue = 0;
	
	if (value == 0)
	{
		if(dir == 1)
		   chvalue = 1;
		else
		   chvalue = 0;
    }
	else if (value == 1)
	{
        if(dir == 1)
		   chvalue = 2;
		else
		   chvalue = 0;
	}
	else if (value == 2)
	{
        if(dir == 1)
		   chvalue = 2;
		else
		   chvalue = 1;
    }

	Cvar_SetValue (full_name, chvalue);

}

void M_AdjustSliders (int dir)
{
	S_LocalSound ("misc/menu3.wav");

	switch (options_cursor)
	{
		case OPT_SUBMENU:
	        m_submenu += dir;
	        if (m_submenu > NUM_SUBMENU-1)
	        	m_submenu = 0;
	        else if (m_submenu < 0)
	        	m_submenu = NUM_SUBMENU-1;
	        break;	
	}
	
    if (m_submenu == 0)
    {
    	switch (options_cursor)
        {
			case OPT_SCRSIZE:	// screen size
				scr_viewsize.value += dir * 10;
				if (scr_viewsize.value < 30)
					scr_viewsize.value = 30;
				if (scr_viewsize.value > 120)
					scr_viewsize.value = 120;
				Cvar_SetValue ("viewsize", scr_viewsize.value);
				break;
			case OPT_GAMMA:	// gamma
				v_gamma.value -= dir * 0.05;
				if (v_gamma.value < 0.5)
					v_gamma.value = 0.5;
				if (v_gamma.value > 1)
					v_gamma.value = 1;
				Cvar_SetValue ("gamma", v_gamma.value);
				break;
			case OPT_MUSICVOL:	// music volume
#ifdef WIN32
				bgmvolume.value += dir * 1.0;
#else
				bgmvolume.value += dir * 0.1;
#endif
				if (bgmvolume.value < 0)
					bgmvolume.value = 0;
				if (bgmvolume.value > 1)
					bgmvolume.value = 1;
				Cvar_SetValue ("bgmvolume", bgmvolume.value);
				changeMp3Volume = 1;
				break;
			case OPT_SNDVOL:	// sfx volume
				volume.value += dir * 0.1;
				if (volume.value < 0)
					volume.value = 0;
				if (volume.value > 1)
					volume.value = 1;
				Cvar_SetValue ("volume", volume.value);
				break;
			case OPT_MUSICTYPE: // bgm type
				if (strcmpi(bgmtype.string,"cd") == 0)
				{
						Cvar_Set("bgmtype","none");
						bmg_type_changed = true;
				}
				else
				{
						Cvar_Set("bgmtype","cd");
						bmg_type_changed = true;
				}
				break;
			case OPT_VSYNC:	
				Cvar_SetValue ("r_vsync", !r_vsync.value);
				break;
            case OPT_MIPMAPS:
				Cvar_SetValue ("r_mipmaps", !r_mipmaps.value);
				break;

			case OPT_MIPMAP_BIAS:	// mipmapping bais
				r_mipmaps_bias.value += dir * 0.5;
				if (r_mipmaps_bias.value < -10)
					r_mipmaps_bias.value = -10;
				if (r_mipmaps_bias.value > 0)
					r_mipmaps_bias.value = 0;

				Cvar_SetValue ("r_mipmaps_bias", r_mipmaps_bias.value);
				break;

			case OPT_CROSSHAIR:	
				Cvar_SetValue ("crosshair", !crosshair.value);
				break;
#ifdef PSP_HARDWARE_VIDEO
			case OPT_WATERTRANS:	// wateralpha
				r_wateralpha.value += dir * 0.1;
				if (r_wateralpha.value < 0)
					r_wateralpha.value = 0;
				if (r_wateralpha.value > 1)
					r_wateralpha.value = 1;
				
				Cvar_SetValue ("r_wateralpha", r_wateralpha.value);
				break;
			case OPT_TEX_SCALEDOWN:	
				Cvar_SetValue ("r_tex_scale_down", !r_tex_scale_down.value);
				break;
			case OPT_SIMPLE_PART:	
				Cvar_SetValue ("r_particles_simple", !r_particles_simple.value);
				break;
			case OPT_DITHERING:	
				Cvar_SetValue ("r_dithering", !r_dithering.value);
				break;
#endif	

        }
    }
    else if (m_submenu == 1)
    {
       	switch (options_cursor)
        {
			case OPT_ALWAYRUN:	// allways run
				if (cl_forwardspeed.value > 200)
				{
					Cvar_SetValue ("cl_forwardspeed", 200);
					Cvar_SetValue ("cl_backspeed", 200);
				}
				else
				{
					Cvar_SetValue ("cl_forwardspeed", 400);
					Cvar_SetValue ("cl_backspeed", 400);
				}
				break;

       		case OPT_IN_SPEED:	// mouse speed
				in_sensitivity.value += dir * 0.5;
				if (in_sensitivity.value < 1)
					in_sensitivity.value = 1;
				if (in_sensitivity.value > 11)
					in_sensitivity.value = 11;
				Cvar_SetValue ("sensitivity", in_sensitivity.value);
				break;
       		
       		case OPT_IN_TOLERANCE:	// mouse tolerance
				in_tolerance.value += dir * 0.05;
				if (in_tolerance.value < 0)
					in_tolerance.value = 0;
				if (in_tolerance.value > 1)
					in_tolerance.value = 1;
				Cvar_SetValue ("tolerance", in_tolerance.value);
				break;

       		case OPT_IN_ACCELERATION:	// mouse tolerance
				in_acceleration.value -= dir * 0.25;
				if (in_acceleration.value < 0.5)
					in_acceleration.value = 0.5;
				if (in_acceleration.value > 2)
					in_acceleration.value = 2;
				Cvar_SetValue ("acceleration", in_acceleration.value);
				break;
				
			case OPT_IN_X_ADJUST:	
				in_x_axis_adjust.value += dir*5;
				if (in_x_axis_adjust.value < -127)
					in_x_axis_adjust.value = -127;
				if (in_x_axis_adjust.value > 127)
					in_x_axis_adjust.value = 127;
				Cvar_SetValue ("in_x_axis_adjust", in_x_axis_adjust.value);
				break;
				
			case OPT_IN_Y_ADJUST:	
				in_y_axis_adjust.value += dir*5;
				if (in_y_axis_adjust.value < -127)
					in_y_axis_adjust.value = -127;
				if (in_y_axis_adjust.value > 127)
					in_y_axis_adjust.value = 127;
				Cvar_SetValue ("in_y_axis_adjust", in_y_axis_adjust.value);
				break;
								
			case OPT_INVMOUSE:	// invert mouse
				Cvar_SetValue ("m_pitch", -m_pitch.value);
				break;
			case OPT_NOMOUSE:	// disable mouse
				Cvar_SetValue ("in_disable_analog", !in_disable_analog.value);
				break;
			case OPT_MOUSESTAFE:
				Cvar_SetValue ("in_analog_strafe", !in_analog_strafe.value);
				break;
			case OPT_MOUSELOOK:
				Cvar_SetValue ("in_mlook", !in_mlook.value);
				break;
        }	
    }
	else if (m_submenu == 2)
    {
       	switch (options_cursor)
        {
			case OPT_DECAL_BLOOD:
                Cvar_SetValue ("r_decal_blood", !r_decal_blood.value);
                break;
            case OPT_DECAL_BULLETS:
                Cvar_SetValue ("r_decal_bullets", !r_decal_bullets.value);
                break;
            case OPT_DECAL_SPARKS:
                Cvar_SetValue ("r_decal_sparks", !r_decal_sparks.value);
                break;
            case OPT_DECAL_EXPLOSIONS:
                Cvar_SetValue ("r_decal_explosions", !r_decal_explosions.value);
                break;
            case OPT_DECAL_ALL:
			    Cbuf_AddText("toggledecals\n");
                break;
        }
    }
    else if (m_submenu == 3)
    {
       	switch (options_cursor)
        {
		    case OPT_PART_EXPLOSIONS:
                MenuQMBSwitch(r_part_explosions.value, "explosions", dir);
			    break;
			case OPT_PART_TRAILS:
                MenuQMBSwitch(r_part_trails.value, "trails", dir);
				break;
			case OPT_PART_SPARKS:
			    MenuQMBSwitch(r_part_sparks.value, "sparks", dir);
                break;
			case OPT_PART_SPIKES:
			    MenuQMBSwitch(r_part_spikes.value, "spikes", dir);
                break;
			case OPT_PART_GUNSHOTS:
  			    MenuQMBSwitch(r_part_gunshots.value, "gunshots", dir);
                break;
			case OPT_PART_BLOOD:
                MenuQMBSwitch(r_part_blood.value, "blood", dir);
                break;
			case OPT_PART_TELESPLASH:
			    MenuQMBSwitch(r_part_telesplash.value, "telesplash", dir);
                break;
            case OPT_PART_BLOBS:
                MenuQMBSwitch(r_part_blobs.value, "blobs", dir);
                break;
			case OPT_PART_LAVASPLASH:
			    MenuQMBSwitch(r_part_lavasplash.value, "lavasplash", dir);
                break;
			case OPT_PART_FLAMES:
				MenuQMBSwitch(r_part_flames.value, "flames", dir);
                break;
			case OPT_PART_LIGHTNING:
				MenuQMBSwitch(r_part_lightning.value, "lightning", dir);
                break;
			case OPT_PART_FLIES:
               	MenuQMBSwitch(r_part_flies.value, "flies", dir);
                break;
			case OPT_PART_MUZZLEFLASH:
			    MenuQMBSwitch(r_part_muzzleflash.value, "muzzleflash", dir);
                break;
                
            case OPT_PART_ALL:
			    Cbuf_AddText("toggleparticles\n");
                break;
		}
    }
}

void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_Options_Draw (void)
{
	float	 r;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print (16, 32+(OPT_CUSTOMIZE*8), "    Customize controls");
	M_Print (16, 32+(OPT_CONSOLE*8),   "         Go to console");
	M_Print (16, 32+(OPT_DEFAULTS*8),  "     Reset to defaults");

	switch (m_submenu)
	{
        case 0:    
			M_Print (16, 32+(OPT_SCRSIZE*8),   "           Screen size");
			r = (scr_viewsize.value - 30) / (120 - 30);
			M_DrawSlider (220, 32+(OPT_SCRSIZE*8), r);
		
			M_Print (16, 32+(OPT_GAMMA*8), 	   "            Brightness");
			r = (1.0 - v_gamma.value) / 0.5;
			M_DrawSlider (220, 32+(OPT_GAMMA*8), r);
		
			M_Print (16, 32+(OPT_MUSICVOL*8), "       CD Music Volume");
			r = bgmvolume.value;
			M_DrawSlider (220, 32+(OPT_MUSICVOL*8), r);
		
			M_Print (16, 32+(OPT_SNDVOL*8),   "          Sound Volume");
			r = volume.value;
			M_DrawSlider (220, 32+(OPT_SNDVOL*8), r);

			M_Print (16, 32+(OPT_MUSICTYPE*8),"            Music Type");
			if (strcmpi(bgmtype.string,"cd") == 0)
				M_Print (220, 32+(OPT_MUSICTYPE*8), "CD/MP3");
			else
				M_Print (220, 32+(OPT_MUSICTYPE*8), "None");
		
			M_Print (16, 32+(OPT_VSYNC*8),           "         VSync Enabled");
			M_DrawCheckbox (220, 32+(OPT_VSYNC*8), r_vsync.value);

			M_Print (16, 32+(OPT_MIPMAPS*8),        "            MipMapping");
			M_DrawCheckbox (220, 32+(OPT_MIPMAPS*8), r_mipmaps.value);

			M_Print (16, 32+(OPT_MIPMAP_BIAS*8),		"         MipMap Amount");
			r = (r_mipmaps_bias.value + 10) / 10;
			M_DrawSlider (220, 32+(OPT_MIPMAP_BIAS*8), r);

			M_Print (16, 32+(OPT_CROSSHAIR*8),	"        Show Crosshair");
			M_DrawCheckbox (220, 32+(OPT_CROSSHAIR*8), crosshair.value);

		#ifdef PSP_HARDWARE_VIDEO
			M_Print (16, 32+(OPT_WATERTRANS*8), "     Water tranparency");
			M_DrawSlider (220, 32+(OPT_WATERTRANS*8), r_wateralpha.value);
		
			M_Print (16, 32+(OPT_TEX_SCALEDOWN*8), "    Texture Scale Down");
			M_DrawCheckbox (220, 32+(OPT_TEX_SCALEDOWN*8), r_tex_scale_down.value);
		
			M_Print (16, 32+(OPT_SIMPLE_PART*8),    "      Simple Particles");
			M_DrawCheckbox (220, 32+(OPT_SIMPLE_PART*8), r_particles_simple.value);
			
			M_Print (16, 32+(OPT_DITHERING*8),      "             Dithering");
			M_DrawCheckbox (220, 32+(OPT_DITHERING*8), r_dithering.value);
			
		#endif
			break;
        case 1:
        	M_Print (16, 32+(OPT_ALWAYRUN*8),        "            Always Run");
			M_DrawCheckbox (220, 32+(OPT_ALWAYRUN*8), cl_forwardspeed.value > 200);
		    
		#ifdef PSP
			M_Print (16, 32+(OPT_IN_SPEED*8), 		 "           A-Nub Speed");
		#else
			M_Print (16, 32+(OPT_IN_SPEED*8), 		 "           Mouse Speed");
		#endif
			r = (in_sensitivity.value - 1)/10;
			M_DrawSlider (220, 32+(OPT_IN_SPEED*8), r);

			M_Print (16, 32+(OPT_IN_ACCELERATION*8), "    A-Nub Acceleration");
			r = 1.0f -((in_acceleration.value - 0.5f)/1.5f);
			M_DrawSlider (220, 32+(OPT_IN_ACCELERATION*8), r);

			M_Print (16, 32+(OPT_IN_TOLERANCE*8), 	 "      A-Nub Tollerance");
			r = (in_tolerance.value )/1.0f;
			M_DrawSlider (220, 32+(OPT_IN_TOLERANCE*8), r);
		
			M_Print (16, 32+(OPT_IN_X_ADJUST*8), 	 "         Adjust Axis X");	
			r = (128+in_x_axis_adjust.value)/255;
			M_DrawSlider (220, 32+(OPT_IN_X_ADJUST*8), r);

			M_Print (16, 32+(OPT_IN_Y_ADJUST*8), 	 "         Adjust Axis Y");	
			r = (128+in_y_axis_adjust.value)/255;
			M_DrawSlider (220, 32+(OPT_IN_Y_ADJUST*8), r);
		
		
		#ifdef PSP
			M_Print (16, 32+(OPT_INVMOUSE*8),        "          Invert A-Nub");
		#else
			M_Print (16, 32+(OPT_INVMOUSE*8),        "          Invert Mouse");
		#endif
			M_DrawCheckbox (220, 32+(OPT_INVMOUSE*8),       m_pitch.value < 0);
		
			M_Print (16, 32+(OPT_MOUSELOOK*8),       "            A-Nub Look");
			M_DrawCheckbox (220, 32+(OPT_MOUSELOOK*8),         in_mlook.value);

			M_Print (16, 32+(OPT_NOMOUSE*8),         "         Disable A-Nub");
			M_DrawCheckbox (220, 32+(OPT_NOMOUSE*8), in_disable_analog.value );
		
			M_Print (16, 32+(OPT_MOUSESTAFE*8),		 "         A-Nub Stafing");
			M_DrawCheckbox (220, 32+(OPT_MOUSESTAFE*8), in_analog_strafe.value );
			break;
		case 2:
            M_Print (16, 32+(OPT_DECAL_BLOOD*8),     "           Decal Blood");
			M_DrawCheckbox (220, 32+(OPT_DECAL_BLOOD*8), r_decal_blood.value);

			M_Print (16, 32+(OPT_DECAL_BULLETS*8),   "         Decal Bullets");
			M_DrawCheckbox (220, 32+(OPT_DECAL_BULLETS*8), r_decal_bullets.value);

			M_Print (16, 32+(OPT_DECAL_SPARKS*8),    "          Decal Sparks");
			M_DrawCheckbox (220, 32+(OPT_DECAL_SPARKS*8), r_decal_sparks.value);

			M_Print (16, 32+(OPT_DECAL_EXPLOSIONS*8),"      Decal Explosions");
			M_DrawCheckbox (220, 32+(OPT_DECAL_EXPLOSIONS*8), r_decal_explosions.value);

			M_Print (16, 32+(OPT_DECAL_ALL*8),       "   Toggle All Decals  ");

			break;

		case 3:
		    M_Print (16, 32+(OPT_PART_EXPLOSIONS*8), "   Particle Explosions");
		    M_Print (220, 32+(OPT_PART_EXPLOSIONS*8), !r_part_explosions.value ? "std" : r_part_explosions.value == 2 ? "q3" : "qmb");
			M_Print (16, 32+(OPT_PART_TRAILS*8),     "       Particle Trails");
		    M_Print (220, 32+(OPT_PART_TRAILS*8), !r_part_trails.value ? "std" : r_part_trails.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_SPARKS*8),     "       Particle Sparks");
		    M_Print (220, 32+(OPT_PART_SPARKS*8), !r_part_sparks.value ? "std" : r_part_sparks.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_SPIKES*8),     "       Particle Spikes");
		    M_Print (220, 32+(OPT_PART_SPIKES*8), !r_part_spikes.value ? "std" : r_part_spikes.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_GUNSHOTS*8),   "    Particle Gun Shots");
		    M_Print (220, 32+(OPT_PART_GUNSHOTS*8), !r_part_gunshots.value ? "std" : r_part_gunshots.value == 2 ? "q3" : "qmb");
            M_Print (16, 32+(OPT_PART_BLOOD*8),      "        Particle Blood");
		    M_Print (220, 32+(OPT_PART_BLOOD*8), !r_part_blood.value ? "std" : r_part_blood.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_TELESPLASH*8), "  Particle Tele Splash");
		    M_Print (220, 32+(OPT_PART_TELESPLASH*8), !r_part_telesplash.value ? "std" : r_part_telesplash.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_BLOBS*8),      "        Particle Blobs");
		    M_Print (220, 32+(OPT_PART_BLOBS*8), !r_part_blobs.value ? "std" : r_part_blobs.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_LAVASPLASH*8), "  Particle Lava Splash");
		    M_Print (220, 32+(OPT_PART_LAVASPLASH*8), !r_part_lavasplash.value ? "std" : r_part_lavasplash.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_FLAMES*8),     "       Particle Flames");
		    M_Print (220, 32+(OPT_PART_FLAMES*8), !r_part_flames.value ? "std" : r_part_flames.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_LIGHTNING*8),  "    Particle Lightning");
		    M_Print (220, 32+(OPT_PART_LIGHTNING*8), !r_part_lightning.value ? "std" : r_part_lightning.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_FLIES*8),      "        Particle Flies");
		    M_Print (220, 32+(OPT_PART_FLIES*8), !r_part_flies.value ? "std" : r_part_flies.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_MUZZLEFLASH*8)," Particle Muzzle Flash");
		    M_Print (220, 32+(OPT_PART_MUZZLEFLASH*8), !r_part_muzzleflash.value ? "std" : r_part_muzzleflash.value == 2 ? "q3" : "qmb");
		    M_Print (16, 32+(OPT_PART_ALL*8)," Toggle All Perticles ");

			break;
	}
	
	M_PrintWhite (16, 32+(OPT_SUBMENU*8),	 "        Select Submenu");
    switch (m_submenu)
        {
        case 0:
            M_PrintWhite (220, 32+(OPT_SUBMENU*8), "More options");         
            break;
        case 1:
            M_PrintWhite (220, 32+(OPT_SUBMENU*8), "Less options");         
            break;
        case 2:
            M_PrintWhite (220, 32+(OPT_SUBMENU*8), "Decals options");
            break;
        case 3:
            M_PrintWhite (220, 32+(OPT_SUBMENU*8), "Particles options");
            break;
        default:
            break;
        }                         	
	// Cursor
	M_DrawCharacter (200, 32 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText ("exec default.cfg\n");
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0) {
			if (m_submenu == 0)
			    options_cursor = OPTIONS_ITEMS_0-1;
	        if (m_submenu == 1)
			    options_cursor = OPTIONS_ITEMS_1-1;
			if (m_submenu == 2)
			    options_cursor = OPTIONS_ITEMS_2-1;
            if (m_submenu == 3)
			    options_cursor = OPTIONS_ITEMS_3-1;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor++;
        if (m_submenu == 0)
            if (options_cursor >= OPTIONS_ITEMS_0)
			    options_cursor = 0;
        if (m_submenu == 1)
            if (options_cursor >= OPTIONS_ITEMS_1)
			    options_cursor = 0;
        if (m_submenu == 2)
            if (options_cursor >= OPTIONS_ITEMS_2)
			    options_cursor = 0;
        if (m_submenu == 3)
            if (options_cursor >= OPTIONS_ITEMS_3)
			    options_cursor = 0;
		break;

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}
}

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 10", 		"change weapon"},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
#ifdef PSP
{"+mlook", 			"analog nub look"},
{"+klook", 			"d-pad look"},
#else
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
#endif
{"+moveup",			"swim up"},
{"+movedown",		"swim down"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

int		keys_cursor;
int		bind_grab;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

#ifdef PSP
	if (bind_grab)
		M_Print (12, 32, "Press a button for this action");
	else
		M_Print (18, 32, "Press CROSS to change, SQUARE to clear");
#else
	if (bind_grab)
		M_Print (12, 32, "Press a key or button for this action");
	else
		M_Print (18, 32, "Enter to change, backspace to clear");
#endif

// search for known bindings
	for (i=0 ; i<NUMCOMMANDS ; i++)
	{
		y = 48 + 8*i;

		M_Print (16, y, bindnames[i][1]);

		l = strlen (bindnames[i][0]);

		M_FindKeysForCommand (bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 48 + keys_cursor*8, '=');
	else
		M_DrawCharacter (130, 48 + keys_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];

	if (bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("misc/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

void M_Menu_Video_f (void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;
}


void M_Video_Draw (void)
{
	(*vid_menudrawfn) ();
}


void M_Video_Key (int key)
{
	(*vid_menukeyfn) (key);
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/help%i.lmp", help_page)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
int		m_quit_prevstate;
qboolean	wasInMenus;

int	m_quit_cursor;
int m_quit_items;


/* .........1.........2....
char *quitMessage [] =
{

  "  Are you gonna quit    ",
  "  this game just like   ",  //msg:0
  "   everything else?     ",
  "                        ",
 
  " Milord, methinks that  ",
  "   thou art a lowly     ",  //msg:1
  " quitter. Is this true? ",
  "                        ",

  " Do I need to bust your ",
  "  face open for trying  ",  //msg:2
  "        to quit?        ",
  "                        ",

  " Man, I oughta smack you",
  "   for trying to quit!  ",  //msg:3
  "     Press X to get     ",
  "      smacked out.      ",
 
  " Press X to quit like a ",
  "   big loser in life.   ",  //msg:4
  "  Press O to stay proud ",
  "    and successful!     ",
 
  "   If you press X to    ",
  "  quit, I will summon   ",  //msg:5
  "  Satan all over your   ",
  "      hard drive!       ",
 
  "  Um, Asmodeus dislikes ",
  " his children trying to ",  //msg:6
  " quit. Press X to return",
  "   to your Tinkertoys.  ",

  "      Really quit?      ",
  "                        ",  //msg:7
  "  Press X to quit,      ",
  " or O to continue.      ",

  "  If you quit now, I'll ",
  "  throw a blanket-party ",  //msg:8
  "   for you next time!   ",
  "                        ",

  " You are Crazy?         ",
  " Press X for Yes        ",  //msg:9
  " or O for no!           ",
  " Global Random Ha Ha Ha  "
};
*/

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;
/*
    Con_Printf ("open source\n");

	FILE *fd = fopen("credits.txt","rb");
	FILE *fd1 = fopen("credits.dat","wb");

	fseek (fd, 0, SEEK_END);
	int len = ftell (fd);
	fseek (fd, 0, SEEK_SET);

	char *str = malloc(len);

	fread (str, 1, len, fd);
    Con_Printf ("source: %s\n",str);

	char *out = strencrypt(str, 110901, len);
    Con_Printf ("enc: %s\n", out);
	fwrite(out, len, 1, fd1);

	Con_Printf ("write encrypted\n");

    //char *out2 = strdecrypt(out, 110901, sizeof(strf));
    //Con_Printf ("dec: %s\n", out2);

    free(out);
    //free(out2);
*/
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
#ifdef PSP
	case K_ENTER:
#endif
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}

}


void M_Quit_Draw (void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

#ifdef WIN32
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, 12,  "  Quake version 1.09 by id Software\n\n");
	M_PrintWhite (16, 28,  "Programming        Art \n");
	M_Print (16, 36,  " John Carmack       Adrian Carmack\n");
	M_Print (16, 44,  " Michael Abrash     Kevin Cloud\n");
	M_Print (16, 52,  " John Cash          Paul Steed\n");
	M_Print (16, 60,  " Dave 'Zoid' Kirsch\n");
	M_PrintWhite (16, 68,  "Design             Biz\n");
	M_Print (16, 76,  " John Romero        Jay Wilbur\n");
	M_Print (16, 84,  " Sandy Petersen     Mike Wilson\n");
	M_Print (16, 92,  " American McGee     Donna Jackson\n");
	M_Print (16, 100,  " Tim Willits        Todd Hollenshead\n");
	M_PrintWhite (16, 108, "Support            Projects\n");
	M_Print (16, 116, " Barrett Alexander  Shawn Green\n");
	M_PrintWhite (16, 124, "Sound Effects\n");
	M_Print (16, 132, " Trent Reznor and Nine Inch Nails\n\n");
	M_PrintWhite (16, 140, "Quake is a trademark of Id Software,\n");
	M_PrintWhite (16, 148, "inc., (c)1996 Id Software, inc. All\n");
	M_PrintWhite (16, 156, "rights reserved. NIN logo is a\n");
	M_PrintWhite (16, 164, "registered trademark licensed to\n");
	M_PrintWhite (16, 172, "Nothing Interactive, Inc. All rights\n");
	M_PrintWhite (16, 180, "reserved. Press y to exit\n");
#elif defined PSP
	M_DrawTextBox (56, 76, 24, 4);
	M_Print (64, 84,	"      Really quit?      ");
	M_Print (64, 92,	"                        ");
	M_Print (64, 100,	"  Press CROSS to quit,  ");
	M_Print (64, 108,	" or CIRCLE to continue. ");
#else
	M_DrawTextBox (56, 76, 24, 4);
	M_Print (64, 84,  quitMessage[msgNumber*4+0]);
	M_Print (64, 92,  quitMessage[msgNumber*4+1]);
	M_Print (64, 100, quitMessage[msgNumber*4+2]);
	M_Print (64, 108, quitMessage[msgNumber*4+3]);
#endif
}
//=============================================================================
/* OSK IMPLEMENTATION */
#define CHAR_SIZE 8
#define MAX_Y 8
#define MAX_X 12

#define MAX_CHAR_LINE 36
#define MAX_CHAR      72

int  osk_pos_x = 0;
int  osk_pos_y = 0;
int  max_len   = 0;
int  m_old_state = 0;

char* osk_out_buff = NULL;
char  osk_buffer[128];

char *osk_text [] = 
	{ 
		" 1 2 3 4 5 6 7 8 9 0 - = ` ",
		" q w e r t y u i o p [ ]   ",
		"   a s d f g h j k l ; ' \\ ",
		"     z x c v b n m   , . / ",
		"                           ",
		" ! @ # $ % ^ & * ( ) _ + ~ ",
		" Q W E R T Y U I O P { }   ",
		"   A S D F G H J K L : \" | ",
		"     Z X C V B N M   < > ? "
	};

char *osk_help [] = 
	{ 
		"CONFIRM: ",
		" SQUARE  ",
		"CANCEL:  ",
		" CIRCLE  ",
		"DELETE:  ",
		" TRIAGLE ",
		"ADD CHAR:",
		" CROSS   ",
		""
	};

void M_Menu_OSK_f (char *input, char *output, int outlen)
{
	key_dest = key_menu;
	m_old_state = m_state;
	m_state = m_osk;
	m_entersound = false;
	max_len = outlen;
	strncpy(osk_buffer,input,max_len);
	osk_buffer[outlen] = '\0';
	osk_out_buff = output; 
}

void Con_OSK_f (char *input, char *output, int outlen)
{
	max_len = outlen;
	strncpy(osk_buffer,input,max_len);
	osk_buffer[outlen] = '\0';
	osk_out_buff = output; 
}


void M_OSK_Draw (void)
{
#ifdef PSP
	int x,y;
	int i;
	
	char *selected_line = osk_text[osk_pos_y]; 
	char selected_char[2];
	
	selected_char[0] = selected_line[1+(2*osk_pos_x)];
	selected_char[1] = '\0';
	if (selected_char[0] == ' ' || selected_char[0] == '\t') 
		selected_char[0] = 'X';
		
	y = 20;
	x = 16;

	M_DrawTextBox (10, 10, 		     26, 10);
	M_DrawTextBox (10+(26*CHAR_SIZE),    10,  10, 10);
	M_DrawTextBox (10, 10+(10*CHAR_SIZE),36,  3);
	
	for(i=0;i<=MAX_Y;i++) 
	{
		M_PrintWhite (x, y+(CHAR_SIZE*i), osk_text[i]);
		if (i % 2 == 0)
			M_Print      (x+(27*CHAR_SIZE), y+(CHAR_SIZE*i), osk_help[i]);
		else			
			M_PrintWhite (x+(27*CHAR_SIZE), y+(CHAR_SIZE*i), osk_help[i]);
	}
	
	int text_len = strlen(osk_buffer);
	if (text_len > MAX_CHAR_LINE) {
		
		char oneline[MAX_CHAR_LINE+1];
		strncpy(oneline,osk_buffer,MAX_CHAR_LINE);
		oneline[MAX_CHAR_LINE] = '\0';
		
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+2)), oneline );
		
		strncpy(oneline,osk_buffer+MAX_CHAR_LINE, text_len - MAX_CHAR_LINE);
		oneline[text_len - MAX_CHAR_LINE] = '\0';
		
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+3)), oneline );
		M_PrintWhite (x+4+(CHAR_SIZE*(text_len - MAX_CHAR_LINE)), y+4+(CHAR_SIZE*(MAX_Y+3)),"_");
	}
	else {
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+2)), osk_buffer );
		M_PrintWhite (x+4+(CHAR_SIZE*(text_len)), y+4+(CHAR_SIZE*(MAX_Y+2)),"_");
	}
	M_Print      (x+((((osk_pos_x)*2)+1)*CHAR_SIZE), y+(osk_pos_y*CHAR_SIZE), selected_char);

#endif
}

void M_OSK_Key (int key)
{
#ifdef PSP
	switch (key)
	{
	case K_RIGHTARROW:
		osk_pos_x++;
		if (osk_pos_x > MAX_X)
			osk_pos_x = MAX_X;
		break;
	case K_LEFTARROW:
		osk_pos_x--;
		if (osk_pos_x < 0)
			osk_pos_x = 0;
		break;
	case K_DOWNARROW:
		osk_pos_y++;
		if (osk_pos_y > MAX_Y)
			osk_pos_y = MAX_Y;
		break;
	case K_UPARROW:
		osk_pos_y--;
		if (osk_pos_y < 0)
			osk_pos_y = 0;
		break;
	case K_ENTER: 
		if (max_len > strlen(osk_buffer)) {
			char *selected_line = osk_text[osk_pos_y]; 
			char selected_char[2];
			
			selected_char[0] = selected_line[1+(2*osk_pos_x)];
			
			if (selected_char[0] == '\t')
				selected_char[0] = ' ';
			
			selected_char[1] = '\0';
			strcat(osk_buffer,selected_char);		
		}
		break;
	case K_DEL:
		if (strlen(osk_buffer) > 0) {
			osk_buffer[strlen(osk_buffer)-1] = '\0';	
		}
		break;
	case K_INS:
		strncpy(osk_out_buff,osk_buffer,max_len);
		
		m_state = m_old_state;
		break;
	case K_ESCAPE:
		m_state = m_old_state;
		break;
	default:
		break;
	}
#endif		
}

void Con_OSK_Key (int key)
{
#ifdef PSP
	switch (key)
	{
	case K_RIGHTARROW:
		osk_pos_x++;
		if (osk_pos_x > MAX_X)
			osk_pos_x = MAX_X;
		break;
	case K_LEFTARROW:
		osk_pos_x--;
		if (osk_pos_x < 0)
			osk_pos_x = 0;
		break;
	case K_DOWNARROW:
		osk_pos_y++;
		if (osk_pos_y > MAX_Y)
			osk_pos_y = MAX_Y;
		break;
	case K_UPARROW:
		osk_pos_y--;
		if (osk_pos_y < 0)
			osk_pos_y = 0;
		break;
	case K_ENTER: 
		if (max_len > strlen(osk_buffer)) {
			char *selected_line = osk_text[osk_pos_y]; 
			char selected_char[2];
			
			selected_char[0] = selected_line[1+(2*osk_pos_x)];
			
			if (selected_char[0] == '\t')
				selected_char[0] = ' ';
			
			selected_char[1] = '\0';
			strcat(osk_buffer,selected_char);		
		}
		break;
	case K_DEL:
		if (strlen(osk_buffer) > 0) {
			osk_buffer[strlen(osk_buffer)-1] = '\0';	
		}
		break;
	case K_INS:
		strncpy(osk_out_buff,osk_buffer,max_len);
		Con_SetOSKActive(false);
		break;
	case K_ESCAPE:
		Con_SetOSKActive(false);
		break;
	default:
		break;
	}
#endif		
}	

//=============================================================================

/* SERIAL CONFIG MENU */

int		serialConfig_cursor;
int		serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define	NUM_SERIALCONFIG_CMDS	6

static int ISA_uarts[]	= {0x3f8,0x2f8,0x3e8,0x2e8};
static int ISA_IRQs[]	= {4,3,4,3};
int serialConfig_baudrate[] = {9600,14400,19200,28800,38400,57600};

int		serialConfig_comport;
int		serialConfig_irq ;
int		serialConfig_baud;
char	serialConfig_phone[16];

void M_Menu_SerialConfig_f (void)
{
	int		n;
	int		port;
	int		baudrate;
	qboolean	useModem;

	key_dest = key_menu;
	m_state = m_serialconfig;
	m_entersound = true;
	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;

	(*GetComPortConfig) (0, &port, &serialConfig_irq, &baudrate, &useModem);

	// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;
	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}
	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;
	if (n == 6)
		n = 5;
	serialConfig_baud = n;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_SerialConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*directModem;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (SerialConfig)
		directModem = "Modem";
	else
		directModem = "Direct Connect";
	M_Print (basex, 32, va ("%s - %s", startJoin, directModem));
	basex += 8;

	M_Print (basex, serialConfig_cursor_table[0], "Port");
	M_DrawTextBox (160, 40, 4, 1);
	M_Print (168, serialConfig_cursor_table[0], va("COM%u", serialConfig_comport));

	M_Print (basex, serialConfig_cursor_table[1], "IRQ");
	M_DrawTextBox (160, serialConfig_cursor_table[1]-8, 1, 1);
	M_Print (168, serialConfig_cursor_table[1], va("%u", serialConfig_irq));

	M_Print (basex, serialConfig_cursor_table[2], "Baud");
	M_DrawTextBox (160, serialConfig_cursor_table[2]-8, 5, 1);
	M_Print (168, serialConfig_cursor_table[2], va("%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print (basex, serialConfig_cursor_table[3], "Modem Setup...");
		if (JoiningGame)
		{
			M_Print (basex, serialConfig_cursor_table[4], "Phone number");
			M_DrawTextBox (160, serialConfig_cursor_table[4]-8, 16, 1);
			M_Print (168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 7, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "Connect");
	}
	else
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 2, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "OK");
	}

	M_DrawCharacter (basex-8, serialConfig_cursor_table [serialConfig_cursor], 12+((int)(realtime*4)&1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter (168 + 8*strlen(serialConfig_phone), serialConfig_cursor_table [serialConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_SerialConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor--;
		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor++;
		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;
		break;

	case K_LEFTARROW:
		if (serialConfig_cursor > 2)
			break;
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;
			if (serialConfig_comport == 0)
				serialConfig_comport = 4;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;
			if (serialConfig_irq == 6)
				serialConfig_irq = 5;
			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;
			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:
		if (serialConfig_cursor > 2)
			break;
forward:
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;
			if (serialConfig_comport > 4)
				serialConfig_comport = 1;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;
			if (serialConfig_irq == 6)
				serialConfig_irq = 7;
			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;
			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_ENTER:
		if (serialConfig_cursor < 3)
			goto forward;

		m_entersound = true;

		if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_Menu_ModemConfig_f ();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

		// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem ();

		if (StartingGame)
		{
			M_Menu_GameOptions_f ();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;
		key_dest = key_game;
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText (va ("connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText ("connect\n");
		break;

	case K_BACKSPACE:
		if (serialConfig_cursor == 4)
		{
			if (strlen(serialConfig_phone))
				serialConfig_phone[strlen(serialConfig_phone)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;
		if (serialConfig_cursor == 4)
		{
			l = strlen(serialConfig_phone);
			if (l < 15)
			{
				serialConfig_phone[l+1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
	{
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;
	}
	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
	{
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
	}
}

//=============================================================================
/* MODEM CONFIG MENU */

int		modemConfig_cursor;
int		modemConfig_cursor_table [] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS	5

char	modemConfig_dialing;
char	modemConfig_clear [16];
char	modemConfig_init [32];
char	modemConfig_hangup [16];

void M_Menu_ModemConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_modemconfig;
	m_entersound = true;
	(*GetModemConfig) (0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}


void M_ModemConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print (basex, modemConfig_cursor_table[0], "Pulse Dialing");
	else
		M_Print (basex, modemConfig_cursor_table[0], "Touch Tone Dialing");

	M_Print (basex, modemConfig_cursor_table[1], "Clear");
	M_DrawTextBox (basex, modemConfig_cursor_table[1]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[1]+12, modemConfig_clear);
	if (modemConfig_cursor == 1)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_clear), modemConfig_cursor_table[1]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[2], "Init");
	M_DrawTextBox (basex, modemConfig_cursor_table[2]+4, 30, 1);
	M_Print (basex+8, modemConfig_cursor_table[2]+12, modemConfig_init);
	if (modemConfig_cursor == 2)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_init), modemConfig_cursor_table[2]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[3], "Hangup");
	M_DrawTextBox (basex, modemConfig_cursor_table[3]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[3]+12, modemConfig_hangup);
	if (modemConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_hangup), modemConfig_cursor_table[3]+12, 10+((int)(realtime*4)&1));

	M_DrawTextBox (basex, modemConfig_cursor_table[4]-8, 2, 1);
	M_Print (basex+8, modemConfig_cursor_table[4], "OK");

	M_DrawCharacter (basex-8, modemConfig_cursor_table [modemConfig_cursor], 12+((int)(realtime*4)&1));
}


void M_ModemConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_SerialConfig_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor--;
		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor++;
		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			S_LocalSound ("misc/menu1.wav");
		}
		break;

	case K_ENTER:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			m_entersound = true;
		}

		if (modemConfig_cursor == 4)
		{
			(*SetModemConfig) (0, va ("%c", modemConfig_dialing), modemConfig_clear, modemConfig_init, modemConfig_hangup);
			m_entersound = true;
			M_Menu_SerialConfig_f ();
		}
		break;

	case K_BACKSPACE:
		if (modemConfig_cursor == 1)
		{
			if (strlen(modemConfig_clear))
				modemConfig_clear[strlen(modemConfig_clear)-1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen(modemConfig_init))
				modemConfig_init[strlen(modemConfig_init)-1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen(modemConfig_hangup))
				modemConfig_hangup[strlen(modemConfig_hangup)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen(modemConfig_clear);
			if (l < 15)
			{
				modemConfig_clear[l+1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen(modemConfig_init);
			if (l < 29)
			{
				modemConfig_init[l+1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen(modemConfig_hangup);
			if (l < 15)
			{
				modemConfig_hangup[l+1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {72, 92, 124};
#define NUM_LANCONFIG_CMDS	3

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor == 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*protocol;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (IPXConfig)
		protocol = "IPX";
	else
		protocol = "TCP/IP";
	M_Print (basex, 32, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 52, "Address:");
	if (IPXConfig)
		M_Print (basex+9*8, 52, my_ipx_address);
	else
		M_Print (basex+9*8, 52, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Search for local games...");
		M_Print (basex, 108, "Join game at:");
		M_DrawTextBox (basex+8, lanConfig_cursor_table[2]-8, 22, 1);
		M_Print (basex+16, lanConfig_cursor_table[2], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 2)
		M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [2], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_INS:
		if (lanConfig_cursor == 0)
		{
			M_Menu_OSK_f(lanConfig_portname, lanConfig_portname, 6);
			break;
		}

		if (lanConfig_cursor == 2)
		{
			M_Menu_OSK_f(lanConfig_joinname, lanConfig_joinname, 22);
			break;
		}
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem ();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 2)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 2)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 2)
		{
			l = strlen(lanConfig_joinname);
			if (l < 21)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	l =  Q_atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
   {"start", "Command HQ"},  // 0

   {"hip1m1", "The Pumping Station"},          // 1
   {"hip1m2", "Storage Facility"},
   {"hip1m3", "The Lost Mine"},
   {"hip1m4", "Research Facility"},
   {"hip1m5", "Military Complex"},

   {"hip2m1", "Ancient Realms"},          // 6
   {"hip2m2", "The Black Cathedral"},
   {"hip2m3", "The Catacombs"},
   {"hip2m4", "The Crypt"},
   {"hip2m5", "Mortum's Keep"},
   {"hip2m6", "The Gremlin's Domain"},

   {"hip3m1", "Tur Torment"},       // 12
   {"hip3m2", "Pandemonium"},
   {"hip3m3", "Limbo"},
   {"hip3m4", "The Gauntlet"},

   {"hipend", "Armagon's Lair"},       // 16

   {"hipdm1", "The Edge of Oblivion"}           // 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] =
{
   {"Scourge of Armagon", 0, 1},
   {"Fortress of the Dead", 1, 5},
   {"Dominion of Darkness", 6, 6},
   {"The Rift", 12, 4},
   {"Final Level", 16, 1},
   {"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

//==================== Map Find System By Crow_bar =============================
int user_maps_num = 0;
char  user_levels[256][MAX_QPATH];

void Map_Finder(void)
{
	SceUID dir = sceIoDopen(va("%s/maps",com_gamedir));
	if(dir < 0)
	{
		return;
	}

	SceIoDirent dirent;

    memset(&dirent, 0, sizeof(SceIoDirent));

	while(sceIoDread(dir, &dirent) > 0)
	{
		if(dirent.d_name[0] == '.')
		{
			continue;
		}

		if(!strcmp(COM_FileExtension(dirent.d_name),"bsp")||
		   !strcmp(COM_FileExtension(dirent.d_name),"BSP"))
	    {
			char ntype[32];
			COM_StripExtension(dirent.d_name, ntype);
			sprintf(user_levels[user_maps_num],"%s", ntype);
			user_maps_num = user_maps_num + 1;
		}
	    memset(&dirent, 0, sizeof(SceIoDirent));
	}
    sceIoDclose(dir);
}
//==============================================================================
void M_Menu_GameOptions_f (void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 112, 120};
#define	NUM_GAMEOPTIONS	9
int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	qpic_t	*p;
	int		x;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_DrawTextBox (152, 32, 10, 1);
	M_Print (160, 40, "begin game");

	M_Print (0, 56, "      Max players");
	M_Print (160, 56, va("%i", maxplayers) );

	M_Print (0, 64, "        Game Type");
	if (coop.value)
		M_Print (160, 64, "Cooperative");
	else
		M_Print (160, 64, "Deathmatch");

	M_Print (0, 72, "        Teamplay");
	if (rogue)
	{
		char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, 72, msg);
	}
	else
	{
		char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, 72, msg);
	}

	M_Print (0, 80, "            Skill");
	if (skill.value == 0)
		M_Print (160, 80, "Easy difficulty");
	else if (skill.value == 1)
		M_Print (160, 80, "Normal difficulty");
	else if (skill.value == 2)
		M_Print (160, 80, "Hard difficulty");
	else
		M_Print (160, 80, "Nightmare difficulty");

	M_Print (0, 88, "       Frag Limit");
	if (fraglimit.value == 0)
		M_Print (160, 88, "none");
	else
		M_Print (160, 88, va("%i frags", (int)fraglimit.value));

	M_Print (0, 96, "       Time Limit");
	if (timelimit.value == 0)
		M_Print (160, 96, "none");
	else
		M_Print (160, 96, va("%i minutes", (int)timelimit.value));

	M_Print (0, 112, "         Episode");
   //MED 01/06/97 added hipnotic episodes
   if (hipnotic)
      M_Print (160, 112, hipnoticepisodes[startepisode].description);
   //PGM 01/07/97 added rogue episodes
   else if (rogue)
      M_Print (160, 112, rogueepisodes[startepisode].description);
   else
   {
		if(user_maps)
		  M_Print (160, 112, "User Maps");
		else
		  M_Print (160, 112, episodes[startepisode].description);
   }

	M_Print (0, 120, "           Level");
   //MED 01/06/97 added hipnotic episodes
   if (hipnotic)
   {
      M_Print (160, 120, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
      M_Print (160, 128, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
   }
   //PGM 01/07/97 added rogue episodes
   else if (rogue)
   {
      M_Print (160, 120, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
      M_Print (160, 128, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
   }
   else
   {
		if(user_maps)
		{
			M_Print (160, 120, user_levels[startlevel]);

		}
		else
		{
			M_Print (160, 120, levels[episodes[startepisode].firstLevel + startlevel].description);
            M_Print (160, 128, levels[episodes[startepisode].firstLevel + startlevel].name);
        }
	 }

// line cursor
	M_DrawCharacter (144, gameoptions_cursor_table[gameoptions_cursor], 12+((int)(realtime*4)&1));

	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print (x, 146, "  More than 4 players   ");
			M_Print (x, 154, " requires using command ");
			M_Print (x, 162, "line parameters; please ");
			M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}
}


void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue ("coop", coop.value ? 0 : 1);
		break;

	case 3:
		if (rogue)
			count = 6;
		else
			count = 2;

		Cvar_SetValue ("teamplay", teamplay.value + dir);
		if (teamplay.value > count)
			Cvar_SetValue ("teamplay", 0);
		else if (teamplay.value < 0)
			Cvar_SetValue ("teamplay", count);
		break;

	case 4:
		Cvar_SetValue ("skill", skill.value + dir);
		if (skill.value > 3)
			Cvar_SetValue ("skill", 0);
		if (skill.value < 0)
			Cvar_SetValue ("skill", 3);
		break;

	case 5:
		Cvar_SetValue ("fraglimit", fraglimit.value + dir*10);
		if (fraglimit.value > 100)
			Cvar_SetValue ("fraglimit", 0);
		if (fraglimit.value < 0)
			Cvar_SetValue ("fraglimit", 100);
		break;

	case 6:
		Cvar_SetValue ("timelimit", timelimit.value + dir*5);
		if (timelimit.value > 60)
			Cvar_SetValue ("timelimit", 0);
		if (timelimit.value < 0)
			Cvar_SetValue ("timelimit", 60);
		break;

	case 7:
		startepisode += dir;
//MED 01/06/97 added hipnotic count
		if (hipnotic)
		{
			count = 6;
//PGM 01/07/97 added rogue count
//PGM 03/02/97 added 1 for dmatch episode
		}
		else if (rogue)
		{
			count = 4;
        }
		else if (registered.value)
		{
			count = 7;
		}
		else if (user_maps)
		{
		    count = 1;
		}
		else
		{
			count = 2;
        }
		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 8:
		startlevel += dir;
    //MED 01/06/97 added hipnotic episodes
		if (hipnotic)
			count = hipnoticepisodes[startepisode].levels;
	//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
			count = rogueepisodes[startepisode].levels;
		else
		{
			if(user_maps)
			   count = user_maps_num;
			else
			   count = episodes[startepisode].levels;
        }
		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			SCR_BeginLoadingPlaque ();

			if (hipnotic)
				Cbuf_AddText ( va ("map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name) );
			else if (rogue)
				Cbuf_AddText ( va ("map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name) );
			else
			{
				if(user_maps)
				  Cbuf_AddText ( va ("map %s\n", user_levels[startlevel]));
                else
			      Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );
            }
			return;
		}

		M_NetStart_Change (1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	qpic_t	*p;
	int x;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12, 1);
	M_Print (x, 40, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU 

int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64];
	qpic_t	*p;

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Q_memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Q_memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Q_memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
			sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf(string, "%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		M_Print (16, 32 + 8*n, string);
	}
	M_DrawCharacter (0, 32 + slist_cursor*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", hostcache[slist_cursor].cname) );
		break;

	default:
		break;
	}

}
*/
//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_maps", M_Menu_Maps_f); //Crow_bar. maplist
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
    Cmd_AddCommand ("menu_slist", M_Menu_ServerList_f);
	//Cmd_AddCommand ("menu_video", M_Menu_Video_f); //FIX
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	
	if(user_maps)
	  Map_Finder();
}


void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}
		else
			Draw_FadeScreen ();

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;
		
	case m_maps: //Crow_bar. maplist
		M_Maps_Draw ();
		break;
		
	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_serialconfig:
		M_SerialConfig_Draw ();
		break;

	case m_modemconfig:
		M_ModemConfig_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;

	case m_sedit:
		M_SEdit_Draw ();
		break;

	case m_osk:
		M_OSK_Draw();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;
		
	case m_maps: //Crow_bar. maplist
		M_Maps_Key (key);
		return;
		
	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_serialconfig:
		M_SerialConfig_Key (key);
		return;

	case m_modemconfig:
		M_ModemConfig_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;
	
			
	case m_slist:
		M_ServerList_Key (key);
		return;

	case m_sedit:
		M_SEdit_Key (key);
		break;

	case m_osk:
		M_OSK_Key(key);	
	}
}


void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText ("stopdemo\n");
	if (SerialConfig || DirectConfig)
	{
		Cbuf_AddText ("com1 enable\n");
	}

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}

