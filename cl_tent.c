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
// cl_tent.c -- client side temporary entities

#include "quakedef.h"

static	vec3_t	playerbeam_end;		// added by joe

int			num_temp_entities;
entity_t	cl_temp_entities[MAX_TEMP_ENTITIES];
beam_t		cl_beams[MAX_BEAMS];

model_t		*cl_q3gunshot_mod, *cl_q3teleport_mod;

sfx_t			*cl_sfx_wizhit;
sfx_t		    *cl_sfx_thunder;
sfx_t			*cl_sfx_knighthit;
sfx_t			*cl_sfx_tink1;
//normal rics
sfx_t			*cl_sfx_ric1;
sfx_t			*cl_sfx_ric2;
sfx_t			*cl_sfx_ric3;
// wood rics
sfx_t                   *cl_sfx_wric1;
sfx_t                   *cl_sfx_wric2;
sfx_t                   *cl_sfx_wric3;
// metall rics
sfx_t                   *cl_sfx_mric1;
sfx_t                   *cl_sfx_mric2;
sfx_t                   *cl_sfx_mric3;
sfx_t                   *cl_sfx_r_exp3;
sfx_t			*cl_sfx_r_exp3;
#ifdef QUAKE2
sfx_t			*cl_sfx_imp;
sfx_t			*cl_sfx_rail;
#endif
	
	//detecting texture code,by Baker,adapted by Ghost_Fang,fixed by st1x
    msurface_t   *new_collision_wall;       //Baker's, idk
    msurface_t  *previous_collision_wall;   //Also Bakers, still dunno.
     
    char* texturehit = "NULL";              //Ghost Fang. Set a global "texturehit" that will store a texture string
                                            //Set it NULL to avoid crashing
     
    int RecursiveWallPoint (mnode_t *node, vec3_t start, vec3_t end)
    {
       float   front, back, frac;
       vec3_t   mid;
     
    loc0:
       if (node->contents < 0)
          return false;      // didn't hit anything
       
    // calculate mid point
       if (node->plane->type < 3)
       {
          front = start[node->plane->type] - node->plane->dist;
          back = end[node->plane->type] - node->plane->dist;
       }
       else
       {
          front = DotProduct(start, node->plane->normal) - node->plane->dist;
          back = DotProduct(end, node->plane->normal) - node->plane->dist;
       }
     
       // optimized recursion
       if ((back < 0) == (front < 0))
       {
          node = node->children[front < 0];
          goto loc0;
       }
       
       frac = front / (front-back);
       mid[0] = start[0] + (end[0] - start[0]) * frac;
       mid[1] = start[1] + (end[1] - start[1]) * frac;
       mid[2] = start[2] + (end[2] - start[2]) * frac;
       
    // go down front side
       if (RecursiveWallPoint(node->children[front < 0], start, mid))
       {
          return true;   // hit something
       }
       else
       {
          int      i, ds, dt;
          msurface_t   *surf;
     
       // check for impact on this node
          //VectorCopy (mid, lightspot);
     
          surf = cl.worldmodel->surfaces + node->firstsurface;
          for (i = 0 ; i < node->numsurfaces ; i++, surf++)
          {
     
             ds = (int)((float)DotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
             dt = (int)((float)DotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);
     
             if (ds < surf->texturemins[0] || dt < surf->texturemins[1])
                continue;
             
             ds -= surf->texturemins[0];
             dt -= surf->texturemins[1];
             
             if (ds > surf->extents[0] || dt > surf->extents[1])
                continue;
     
             // At this point we have a collision with this surface
             new_collision_wall = surf;
     
             return true;   // success
          }
     
       // go down back side
          return RecursiveWallPoint (node->children[front >= 0], mid, end);
       }
    }
     
    int R_SurfacePoint ()
    {  
       vec3_t      player;
       vec3_t      forward, up, right;
       vec3_t      destination;
       int         texturenum;
     
       qboolean      changed=false;
       
       VectorCopy (r_refdef.vieworg, player);            // Player eye position
       AngleVectors (cl.viewangles, forward, right, up);   // Take into account the angles
       VectorMA (player, 4096, forward, destination);            // Walk us forward
       
       new_collision_wall = NULL;   // Set to null
     
       texturenum = RecursiveWallPoint (cl.worldmodel->nodes, player, destination);
      
       // If there is a collision wall AND either there is no previous or the previous doesn't match, we have a change.
       changed = (new_collision_wall && (!previous_collision_wall || (previous_collision_wall != new_collision_wall)));
     
       if (previous_collision_wall && changed)
       {
  		  previous_collision_wall = NULL;
       }
       
       if (new_collision_wall && changed)
       {
          previous_collision_wall = new_collision_wall;
          texturehit = previous_collision_wall->texinfo->texture->name;
       }
     
       return texturenum;
    }
/*
=================
CL_ParseTEnt
=================
*/
void CL_InitTEnts (void)
{
	cl_sfx_wizhit = S_PrecacheSound ("wizard/hit.wav");
	cl_sfx_knighthit = S_PrecacheSound ("hknight/hit.wav");
	cl_sfx_tink1 = S_PrecacheSound ("weapons/tink1.wav");
	cl_sfx_r_exp3 = S_PrecacheSound ("weapons/r_exp3.wav");
    cl_sfx_thunder	= S_PrecacheSound ("ambience/thunder1.wav");
	//normal rics
	cl_sfx_ric1 = S_PrecacheSound ("weapons/rics/ric1.wav");
	cl_sfx_ric2 = S_PrecacheSound ("weapons/rics/ric2.wav");
	cl_sfx_ric3 = S_PrecacheSound ("weapons/rics/ric3.wav");
    // wood rics
    cl_sfx_wric1 = S_PrecacheSound ("weapons/rics/wood_ric1.wav");
    cl_sfx_wric2 = S_PrecacheSound ("weapons/rics/wood_ric2.wav");
    cl_sfx_wric3 = S_PrecacheSound ("weapons/rics/wood_ric3.wav");
    // metall rics
    cl_sfx_mric1 = S_PrecacheSound ("weapons/rics/metal_ric1.wav");
    cl_sfx_mric2 = S_PrecacheSound ("weapons/rics/metal_ric2.wav");
    cl_sfx_mric3 = S_PrecacheSound ("weapons/rics/metal_ric3.wav");	
#ifdef QUAKE2
	cl_sfx_imp = S_PrecacheSound ("shambler/sattck1.wav");
	cl_sfx_rail = S_PrecacheSound ("weapons/lstart.wav");
#endif
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	//cl_bolt1_mod = cl_bolt2_mod = cl_bolt3_mod = cl_beam_mod = NULL;
	cl_q3gunshot_mod = cl_q3teleport_mod = NULL;

	memset (&cl_beams, 0, sizeof(cl_beams));
}

/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam (model_t *m)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	ent = MSG_ReadShort ();
	
	start[0] = MSG_ReadCoord ();
	start[1] = MSG_ReadCoord ();
	start[2] = MSG_ReadCoord ();
	
	end[0] = MSG_ReadCoord ();
	end[1] = MSG_ReadCoord ();
	end[2] = MSG_ReadCoord ();

    if (ent == cl.viewentity)
		VectorCopy(end, playerbeam_end);	// for cl_truelightning

// override any beam with the same entity
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}
	}
	Con_Printf ("beam list overflow!\n");	
}

