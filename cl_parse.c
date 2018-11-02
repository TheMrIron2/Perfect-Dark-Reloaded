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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"

extern 	cvar_t	bgmtype;

char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value
	
	"svc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from
	
	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",
	
	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",			// [string] music [string] text
	"svc_cdtrack",			// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_cutscene",
	"svc_showlmp",		// [string] iconlabel [string] lmpfile [byte] x [byte] y
	"svc_hidelmp",		// [string] iconlabel
	"svc_skybox",		   // [string] skyname
	"",
	"",
	"",
    "svc_fog",    // 41		// [byte] start [byte] end [byte] red [byte] green [byte] blue [float] time
    "svc_bspdecal" //42     // [string] name [byte] decal_size [coords] pos
};

//=============================================================================
#define SHOWLMP_MAXLABELS	256
typedef struct showlmp_s
{
	qboolean	isactive;
	float		x, y;
	char		label[32];
	char		pic[128];
} showlmp_t;

showlmp_t	showlmp[SHOWLMP_MAXLABELS];

void SHOWLMP_decodehide (void)
{
	int	i;
	byte	*lmplabel;

	lmplabel = MSG_ReadString ();
	for (i=0 ; i<SHOWLMP_MAXLABELS ; i++)
	{
		if (showlmp[i].isactive && !strcmp(showlmp[i].label, lmplabel))
		{
			showlmp[i].isactive = false;
			return;
		}
	}
}

void SHOWLMP_decodeshow (void)
{
	int	i, k;
	byte	lmplabel[256], picname[256];
	float	x, y;

	strcpy (lmplabel, MSG_ReadString());
	strcpy (picname, MSG_ReadString());
	x = MSG_ReadByte ();
	y = MSG_ReadByte ();
	k = -1;
	for (i=0 ; i<SHOWLMP_MAXLABELS ; i++)
	{
		if (showlmp[i].isactive)
		{
			if (!strcmp(showlmp[i].label, lmplabel))
			{
				k = i;
				break;	// drop out to replace it
			}
		}
		else if (k < 0)	// find first empty one to replace
		{
			k = i;
		}
	}

	if (k < 0)
		return;	// none found to replace
	// change existing one
	showlmp[k].isactive = true;
	strcpy (showlmp[k].label, lmplabel);
	strcpy (showlmp[k].pic, picname);
	showlmp[k].x = x;
	showlmp[k].y = y;
}

void SHOWLMP_drawall (void)
{
	int	i;

	for (i=0 ; i<SHOWLMP_MAXLABELS ; i++)
		if (showlmp[i].isactive)
			Draw_TransPic (showlmp[i].x, showlmp[i].y, Draw_CachePic(showlmp[i].pic));
}

void SHOWLMP_clear (void)
{
	int	i;

	for (i=0 ; i<SHOWLMP_MAXLABELS ; i++)
		showlmp[i].isactive = false;
}
/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
entity_t	*CL_EntityNum (int num)
{
	if (num >= cl.num_entities)
	{
		if (num >= MAX_EDICTS)
			Host_Error ("CL_EntityNum: %i is an invalid number",num);
		while (cl.num_entities<=num)
		{
			cl_entities[cl.num_entities].colormap = vid.colormap;
			cl.num_entities++;
		}
	}
		
	return &cl_entities[num];
}


/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;  
 	int		i;
	           
    field_mask = MSG_ReadByte(); 

    if (field_mask & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (field_mask & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	channel = MSG_ReadShort ();
	sound_num = MSG_ReadByte ();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);
	
	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();
 
    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);
}

