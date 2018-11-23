/*
Copyright (C) 2010 Crow_bar.

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
// r_main.c

extern "C"
{
 #include "../quakedef.h"
}


typedef struct
{
  byte *tex;
  int zone_h
  int zone_w
  int zone_x;
  int zone_y;
} texzone_t;

#define MAX_TEXZONES 128

texzone_t texzonelist[MAX_TEXZONES];
int texzoneused[MAX_TEXZONES];
int numtexzones;


void RenderTexture_8bitGetZone(int inwidth, int inheight, byte *data, int *out_x[2], int *out_y[2])
{
	 int x, y;

	 int next       = 0;
	 int prev       = 0;
     int start_x    = 0;
	 int start_y    = 0;
	 int start_line = 0;
	 int end_x      = 0;
	 int end_y      = 0;
	 int end_line   = inheight;

	 for(x = 0; x < inwidth; x++)
	 {
		 for(y = next_line; y < end_line; y++)
	     {
			 next = data[x*y];
			 if((next == 255) && (next != prev))
			 {
                 if(!statrt_line)
                 {
					 start_x  = x;
		             start_y  = y;
					 start_line = y;
                 }
			 }
			 
			 if((prev == 255) && (prev != next))
			 {
	             end_x = x;
	             end_y = y;

				 if(!end_line)
				     end_line  = y;
			 }
			 prev = next;
	     }
	 }

	 //position
	 *out_x[0] = start_x;
	 *out_y[0] = start_y;

	 //size
	 *out_x[1] = end_x;
	 *out_y[1] = end_y;
}

byte* RenderTexture_GetTex(void)
{
	 byte *texture;
	 sceGuDrawBufferList(GU_PSM_4444,vrelptr(renderTarget), texture);
	 
	 

	 return reinterpret_cast<char*>(sceGeEdramGetAddr()) + reinterpret_cast<std::size_t>(vrelptr(renderTarget));
}