enum
{
	DECAL_NONE,
	DECAL_BULLTMRK,
	DECAL_BLOODMRK1,
	DECAL_BLOODMRK2,
	DECAL_BLOODMRK3,
	DECAL_EXPLOMRK
};
//==============================================================================
extern cvar_t	r_decal_bullets;
extern cvar_t   r_decal_explosions;

extern int decal_blood1, decal_blood2, decal_blood3, decal_burn, decal_mark, decal_glow;
//==============================================================================
/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos;
#ifdef QUAKE2
	vec3_t	endpos;
#endif
	dlight_t	*dl;
	int		rnd;
	int		colorStart, colorLength;

	type = MSG_ReadByte ();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
        if (r_part_spikes.value == 2 && !cl_q3gunshot_mod)
			cl_q3gunshot_mod = Mod_ForName ("progs/bullet.md3", true);
		R_RunParticleEffect (pos, vec3_origin, 20, 30);
		S_StartSound (-1, 0, cl_sfx_wizhit, pos, 1, 1);
		break;
		
	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
        if (r_part_spikes.value == 2 && !cl_q3gunshot_mod)
			cl_q3gunshot_mod = Mod_ForName ("progs/bullet.md3", true);
		R_RunParticleEffect (pos, vec3_origin, 226, 20);
		S_StartSound (-1, 0, cl_sfx_knighthit, pos, 1, 1);
		break;
		
	case TE_SPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
        if (r_part_spikes.value == 2 && !cl_q3gunshot_mod)
			cl_q3gunshot_mod = Mod_ForName ("progs/bullet.md3", true);
		//R00k--start
		if (r_decal_bullets.value)
		{
			R_SpawnDecalStatic(pos, decal_mark, 8);
		}
		//R00k--end

#ifdef GLTEST
		Test_Spawn (pos);
#else
		R_RunParticleEffect (pos, vec3_origin, 0, 10);