/*
==================
CL_ParseBSPDecal

Spawn decals on map
Crow_bar.
================== 
*/
void CL_ParseBSPDecal (void)
{
	vec3_t		pos;
	int			decal_size;
	char        *texname;

    texname     = MSG_ReadString ();
    decal_size  = MSG_ReadByte   ();
	pos[0]      = MSG_ReadCoord  ();
    pos[1]      = MSG_ReadCoord  ();
    pos[2]      = MSG_ReadCoord  ();

	if(!texname)
		return;

	Con_Printf("BSPDECAL[tex: %s size: %i pos: %f %f %f]\n", texname, decal_size, pos[0], pos[1], pos[2]);

	R_SpawnDecalBSP(pos, texname, decal_size);
}

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage (void)
{
	double	time;
	static float lastmsg;
	int		ret;
	sizebuf_t	old;
	byte		olddata[8192];
	
	if (sv.active)
		return;		// no need if server is local
	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	old = net_message;
	memcpy (olddata, net_message.data, net_message.cursize);
	
	do
	{
		ret = CL_GetMessage ();
		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");		
		case 0:
			break;	// nothing waiting
		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (MSG_ReadByte() != svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");
			break;
		}
	} while (ret);

	net_message = old;
	memcpy (net_message.data, olddata, net_message.cursize);

// check time
	time = Sys_FloatTime ();
	if (time - lastmsg < 5)
		return;
	lastmsg = time;

// write out a nop
	Con_Printf ("--> client to server keepalive\n");

	MSG_WriteByte (&cls.message, clc_nop);
	NET_SendMessage (cls.netcon, &cls.message);
	SZ_Clear (&cls.message);
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	char	*str, tempname[MAX_QPATH];;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_MODELS][MAX_QPATH];
	char	sound_precache[MAX_SOUNDS][MAX_QPATH];
	extern qboolean r_loadq3player;

	//void R_PreMapLoad (char *);

	char	mapname[MAX_QPATH];
	
	Con_DPrintf ("Serverinfo packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	i = MSG_ReadLong ();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf ("Server returned version %i, not %i", i, PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.maxclients = MSG_ReadByte ();
	if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}
	cl.scores = Hunk_AllocName (cl.maxclients*sizeof(*cl.scores), "scores");

// parse gametype
	cl.gametype = MSG_ReadByte ();

// parse signon message
	str = MSG_ReadString ();
	strncpy (cl.levelname, str, sizeof(cl.levelname)-1);

// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

//
// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it
//

// precache models
	for (i=0 ; i<NUM_MODELINDEX ; i++)
		cl_modelindex[i] = -1;

// precache models
	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	for (nummodels=1 ; ; nummodels++)
	{
		str = MSG_ReadString ();

		if (!str[0])
			break;

		if (nummodels==MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches\n");
			return;
     	}

		if (r_loadq3models.value)
		{
			if (!strcmp(str, "progs/player.mdl") &&
			    FS_FindFile("progs/player/head.md3") &&
			    FS_FindFile("progs/player/upper.md3") &&
			    FS_FindFile("progs/player/lower.md3"))
			{
				Q_strncpyz (tempname, "progs/player/lower.md3", MAX_QPATH);
				str = tempname;
				cl_modelindex[mi_player] = nummodels;
				r_loadq3player = true;
			}
			else
			{
				COM_StripExtension (str, tempname);
				strcat (tempname, ".md3");

				if (FS_FindFile(tempname))
					str = tempname;
			}
		}

        Q_strncpyz (model_precache[nummodels], str, sizeof(model_precache[nummodels]));

		Mod_TouchModel (str);

		if (!strcmp(model_precache[nummodels], "progs/player.mdl"))
			cl_modelindex[mi_player] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/eyes.mdl"))
			cl_modelindex[mi_eyes] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/missile.mdl"))
			cl_modelindex[mi_rocket] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/grenade.mdl"))
			cl_modelindex[mi_grenade] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/flame.mdl"))
			cl_modelindex[mi_flame1] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/flame2.mdl"))
			cl_modelindex[mi_flame2] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/s_expl.spr"))
			cl_modelindex[mi_explo1] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/s_explod.spr"))
			cl_modelindex[mi_explo2] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/s_bubble.spr"))
			cl_modelindex[mi_bubble] = nummodels;

		else if (!strcmp(model_precache[nummodels], "progs/gib1.mdl"))
			cl_modelindex[mi_gib1] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/gib2.mdl"))
			cl_modelindex[mi_gib2] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/gib3.mdl"))
			cl_modelindex[mi_gib3] = nummodels;

		else if (!strcmp(model_precache[nummodels], "progs/fish.mdl"))
			cl_modelindex[mi_fish] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/dog.mdl"))
			cl_modelindex[mi_dog] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/soldier.mdl"))
			cl_modelindex[mi_soldier] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/enforcer.mdl"))
			cl_modelindex[mi_enforcer] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/knight.mdl"))
			cl_modelindex[mi_knight] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/hknight.mdl"))
			cl_modelindex[mi_hknight] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/wizard.mdl"))
			cl_modelindex[mi_scrag] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/ogre.mdl"))
			cl_modelindex[mi_ogre] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/demon.mdl"))
			cl_modelindex[mi_fiend] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/shalrath.mdl"))
			cl_modelindex[mi_vore] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/shambler.mdl"))
			cl_modelindex[mi_shambler] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_dog.mdl"))
			cl_modelindex[mi_h_dog] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_guard.mdl"))
			cl_modelindex[mi_h_soldier] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_mega.mdl"))
			cl_modelindex[mi_h_enforcer] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_knight.mdl"))
			cl_modelindex[mi_h_knight] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_hknight.mdl"))
			cl_modelindex[mi_h_hknight] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_wizard.mdl"))
			cl_modelindex[mi_h_scrag] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_ogre.mdl"))
			cl_modelindex[mi_h_ogre] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_demon.mdl"))
			cl_modelindex[mi_h_fiend] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_shal.mdl"))
			cl_modelindex[mi_h_vore] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_shams.mdl"))
			cl_modelindex[mi_h_shambler] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_zombie.mdl"))
			cl_modelindex[mi_h_zombie] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/h_player.mdl"))
			cl_modelindex[mi_h_player] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/w_s_key.mdl"))
			cl_modelindex[mi_w_s_key] = nummodels;
		else if (!strcmp(model_precache[nummodels], "progs/w_g_key.mdl"))
			cl_modelindex[mi_w_g_key] = nummodels;
  }

    if (r_loadq3player)
	{
		if (nummodels + 1 >= MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches -> can't load Q3 player model\n");
			Q_strncpyz (model_precache[cl_modelindex[mi_player]], cl_modelnames[mi_player], sizeof(model_precache[cl_modelindex[mi_player]]));
		}
		else
		{
			Q_strncpyz (model_precache[nummodels], "progs/player/upper.md3", sizeof(model_precache[nummodels]));
			cl_modelindex[mi_q3torso] = nummodels++;
			Q_strncpyz (model_precache[nummodels], "progs/player/head.md3", sizeof(model_precache[nummodels]));
			cl_modelindex[mi_q3head] = nummodels++;
		}
	}

