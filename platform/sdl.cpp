#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <SDL.h>

#include "platform.h"
#include "snes9x.h"
#include "gfx.h"
#include "display.h"
#include "memmap.h"
#include "soundux.h"
#include "hacks.h"
#include "snapshot.h"
#include "hgw.h"

#define kPollEveryNFrames		5		//Poll input only every this many frames
#define kPollHgwEveryNFrames	10		//Poll osso only every this many frames

#define TRACE printf("trace: %s:%s\n", __FILE__, __func__);
#define DIE(format, ...) do { \
		fprintf(stderr, "Died at %s:%d: ", __FILE__, __LINE__ ); \
		fprintf(stderr, format "\n", ## __VA_ARGS__); \
		abort(); \
	} while (0);

void S9xMessage(int type, int number, const char * message)
{
	printf("%s\n", message);
}

void S9xLoadSDD1Data()
{TRACE
	Settings.SDD1Pack=FALSE;
}

void S9xAutoSaveSRAM()
{
	Memory.SaveSRAM(S9xGetFilename(".srm"));
}

static void S9xInit() 
{
	if (!Memory.Init () || !S9xInitAPU())
         DIE("Memory or APU failed");

	if (!S9xInitSound ())
		DIE("Sound failed");
	S9xSetSoundMute (TRUE);
	
	// TODO: PAL/NTSC something better than this
	Settings.PAL = Settings.ForcePAL;
	
	Settings.FrameTime = Settings.PAL?Settings.FrameTimePAL:Settings.FrameTimeNTSC;
	Memory.ROMFramesPerSecond = Settings.PAL?50:60;
	
	IPPU.RenderThisFrame = TRUE;
}

static void loadRom()
{
	const char * file = S9xGetFilename(".smc");

	printf("ROM: %s\n", file);

	if (!Memory.LoadROM(file))
		DIE("Loading ROM failed");
	
	file = S9xGetFilename(".srm");
	printf("SRAM: %s\n", file);
	Memory.LoadSRAM(file); 
}

static void resumeGame()
{
	if (!Config.snapshotLoad) return;

	const char * file = S9xGetFilename(".frz.gz");
	int result = S9xUnfreezeGame(file);

	printf("Unfreeze: %s", file);

	if (!result) {
		printf(" failed");
		FILE* fp = fopen(file, "rb");
		if (fp) {
			if (Config.snapshotSave) {
				puts(", but the file exists, so I'm not going to overwrite it");
				Config.snapshotSave = false;
			} else {
				puts(" (bad file?)");
			}
			fclose(fp);
		} else {
			puts(" (file does not exist)");
		}
	} else {
		puts(" ok");
	}
}

static void pauseGame()
{
	if (!Config.snapshotSave) return;

	const char * file = S9xGetFilename(".frz.gz");
	int result = S9xFreezeGame(file);

	printf("Freeze: %s", file);

	if (!result) {
		Config.snapshotSave = false; // Serves as a flag to Hgw
		puts(" failed");
	} else {
		puts(" ok");
	}
}

