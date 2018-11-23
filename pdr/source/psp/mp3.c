/*
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 * Copyright (c) 2009 Crow_bar
*/
#include <pspkernel.h>
#include <pspsdk.h>
#include <stdio.h>
#include <string.h>
#include <pspaudiocodec.h>
#include <pspmp3.h>
#include <pspaudio.h> 
#include <psputility.h>
#include "mp3.h"


int mp3_last_error = 0;

static int initialized = 0;
static SceUID thread_job_sem = -1;
static SceUID thread_busy_sem = -1;
static int thread_exit = 0;

int mp3_job_started = 0;

// Input and Output buffers
static char  mp3Buf[32*1024]      __attribute__((aligned(64)));
static short pcmBuf[32*(1152/2)]  __attribute__((aligned(64)));

static const char *mp3_fname = NULL;
static SceUID mp3_handle = -1;
static int mp3_src_pos = 0, mp3_src_size = 0;
int mp3_volume, mp3_status, mp3_pause;
/*
enum
{
MP3_PLAY,
MP3_STOP,
MP3_END,
MP3_FREE,
MP3_ERR,
MP3_NEXT
};
*/
static int decode_thread(SceSize args, void *argp);

static void psp_sem_lock(SceUID sem)
{
	int ret = sceKernelWaitSema(sem, 1, 0);
	if (ret < 0) printf("sceKernelWaitSema(%08x) failed with %08x\n", sem, ret);
}

static void psp_sem_unlock(SceUID sem)
{
	int ret = sceKernelSignalSema(sem, 1);
	if (ret < 0) printf("sceKernelSignalSema(%08x) failed with %08x\n", sem, ret);
}

int fillStreamBuffer( int afd, int handle ) 
{ 
   char* dst; 
   int write; 
   int pos; 
   // Get Info on the stream (where to fill to, how much to fill, where to fill from) 
   int status = sceMp3GetInfoToAddStreamData( handle, &dst, &write, &pos); 
   if (status<0) 
   { 
      printf("ERROR: sceMp3GetInfoToAddStreamData returned 0x%08X\n", status); 
   } 

   // Seek file to position requested 
   status = sceIoLseek32( afd, pos, SEEK_SET ); 
   if (status<0) 
   { 
      printf("ERROR: sceIoLseek32 returned 0x%08X\n", status); 
   } 
    
   // Read the amount of data 
   int read = sceIoRead( afd, dst, write ); 
   if (read < 0) 
   { 
      printf("ERROR: Could not read from file - 0x%08X\n", read); 
   } 
    
   if (read==0) 
   { 
      // End of file? 
      return 0; 
   } 
    
   // Notify mp3 library about how much we really wrote to the stream buffer 
   status = sceMp3NotifyAddStreamData( handle, read ); 
   if (status<0) 
   { 
      printf("ERROR: sceMp3NotifyAddStreamData returned 0x%08X\n", status); 
   } 
    
   return (pos>0); 
}

int mp3_init(void)
{
	SceUID thid;
	int ret = 0;
     
      // Load modules
	int status = sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
	if (status<0)
	{
		printf("ERROR: sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC) returned 0x%08X\n", status);
	    goto fail;
    }
	
	status = sceUtilityLoadModule(PSP_MODULE_AV_MP3);
	if (status<0)
	{
		printf("ERROR: sceUtilityLoadModule(PSP_MODULE_AV_MP3) returned 0x%08X\n", status);
	    goto fail;
    }
      
    // Init mp3 resources
	status = sceMp3InitResource();
	if (status<0)
	{
		printf("ERROR: sceMp3InitResource returned 0x%08X\n", status);
	    goto fail;
    }

	/* thread and stuff */
	thread_job_sem = sceKernelCreateSema("p_mp3job_sem", 0, 0, 1, NULL);
	if (thread_job_sem < 0) {
		printf("sceKernelCreateSema() failed: %08x\n", thread_job_sem);
		ret = thread_job_sem;
		goto fail1;
	}

	thread_busy_sem = sceKernelCreateSema("p_mp3busy_sem", 0, 1, 1, NULL);
	if (thread_busy_sem < 0) {
		printf("sceKernelCreateSema() failed: %08x\n", thread_busy_sem);
		ret = thread_busy_sem;
		goto fail2;
	}

	thread_exit = 0;
	thid = sceKernelCreateThread("mp3decode_thread", decode_thread, 30, 0x2000, 0, 0); /* use slightly higher prio then main */
	if (thid < 0) {
		printf("failed to create decode thread: %08x\n", thid);
		ret = thid;
		goto fail3;
	}
	
	ret = sceKernelStartThread(thid, 0, 0);
	if (ret < 0) 
	{
		printf("failed to start decode thread: %08x\n", ret);
		goto fail3;
	}

	mp3_last_error = 0;
	initialized = 1;
	return 0;

fail3:
	sceKernelDeleteSema(thread_busy_sem);
	thread_busy_sem = -1;
fail2:
	sceKernelDeleteSema(thread_job_sem);
	thread_job_sem = -1;
fail1:
	//sceAudiocodecReleaseEDRAM(mp3_codec_struct);
fail:
	mp3_last_error = ret;
	initialized = 0;
	return 1;
}