// precache sounds
	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
		{
			Con_Printf ("Server sent too many sound precaches\n");
			return;
		}
		strcpy (sound_precache[numsounds], str);
		S_TouchSound (str);
	}

    //COM_StripExtension (COM_SkipPath(model_precache[1]), mapname);
	//R_PreMapLoad (mapname);

//
// now we try to load everything else until a cache allocation fails
//
      
    loading_num_step[loading_level] = nummodels + numsounds;
    sprintf(loading_name[loading_level], "Models and Sounds");

	for (i=1 ; i<nummodels ; i++)
	{
		cl.model_precache[i] = Mod_ForName (model_precache[i], false);
		if (cl.model_precache[i] == NULL)
		{
			Con_Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage ();
	    loading_cur_step[loading_level]++;
	    SCR_UpdateScreen ();
	}

	// load the extra "no-flamed-torch" model
	cl.model_precache[nummodels] = Mod_ForName ("progs/flame0.mdl", false);
	cl_modelindex[mi_flame0] = nummodels++;

	cl.model_precache[nummodels] = Mod_ForName ("progs/flag.mdl", false);
    cl_modelindex[mi_flag] = nummodels++;

	S_BeginPrecaching ();
	for (i=1 ; i<numsounds ; i++)
	{
		cl.sound_precache[i] = S_PrecacheSound (sound_precache[i]);
		CL_KeepaliveMessage ();
	    loading_cur_step[loading_level]++;
	    SCR_UpdateScreen ();
    }
	S_EndPrecaching ();

   	Clear_LoadingFill ();

// local state
	cl_entities[0].model = cl.worldmodel = cl.model_precache[1];
	
	R_NewMap ();

	Hunk_Check ();		// make sure nothing is hurt
	
	noclip_anglehack = false;		// noclip is turned off at start	
}


/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/
int	bitcounts[16];