#endif
		if ( rand() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
        if (r_part_spikes.value == 2 && !cl_q3gunshot_mod)
			cl_q3gunshot_mod = Mod_ForName ("progs/bullet.md3", true);
		R_RunParticleEffect (pos, vec3_origin, 0, 20);

		//R00k--start
		if (r_decal_bullets.value)
		{
			R_SpawnDecalStatic(pos, decal_mark, 10);
		}
		//R00k--end

		if ( rand() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
		
	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		R_SurfacePoint();
        if (r_part_gunshots.value == 2 && !cl_q3gunshot_mod)
			cl_q3gunshot_mod = Mod_ForName ("progs/bullet.md3", true);
                if(!Q_strncmp(texturehit,"sky",3) )                             //Don't draw decals or sparks on the skybox
        break;
		//R00k--start
		if (r_decal_bullets.value)
		{
			R_SpawnDecalStatic(pos, decal_mark, 8);
	}
		//R00k--end
                if(!Q_strncmp(texturehit,"metal",5) )
                {
                        rnd = rand() & 3;
                        if (rnd == 1)
                                S_StartSound (-1, 0, cl_sfx_mric1, pos, 1, 1);
                        else if (rnd == 2)
                                S_StartSound (-1, 0, cl_sfx_mric2, pos, 1, 1);
                        else
                                S_StartSound (-1, 0, cl_sfx_mric3, pos, 1, 1);
                } 
            else if(!Q_strncmp(texturehit,"wood",4) )
                {
                        rnd = rand() & 3;  
                        if (rnd == 1)
                                S_StartSound (-1, 0, cl_sfx_wric1, pos, 1, 1);
                        else if (rnd == 2)
                                S_StartSound (-1, 0, cl_sfx_wric2, pos, 1, 1);
                        else
                                S_StartSound (-1, 0, cl_sfx_wric3, pos, 1, 1);
                }
                else
                {
                        rnd = rand() & 3;
                        if (rnd == 1)
                                S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
                        else if (rnd == 2)
                                S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
                        else
                                S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
                }
		R_RunParticleEffect (pos, vec3_origin, 0, 20);	
	break;	
	case TE_EXPLOSION:			// rocket explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		R_ParticleExplosion (pos);
        if (r_decal_explosions.value)
		{
			R_SpawnDecalStatic(pos, decal_burn, 100);
		}
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;
		
	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		R_BlobExplosion (pos);
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt.mdl", true));
		break;
	
	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt2.mdl", true));
		break;
	
	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt3.mdl", true));
		break;
	
// PGM 01/21/97 
	case TE_BEAM:				// grappling hook beam
		CL_ParseBeam (Mod_ForName("progs/beam.mdl", true));
		break;
// PGM 01/21/97

	case TE_LAVASPLASH:	
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		R_LavaSplash (pos);
        dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 150;
		dl->die = cl.time + 0.75;
		dl->decay = 200;
		break;
	
	case TE_TELEPORT:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
        if (r_part_telesplash.value == 2 && !cl_q3teleport_mod)
			cl_q3teleport_mod = Mod_ForName ("progs/telep.md3", true);
		R_TeleportSplash (pos);
		break;
		
	case TE_EXPLOSION2:				// color mapped explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		colorStart = MSG_ReadByte ();
		colorLength = MSG_ReadByte ();
		R_ParticleExplosion2 (pos, colorStart, colorLength);

		if (r_decal_explosions.value)
		{
			R_SpawnDecalStatic(pos, decal_burn, 100);
		}

		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;
		
#ifdef QUAKE2
	case TE_IMPLOSION:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		S_StartSound (-1, 0, cl_sfx_imp, pos, 1, 1);
		break;

	case TE_RAILTRAIL:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		endpos[0] = MSG_ReadCoord ();
		endpos[1] = MSG_ReadCoord ();
		endpos[2] = MSG_ReadCoord ();
		S_StartSound (-1, 0, cl_sfx_rail, pos, 1, 1);
		S_StartSound (-1, 1, cl_sfx_r_exp3, endpos, 1, 1);
		R_RocketTrail (pos, endpos, 0+128);
		R_ParticleExplosion (endpos);
		dl = CL_AllocDlight (-1);
		VectorCopy (endpos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		break;
#endif

	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}


/*
=================
CL_NewTempEntity
=================
*/
entity_t *CL_NewTempEntity (void)
{
	entity_t	*ent;

	if (cl_numvisedicts == MAX_VISEDICTS)
		return NULL;
	if (num_temp_entities == MAX_TEMP_ENTITIES)
		return NULL;
	ent = &cl_temp_entities[num_temp_entities];
	memset (ent, 0, sizeof(*ent));
	num_temp_entities++;
	cl_visedicts[cl_numvisedicts] = ent;
	cl_numvisedicts++;

	ent->colormap = vid.colormap;
	return ent;
}

/*
=================
TraceLineN
=================
*/
qboolean TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal)
{
	trace_t	trace;

	memset (&trace, 0, sizeof(trace));
	if (!SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, start, end, &trace))
	{
		if (trace.fraction < 1)
		{
			VectorCopy (trace.endpos, impact);
			if (normal)
				VectorCopy (trace.plane.normal, normal);

			return true;
		}
	}

	return false;
}