void mp3_deinit(void)
{
	printf("mp3_deinit, initialized=%i\n", initialized);

	if (!initialized) return;
	thread_exit = 1;
	psp_sem_lock(thread_busy_sem);
	psp_sem_unlock(thread_busy_sem);

	sceKernelSignalSema(thread_job_sem, 1);
	sceKernelDelayThread(100*1000);

	if (mp3_handle >= 0) 
	  sceIoClose(mp3_handle);
	mp3_handle = -1;
	mp3_fname = NULL;
	
	psp_sem_lock(thread_job_sem);
	psp_sem_unlock(thread_job_sem);

	sceKernelDeleteSema(thread_busy_sem);
	thread_busy_sem = -1;
	sceKernelDeleteSema(thread_job_sem);
	thread_job_sem = -1;
	
	int status = sceMp3TermResource();
	if (status<0)
	{
		printf("ERROR: sceMp3TermResource returned 0x%08X\n", status);
	}

	sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC);
	sceUtilityUnloadModule(PSP_MODULE_AV_MP3);

	initialized = 0;
	
}


short mp3_output_buffer[4][1152 * 2] __attribute__((aligned(64)));
int mp3_output_index = 0; 

// may overflow stack?
static int decode_thread(SceSize args, void *argp)
{
	while (!thread_exit)
	{
		psp_sem_lock(thread_job_sem);
		
		if (thread_exit) 
        {
			psp_sem_unlock(thread_job_sem);
			break;
		}

		psp_sem_lock(thread_busy_sem);

	    // Reserve a mp3 handle for our playback

	    SceMp3InitArg mp3Init;

	    mp3Init.mp3StreamStart = 0;
	    mp3Init.mp3StreamEnd   = mp3_src_size;
	    mp3Init.unk1           = 0;
	    mp3Init.unk2           = 0;
	    mp3Init.mp3Buf         = mp3Buf;
	    mp3Init.mp3BufSize     = sizeof(mp3Buf);
	    mp3Init.pcmBuf         = pcmBuf;
	    mp3Init.pcmBufSize     = sizeof(pcmBuf);

	    int handle = sceMp3ReserveMp3Handle( &mp3Init );
	    if (handle<0)
	    {
	     printf("ERROR: sceMp3ReserveMp3Handle returned 0x%08X\n", handle);
	    }

	    // Fill the stream buffer with some data so that sceMp3Init has something to work with
	    fillStreamBuffer( mp3_handle, handle );

	    int status = sceMp3Init( handle );
	    if (status<0)
	    {
	         printf("ERROR: sceMp3Init returned 0x%08X\n", status);
	         mp3_status = MP3_ERR;
			 mp3_job_started = 0;
		}

	 	int channel      = -1;
	    int samplingRate = sceMp3GetSamplingRate( handle );
	    int numChannels  = sceMp3GetMp3ChannelNum( handle );
	    int lastDecoded  = 0;
	    int numPlayed    = 0;
	    int paused       = 0;

		status = sceMp3SetLoopNum( handle, 0 );
	    if (status<0)
		{
		 	printf("ERROR: sceMp3SetLoopNum returned 0x%08X\n", status);
		}
		
		while (mp3_job_started)
		{
		
		    //if (!mp3_pause)
		    {
			    // Check if we need to fill our stream buffer
			    if (sceMp3CheckStreamDataNeeded( handle )>0)
			    {
				   fillStreamBuffer( mp3_handle, handle );
			    }

			    // Decode some samples
			    short* buf;
			    int bytesDecoded;
			    int retries = 0;
			    // We retry in case it's just that we reached the end of the stream and need to loop
			    for (;retries<1;retries++)
			    {
				    bytesDecoded = sceMp3Decode( handle, &buf );
				    if (bytesDecoded>0)
					     break;
				
				    if (sceMp3CheckStreamDataNeeded( handle )<=0)
					     break;
				
				    if (!fillStreamBuffer( mp3_handle, handle ))
				    {
					    numPlayed = 0;
				    }
			    }
			    if (bytesDecoded<0 && bytesDecoded!=0x80671402)
			    {
				    printf("ERROR: sceMp3Decode returned 0x%08X\n", bytesDecoded);
			        mp3_status = MP3_ERR;
					mp3_job_started = 0;
				}
			
			    // Nothing more to decode? Must have reached end of input buffer
			    if (bytesDecoded==0 || bytesDecoded==0x80671402)
			    {
				   printf("mp3_end_play\n");
					 //paused = 1;
				   //sceMp3ResetPlayPosition( handle );
				   mp3_status = MP3_FREE;
				   mp3_job_started = 0;
				   numPlayed = 0;
			    }
			    else
			    {
				  // Reserve the Audio channel for our output if not yet done
				    if (channel<0 || lastDecoded!=bytesDecoded)
				    {
					    if (channel>=0)
						     sceAudioSRCChRelease();
					
					    channel = sceAudioSRCChReserve( bytesDecoded/(2*numChannels), samplingRate, numChannels );
				    }
				    // Output the decoded samples and accumulate the number of played samples to get the playtime
				    numPlayed += sceAudioSRCOutputBlocking( mp3_volume, buf );
			    }
		    }			
		}

		if (!mp3_job_started) 
        {
			// Cleanup time...
            if (channel>=0)
		      sceAudioSRCChRelease();

			printf("Clear res\n");
 
 			status = sceMp3ReleaseMp3Handle( handle );
		    if (status<0)
		    {
			   printf("ERROR: sceMp3ReleaseMp3Handle returned 0x%08X\n", status);
		    }

			if (mp3_handle >= 0) 
              sceIoClose(mp3_handle);

			mp3_handle = -1;
			mp3_fname = NULL;
		  
			if(mp3_status == MP3_ERR)
		        mp3_status = MP3_NEXT;
			else
			{
			 if(mp3_status != MP3_FREE)
			    mp3_status = MP3_STOP;
			 else
			    mp3_status = MP3_END;
		    }
		}
		psp_sem_unlock(thread_busy_sem);
	}

	printf("leaving decode thread\n");
	sceKernelExitDeleteThread(0);
	return 0;
}