void CL_ParseUpdate (int bits)
{
	int			i;
	model_t		*model;
	int			modnum;
	qboolean	forcelink;
	entity_t	*ent;
	int			num;
    //int		    skin;
    
	if (cls.signon == SIGNONS - 1)
	{	// first update is the final signon stage
		cls.signon = SIGNONS;
		CL_SignonReply ();
	}

	if (bits & U_MOREBITS)
	{
		i = MSG_ReadByte ();
		bits |= (i<<8);
	}

	// Tomaz - QC Control Begin
    if (bits & U_EXTEND1)
    {
	    bits |= MSG_ReadByte() << 16;

		if (bits & U_EXTEND2)
		{
		  bits |= MSG_ReadByte() << 24;
	    }
	}
	// Tomaz - QC Control End


	if (bits & U_LONGENTITY)	
		num = MSG_ReadShort ();
	else
		num = MSG_ReadByte ();

	ent = CL_EntityNum (num);

for (i=0 ; i<16 ; i++)
if (bits&(1<<i))
	bitcounts[i]++;

	if (ent->msgtime != cl.mtime[1])
		forcelink = true;	// no previous frame to lerp from
	else
		forcelink = false;

	ent->msgtime = cl.mtime[0];
	
	if (bits & U_MODEL)
	{
		modnum = MSG_ReadByte ();
		if (modnum >= MAX_MODELS)
			Host_Error ("CL_ParseModel: bad modnum");
	}
	else
		modnum = ent->baseline.modelindex;
		
	model = cl.model_precache[modnum];
	if (model != ent->model)
	{
		ent->model = model;
	// automatic animation (torches, etc) can be either all together
	// or randomized
		if (model)
		{
			if (model->synctype == ST_RAND)
				ent->syncbase = (float)(rand()&0x7fff) / 0x7fff;
			else
				ent->syncbase = 0.0;
		}
		else
			forcelink = true;	// hack to make null model players work
#if defined(GLQUAKE) //|| defined(PSP_HARDWARE_VIDEO)
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
#endif
	}
	
	if (bits & U_FRAME)
		ent->frame = MSG_ReadByte ();
	else
		ent->frame = ent->baseline.frame;
	if (bits & U_FRAME2)
		ent->frame = (ent->frame & 0x00FF) | (MSG_ReadByte() << 8);	

	if (bits & U_COLORMAP)
		i = MSG_ReadByte();
	else
		i = ent->baseline.colormap;
	if (!i)
		ent->colormap = vid.colormap;
	else
	{
		if (i > cl.maxclients)
			Sys_Error ("i >= cl.maxclients");
		ent->colormap = cl.scores[i-1].translations;
	}

#if defined(GLQUAKE) //|| defined(PSP_HARDWARE_VIDEO)
	if (bits & U_SKIN)
		skin = MSG_ReadByte();
	else
		skin = ent->baseline.skin;
	if (skin != ent->skinnum) {
		ent->skinnum = skin;
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
	}

#else

	if (bits & U_SKIN)
		ent->skinnum = MSG_ReadByte();
	else
		ent->skinnum = ent->baseline.skin;
#endif

	if (bits & U_EFFECTS)
		ent->effects = MSG_ReadByte();
	else
		ent->effects = ent->baseline.effects;

// shift the known values for interpolation
	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & U_ORIGIN1)
		ent->msg_origins[0][0] = MSG_ReadCoord ();
	else
		ent->msg_origins[0][0] = ent->baseline.origin[0];
	if (bits & U_ANGLE1)
		ent->msg_angles[0][0] = MSG_ReadAngle();
	else
		ent->msg_angles[0][0] = ent->baseline.angles[0];

	if (bits & U_ORIGIN2)
		ent->msg_origins[0][1] = MSG_ReadCoord ();
	else
		ent->msg_origins[0][1] = ent->baseline.origin[1];
	if (bits & U_ANGLE2)
		ent->msg_angles[0][1] = MSG_ReadAngle();
	else
		ent->msg_angles[0][1] = ent->baseline.angles[1];

	if (bits & U_ORIGIN3)
		ent->msg_origins[0][2] = MSG_ReadCoord ();
	else
		ent->msg_origins[0][2] = ent->baseline.origin[2];
	if (bits & U_ANGLE3)
		ent->msg_angles[0][2] = MSG_ReadAngle();
	else
		ent->msg_angles[0][2] = ent->baseline.angles[2];