void QMB_Lightning_Splash (vec3_t org);
extern cvar_t scr_ofsy;
/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	int			i;
	beam_t		*b;
	vec3_t		dist, org, beamstart;
	float		d;
	entity_t	*ent;
	float		yaw, pitch;
	float		forward;

	int			j;
	vec3_t		beamend;
//	qboolean	sparks = false;

	num_temp_entities = 0;

// update lightning
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl.viewentity)
		{
			VectorCopy (cl_entities[cl.viewentity].origin, b->start);

			b->start[2] += cl.crouch + bound(-7, scr_ofsy.value, 4);
			b->start[2] += bound(0, cl_lightning_zadjust.value, 20);//progs.dat aims from 20 for traceline

			if (cl_truelightning.value)
			{
				vec3_t	forward, v, org, ang;
				float	f, delta;
				trace_t	trace;

				f = fmax(0, fmin(1, cl_truelightning.value));

				VectorSubtract (playerbeam_end, cl_entities[cl.viewentity].origin, v);
				//v[2] -= 22;		// adjust for view height
				v[2] -= cl.crouch; //
				v[2] -= bound(0, cl_lightning_zadjust.value, 20);

				vectoangles (v, ang);

				// lerp pitch
				ang[0] = -ang[0];
				if (ang[0] < -180)
					ang[0] += 360;
				ang[0] += (cl.viewangles[0] - ang[0]) * f;

				// lerp yaw
				delta = cl.viewangles[1] - ang[1];
				if (delta > 180)
					delta -= 360;
				if (delta < -180)
					delta += 360;
				ang[1] += delta * f;
				ang[2] = 0;

				AngleVectors (ang, forward, NULLVEC, NULLVEC);
				VectorScale(forward, 600, forward);
				VectorCopy(cl_entities[cl.viewentity].origin, org);
				org[2] += bound(0, cl_lightning_zadjust.value, 20);//progs.dat aims from 20 for teaceline
				VectorAdd(org, forward, b->end);

				memset (&trace, 0, sizeof(trace_t));
				if (!SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, org, b->end, &trace))
					VectorCopy(trace.endpos, b->end);
			}
		}

/*
	// if coming from the player, update the start position
		if (b->entity == cl.viewentity)
		{
			VectorCopy (cl_entities[cl.viewentity].origin, b->start);
		}
*/
	// calculate pitch and yaw
		VectorSubtract (b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2f(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrtf (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (int) (atan2f(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}
        // add new entities for the lightning
		VectorCopy(b->start, org);
		VectorCopy(b->start, beamstart);
		d = VectorNormalize (dist);
		VectorScale (dist, 30, dist);

		if (key_dest == key_game)
		{
			for ( ; d > 0 ; d -= 30)
			{
				if ((qmb_initialized && r_part_lightning.value) && (!cl.paused))
				{
					VectorAdd(org, dist, beamend);
					for (j=0 ; j<3 ; j++)
						beamend[j] += ((rand()%10)-5);
					QMB_LightningBeam (beamstart, beamend);
					//if ((r_glowlg.value) && (r_dynamic.value))
					//	CL_NewDlight (i, beamstart, 100, 0.1, lt_blue);
					VectorCopy(beamend, beamstart);
				}
				else
				{
					if (!(ent = CL_NewTempEntity()))
						return;
					VectorCopy(org, ent->origin);
					ent->model = b->model;
					ent->angles[0] = pitch;
					ent->angles[1] = yaw;
					ent->angles[2] = rand() % 360;
				}
				VectorAdd(org, dist, org);
			}
		}
/*
	// add new entities for the lightning
		VectorCopy (b->start, org);
		d = VectorNormalize(dist);
     	while (d > 0)
		{
			ent = CL_NewTempEntity ();
			if (!ent)
				return;
			VectorCopy (org, ent->origin);
			ent->model = b->model;
			ent->angles[0] = pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = rand()%360;

			for (i=0 ; i<3 ; i++)
				org[i] += dist[i]*30;
			d -= 30;
		}
*/
	}
	
}