int mp3_start_play(char *fname, int pos)
{

	printf("mp3_start_play(%s) @ %i\n", fname, pos);
	psp_sem_lock(thread_busy_sem);

	if (mp3_fname != fname || mp3_handle < 0)
	{
		if (mp3_handle >= 0)
		    sceIoClose(mp3_handle);

		mp3_handle = sceIoOpen(fname, PSP_O_RDONLY, 0777);
	
		if (mp3_handle < 0) 
		{
			printf("sceIoOpen(%s) failed\n", fname);
			mp3_status = MP3_FREE;
			psp_sem_unlock(thread_busy_sem);
			sceIoClose(mp3_handle);
			return 2;
		}
		mp3_src_size = sceIoLseek32(mp3_handle, 0, PSP_SEEK_END);
		mp3_fname = fname;
	}

    mp3_status = MP3_PLAY;

	// seek..
	mp3_src_pos = (int) (((float)pos / 1023.0f) * (float)mp3_src_size);
	sceIoLseek32(mp3_handle, mp3_src_pos, PSP_SEEK_SET);
	printf("seek %i: %i/%i\n", pos, mp3_src_pos, mp3_src_size);

	mp3_job_started = 1;
	//working_buf = 0;

	/* send a request to decode first frame */
	psp_sem_unlock(thread_busy_sem);
	psp_sem_unlock(thread_job_sem);
	sceKernelDelayThread(1); // reschedule
	
	return 0;
}