#ifdef HL_RENDER
// Tomaz - QC Alpha Scale Glow Begin
	if (bits & U_RENDERAMT)
	    ent->renderamt = MSG_ReadFloat();
    else
	    ent->renderamt = 0;

    if (bits & U_RENDERMODE)
	    ent->rendermode = MSG_ReadFloat();
    else
	    ent->rendermode = 0;

	if (bits & U_RENDERCOLOR1)
	    ent->rendercolor[0] = MSG_ReadFloat();
    else
	    ent->rendercolor[0] = 0;

	if (bits & U_RENDERCOLOR2)
	    ent->rendercolor[1] = MSG_ReadFloat();
    else
	    ent->rendercolor[1] = 0;
	    
	if (bits & U_RENDERCOLOR3)
	    ent->rendercolor[2] = MSG_ReadFloat();
    else
	    ent->rendercolor[2] = 0;
#endif
// Tomaz - QC Alpha Scale Glow End
    if ( bits & U_NOLERP )
		ent->forcelink = true;

	if ( forcelink )
	{	// didn't have an update last message
		VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy (ent->msg_origins[0], ent->origin);
		VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy (ent->msg_angles[0], ent->angles);
		ent->forcelink = true;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_t *ent)
{
	int			i;
	
	ent->baseline.modelindex = MSG_ReadByte ();
	ent->baseline.frame = MSG_ReadByte ();
	ent->baseline.colormap = MSG_ReadByte();
	ent->baseline.skin = MSG_ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		ent->baseline.origin[i] = MSG_ReadCoord ();
		ent->baseline.angles[i] = MSG_ReadAngle ();
	}
}

/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata (int bits)
{
	int		i, j;
	
	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = MSG_ReadChar ();
	else
		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = MSG_ReadChar ();
	else
		cl.idealpitch = 0;
	
	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			cl.punchangle[i] = MSG_ReadChar();
		else
			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1<<i) )
			cl.mvelocity[0][i] = MSG_ReadChar()*16;
		else
			cl.mvelocity[0][i] = 0;
	}

// [always sent]	if (bits & SU_ITEMS)
		i = MSG_ReadLong ();

	if (cl.items != i)
	{	// set flash times
		Sbar_Changed ();
		for (j=0 ; j<32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.time;
		cl.items = i;
	}
		
	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		cl.stats[STAT_WEAPONFRAME] = MSG_ReadByte ();
	else
		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
		i = MSG_ReadByte ();
	else
		i = 0;
	if (cl.stats[STAT_ARMOR] != i)
	{
		cl.stats[STAT_ARMOR] = i;
		Sbar_Changed ();
	}

	if (bits & SU_WEAPON)
		i = MSG_ReadByte ();
	else
		i = 0;
	if (cl.stats[STAT_WEAPON] != i)
	{
		cl.stats[STAT_WEAPON] = i;
		Sbar_Changed ();
	}
	
	i = MSG_ReadShort ();
	if (cl.stats[STAT_HEALTH] != i)
	{
		cl.stats[STAT_HEALTH] = i;
		Sbar_Changed ();
	}

	i = MSG_ReadByte ();
	if (cl.stats[STAT_AMMO] != i)
	{
		cl.stats[STAT_AMMO] = i;
		Sbar_Changed ();
	}

	for (i=0 ; i<4 ; i++)
	{
		j = MSG_ReadByte ();
		if (cl.stats[STAT_SHELLS+i] != j)
		{
			cl.stats[STAT_SHELLS+i] = j;
			Sbar_Changed ();
		}
	}

	i = MSG_ReadByte ();

	if (standard_quake)
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != i)
		{
			cl.stats[STAT_ACTIVEWEAPON] = i;
			Sbar_Changed ();
		}
	}
	else
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != (1<<i))
		{
			cl.stats[STAT_ACTIVEWEAPON] = (1<<i);
			Sbar_Changed ();
		}
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	int		i, j;
	int		top, bottom;
	byte	*dest, *source;
	
	if (slot > cl.maxclients)
		Sys_Error ("CL_NewTranslation: slot > cl.maxclients");
	dest = cl.scores[slot].translations;
	source = vid.colormap;
	memcpy (dest, vid.colormap, sizeof(cl.scores[slot].translations));
	top = cl.scores[slot].colors & 0xf0;
	bottom = (cl.scores[slot].colors &15)<<4;
	
#if defined(GLQUAKE) || defined(PSP_HARDWARE_VIDEO)
	R_TranslatePlayerSkin (slot);
