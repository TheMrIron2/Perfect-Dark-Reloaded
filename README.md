==========
DQuakePlus
==========
Fork of DQuake>>http://sourceforge.net/projects/pdquake/ 
Original DQuake by Crow_Bar. 
Used parts from:  
QRack by R00K,   
JoeQuake by Joseph,    
FitzQuake by John Fitzburg,  
Kurok by mdave,    
FuhQuake by fuh a norai,   
FteQuake by Spike,   
and DarkPlaces by Lord Havok!   

Features:
- Q1BSP,HLBSP
- MDL(Quake and Hexen 2), MD2, MD3
- Halflife / CS1.6 Modelformat(without bone controll)
- QMB and Quake 3 Particles
- External Textures (bmp,tga,png,pcx)
- spr(Quake),spr32(DarkPlaces),sp2(Quake2),spr(HL,not working) for sprites.
- decals
- skybox from kurok
- color lightmaps (from kurok / and for hlbsp);
- fog
- usermaps browser    
- nodrawtoclient, drawonlytoclient    
- .ent file support    
- ricochets   
- FRIK_FILE support   
- Rotation brush support    
- MOVETYPE_FOLLOW & MOVETYPE_COMPOUND implemented     
- qcexec   
- sv_freezenonclients    
- HUB System(Hexen 2 version), compile engine with #ifdef BREAKALL flag
==========
How to compile
==========
1)download cygwin>http://quake-1.com/cache/cygwin.rar

2)Extract folder on C://

3)Go in to cygwin folder and launch .bat file

4)Type "cd" and path to psp folder with make files,like a "cd dquakeplus/psp"

5)To compile SLIM EBOOT type "make -f  makeslim install"

  To compile PHAT EBOOT type "make -f makephat install"

==========
Transparency
==========
use prefix "{" for transparent textures like in HalfLife and like Kurok(see kurok.wad for example)     
R:159 G:091 B:083 for transparent textures like a Kurok      
R:0 G:0 B:255 for transparent textures like a Half Life      
little example:      
{mytexture     
For transparent parts on models you need add your models in engine(see DQ+ note in video_hardware_main in psp folder)   

==========
Ricochets
==========
Must be here    
//normal rics    
weapons/rics/ric1.wav   
weapons/rics/ric2.wav   
weapons/rics/ric3.wav   
// wood rics   
weapons/rics/wood_ric1.wav   
weapons/rics/wood_ric2.wav   
weapons/rics/wood_ric3.wav   
// metall rics  
weapons/rics/metal_ric1.wav   
weapons/rics/metal_ric2.wav   
weapons/rics/metal_ric3.wav   

==========
MOVETYPE_FOLLOW
==========
aiment - the entity this is attached to.   
punchangle - the original angles when the follow began.   
view_ofs - the relative origin (note that this is un-rotated by punchangle, and that is actually the only purpose of punchangle).    
v_angle - the relative angles.   
Here's an example of how you would set a bullet hole sprite to follow a bmodel it was created on, even if the bmodel rotates:    
hole.movetype = MOVETYPE_FOLLOW; // make the hole follow    
hole.solid = SOLID_NOT;          // MOVETYPE_FOLLOW is always non-solid   
hole.aiment = bmodel;            // make the hole follow bmodel    
hole.punchangle = bmodel.angles; // the original angles of bmodel   
hole.view_ofs = hole.origin - bmodel.origin; // relative origin   
hole.v_angle = hole.angles - bmodel.angles;  // relative angles   

=========
MOVETYPE_COMPOUND
=========
It allows to tie one entity to another, taking into account movement and rotation of the former entity.   




