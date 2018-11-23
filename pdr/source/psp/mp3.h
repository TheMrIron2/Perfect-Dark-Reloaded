// additional stuff for PSP mp3 decoder implementation
#ifdef __cplusplus
extern "C" {
#endif

int mp3_init(void);
void mp3_deinit(void);
int mp3_start_play(char *fname, int pos);

extern int mp3_job_started;

extern int mp3_volume;
int changeMp3Volume;
extern int mp3_status;
extern int mp3_pause;

enum
{
MP3_PLAY,
MP3_STOP,
MP3_END,
MP3_FREE,
MP3_ERR,
MP3_NEXT
};

#ifdef __cplusplus
}
#endif