#endif

	for (i=0 ; i<VID_GRADES ; i++, dest += 256, source+=256)
	{
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
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;
	int		i;
		
	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_Error ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;
	CL_ParseBaseline (ent);

// copy it to the current state
	ent->model = cl.model_precache[ent->baseline.modelindex];
	ent->frame = ent->baseline.frame;
	ent->colormap = vid.colormap;
	ent->skinnum = ent->baseline.skin;
	ent->effects = ent->baseline.effects;

	VectorCopy (ent->baseline.origin, ent->origin);
	VectorCopy (ent->baseline.angles, ent->angles);	
	R_AddEfrags (ent);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();
	
	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}


#define SHOWNET(x) if(cl_shownet.value==2)Con_Printf ("%3i:%s\n", msg_readcount-1, x);

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	int			i;
	
//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_Printf ("------------------\n");
	
	cl.onground = false;	// unless the server says otherwise	
//
// parse the message
//
	MSG_BeginReading ();
	
	while (1)
	{
		if (msg_badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			SHOWNET("fast update");
			CL_ParseUpdate (cmd&127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);
	
	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message\n");
			break;
			
		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;
			
		case svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = MSG_ReadFloat ();			
			break;
			
		case svc_clientdata:
			i = MSG_ReadShort ();
			CL_ParseClientdata (i);
			break;
		
		case svc_version:
			i = MSG_ReadLong ();
			if (i != PROTOCOL_VERSION)
				Host_Error ("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;
			
		case svc_disconnect:
			Host_EndGame ("Server disconnected\n");

		case svc_print:
			Con_Printf ("%s", MSG_ReadString ());
			break;
			
		case svc_centerprint:
			SCR_CenterPrint (MSG_ReadString ());
			break;
			
		case svc_stufftext:
			Cbuf_AddText (MSG_ReadString ());
			break;
			
		case svc_damage:
			V_ParseDamage ();
			break;
			
		case svc_serverinfo:
			CL_ParseServerInfo ();
			vid.recalc_refdef = true;	// leave intermission full screen
			break;
			
		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = MSG_ReadAngle ();
			break;
			
		case svc_setview:
			cl.viewentity = MSG_ReadShort ();
			break;
					
		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
			Q_strcpy (cl_lightstyle[i].map,  MSG_ReadString());
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
			break;
			
		case svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;
		
		case svc_updatename:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
			strcpy (cl.scores[i].name, MSG_ReadString ());
			break;
			
		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.scores[i].frags = MSG_ReadShort ();
			break;			

		case svc_updatecolors:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
			cl.scores[i].colors = MSG_ReadByte ();
			//CL_NewTranslation (i);
			break;
			
		case svc_particle:
			R_ParseParticleEffect ();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline (CL_EntityNum(i));
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;			
		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_setpause:
			{
				cl.paused = MSG_ReadByte ();

				if (cl.paused)
				{
					CDAudio_Pause ();
#ifdef WIN32
					VID_HandlePause (true);
#endif
				}
				else
				{
					CDAudio_Resume ();
#ifdef WIN32
					VID_HandlePause (false);
#endif
				}
			}
			break;
			
		case svc_signonnum:
			i = MSG_ReadByte ();
			if (i <= cls.signon)
				Host_Error ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			if (i < 0 || i >= MAX_CL_STATS)
				Sys_Error ("svc_updatestat: %i is invalid", i);
			cl.stats[i] = MSG_ReadLong ();;
			break;
			
		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			cl.looptrack = MSG_ReadByte ();

			if (strcmpi(bgmtype.string,"cd") == 0)
			{
				if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
					CDAudio_Play ((byte)cls.forcetrack, true);
				else
					CDAudio_Play ((byte)cl.cdtrack, true);
			}
			else
			   CDAudio_Stop();
			break;

		case svc_intermission:
			cl.intermission = 1;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			break;

		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (MSG_ReadString ());			
			break;

		case svc_cutscene:
			cl.intermission = 3;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (MSG_ReadString ());			
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;

	    case svc_skybox:
			Sky_LoadSkyBox(MSG_ReadString());
			break;

		case svc_fog:
			Fog_ParseServerMessage ();
			break;

        case svc_bspdecal:
			CL_ParseBSPDecal ();
			break;
		case svc_hidelmp:
			SHOWLMP_decodehide ();
			break;

		case svc_showlmp:
			SHOWLMP_decodeshow ();
			break;	
		}
	}
}