/* This comes nearly straight from snes9x */
static void frameSync() {
	static struct timeval next1 = {0, 0};
	struct timeval now;
	
	if (Settings.TurboMode)
	{
		if(Settings.SkipFrames == AUTO_FRAMERATE || 
			++IPPU.FrameSkip >= Settings.SkipFrames)
		{
			IPPU.FrameSkip = 0;
			IPPU.SkippedFrames = 0;
			IPPU.RenderThisFrame = TRUE;
		}
		else
		{
			++IPPU.SkippedFrames;
			IPPU.RenderThisFrame = FALSE;
		}
		return;
	}
	
	/* Normal mode */
	
	while (gettimeofday(&now, 0) < 0);
	
	/* If there is no known "next" frame, initialize it now */
	if (next1.tv_sec == 0) { next1 = now; ++next1.tv_usec; }
	
    /* If we're on AUTO_FRAMERATE, we'll display frames always
     * only if there's excess time.
     * Otherwise we'll display the defined amount of frames.
     */
    unsigned limit = Settings.SkipFrames == AUTO_FRAMERATE
                     ? (timercmp(&next1, &now, <) ? 10 : 1)
                     : Settings.SkipFrames;
                     
    IPPU.RenderThisFrame = ++IPPU.SkippedFrames >= limit;
    if(IPPU.RenderThisFrame)
    {
        IPPU.SkippedFrames = 0;
    }
    else
    {
        /* If we were behind the schedule, check how much it is */
        if(timercmp(&next1, &now, <))
        {
            unsigned lag =
                (now.tv_sec - next1.tv_sec) * 1000000
               + now.tv_usec - next1.tv_usec;
            if(lag >= 500000)
            {
                /* More than a half-second behind means probably
                 * pause. The next line prevents the magic
                 * fast-forward effect.
                 */
                next1 = now;
            }
        }
    }
    
    /* Delay until we're completed this frame */

    /* Can't use setitimer because the sound code already could
     * be using it. We don't actually need it either.
     */

    while(timercmp(&next1, &now, >))
    {
        /* If we're ahead of time, sleep a while */
        unsigned timeleft =
            (next1.tv_sec - now.tv_sec) * 1000000
           + next1.tv_usec - now.tv_usec;

        usleep(timeleft);

        // XXX : CHECK_SOUND(); S9xProcessEvents(FALSE);

        while (gettimeofday(&now, 0) < 0);
        /* Continue with a while-loop because usleep()
         * could be interrupted by a signal
         */
    }

    /* Calculate the timestamp of the next frame. */
    next1.tv_usec += Settings.FrameTime;
    if (next1.tv_usec >= 1000000)
    {
        next1.tv_sec += next1.tv_usec / 1000000;
        next1.tv_usec %= 1000000;
    }
}

/** Wraps s9xProcessEvents, taking care of kPollEveryNFrames */
static inline void pollEvents() {
	static int frames = 0;
	
	if (++frames > kPollEveryNFrames) {
		S9xProcessEvents(FALSE);
		frames = 0;
	}
}

/** Wraps HgwPollEvents, taking care of kPollHgwEveryNFrames */
static inline void pollHgwEvents() {
	static int frames = 0;
	
	if (!hgwLaunched) return;
	
	if (++frames > kPollHgwEveryNFrames) {
		HgwPollEvents();
		frames = 0;
	}
}

int main(int argc, const char ** argv) {	
	// Initialise SDL
	if (SDL_Init(0) < 0) 
		DIE("SDL_Init: %s", SDL_GetError());
	
	// Configure snes9x
	HgwInit();						// Hildon-games-wrapper initialization.
	S9xLoadConfig(argc, argv);		// Load config files and parse cmd line.
	HgwConfig();					// Apply specific hildon-games config.
	
	// S9x initialization
	S9xInitDisplay(argc, argv);
	S9xInitAudioOutput();
	S9xInitInputDevices();
	S9xInit();
	S9xReset();
	
	// Load rom and related files: state, unfreeze if needed
	loadRom();
	resumeGame();
	
	// Late initialization
	sprintf(String, "DrNokSnes - %s", Memory.ROMName);
	S9xSetTitle(String);
	S9xHacksLoadFile(Config.hacksFile[0] ? Config.hacksFile : 0);
	if (!S9xGraphicsInit())
         DIE("S9xGraphicsInit failed");
	S9xAudioOutputEnable(true);

	do {
		frameSync();			// May block, or set frameskip to true.
		S9xMainLoop();			// Does CPU things, renders if needed.
		pollEvents();
		pollHgwEvents();
	} while (!Config.quitting);
	
	// Deinitialization
	S9xAudioOutputEnable(false);
	S9xDeinitAudioOutput();
	S9xDeinitDisplay();

	// Save state
	Memory.SaveSRAM(S9xGetFilename(".srm"));
	pauseGame();

	// Late deinitialization
	S9xGraphicsDeinit();
	Memory.Deinit();
	HgwDeinit();

	SDL_Quit();

	return 0;
}

void S9xDoAction(unsigned char action)
{
	if (action & kActionQuit) 
		Config.quitting = true;
		
	if (action & kActionToggleFullscreen)
		S9xVideoToggleFullscreen();
}

