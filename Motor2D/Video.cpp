#include "j1App.h"
#include "j1FileSystem.h"
#include "Video.h"
#include "Log.h"
#include "j1Window.h"
#include "j1Input.h"
#include "j1Textures.h"
#include "j1Render.h"
#include "j1Audio.h"
#include "j1Scene.h"
#include "SceneTest.h"
#include <sstream>

#include <stdio.h>

#pragma comment( lib, "ogg/libogg.lib" )
#pragma comment( lib, "vorbis/libvorbis.lib" )
#pragma comment( lib, "theora/libtheora.lib" )

#include "theora/theoradec.h"
#include "vorbis/codec.h"
#include "theoraplay.h"

Video::Video() : j1Module()
{
	decoder = NULL;
	video = NULL;
	audio = NULL;
	screen = NULL;
	texture = NULL;
	pixels = NULL;

	baseticks = 0;
	framems = 0;
	pause = false;
	init_failed = 0;
	quit = 0;
	pitch = 0;
	want_to_play = false;

	name = "video";
}

// Destructor
Video::~Video()
{}

// Called before render is available
bool Video::Awake(pugi::xml_node& config)
{
	return true;
}

// Called before quitting
bool Video::CleanUp()
{
	if (!active)
		return true;

	if (texture) SDL_DestroyTexture(texture);
	if (video) THEORAPLAY_freeVideo(video);
	if (audio) THEORAPLAY_freeAudio(audio);
	if (decoder) THEORAPLAY_stopDecode(decoder);
	SDL_CloseAudio();
	SDL_Quit();

	return true;
}


// Audio =============================================================================================

AudioQueue* Video::audio_queue = NULL;
AudioQueue* Video::audio_queue_tail = NULL;

void SDLCALL Video::audio_callback(void *userdata, Uint8 *stream, int len)
{
	Sint16 *dst = (Sint16 *)stream;

	while (audio_queue && (len > 0)) {
		volatile AudioQueue *item = audio_queue;
		AudioQueue *next = item->next;
		const int channels = item->audio->channels;

		const float *src = item->audio->samples + (item->offset * channels);
		int cpy = (item->audio->frames - item->offset) * channels;
		int i;

		if (cpy > (len / sizeof(Sint16)))
			cpy = len / sizeof(Sint16);

		for (i = 0; i < cpy; i++) {
			const float val = *(src++); //crash
			if (val < -1.0f)
				*(dst++) = -32768;
			else if (val > 1.0f)
				*(dst++) = 32767;
			else
				*(dst++) = (Sint16)(val * 32767.0f);
		}

		item->offset += (cpy / channels);
		len -= cpy * sizeof(Sint16);

		if (item->offset >= item->audio->frames) {
			THEORAPLAY_freeAudio(item->audio);
			SDL_free((void *)item);
			audio_queue = next;
		}
	}

	if (!audio_queue)
		audio_queue_tail = NULL;

	if (len > 0)
		memset(dst, '\0', len);
}


void Video::queue_audio(const THEORAPLAY_AudioPacket *audio)
{
	AudioQueue *item = (AudioQueue *)SDL_malloc(sizeof(AudioQueue));
	if (!item) {
		THEORAPLAY_freeAudio(audio);
		return;
	}

	item->audio = audio;
	item->offset = 0;
	item->next = NULL;

	SDL_LockAudio();
	if (audio_queue_tail)
		audio_queue_tail->next = item;
	else
		audio_queue = item;
	audio_queue_tail = item;
	SDL_UnlockAudio();
}

// Video ===========================================================================================

void Video::LoadVideo(const char *fname)
{
	// Loading --------------------------------------------------------

	decoder = THEORAPLAY_startDecodeFile(fname, 30, THEORAPLAY_VIDFMT_IYUV);

	while (!audio || !video) {
		if (!audio) audio = THEORAPLAY_getAudio(decoder);
		if (!video) video = THEORAPLAY_getVideo(decoder);
		SDL_Delay(10);
	}

	framems = (video->fps == 0.0) ? 0 : ((Uint32)(1000.0 / video->fps));

	screen = App->win->window;
	texture = SDL_CreateTexture(App->render->renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video->width, video->height);

	init_failed = quit = (!screen || !texture);

	memset(&spec, '\0', sizeof(SDL_AudioSpec));
	spec.freq = audio->freq;
	spec.format = AUDIO_S16SYS;
	spec.channels = audio->channels;
	spec.samples = 2048;
	spec.callback = audio_callback;

	init_failed = quit = (init_failed || (SDL_OpenAudio(&spec, NULL) != 0)); // Audio is already opened?

	SDL_PauseAudio(0);
}

void Video::PlayVideo(const char *fname, SDL_Rect r)
{
	// Taking control in the sound -------

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	// Loading video ---------------------

	ResetValues();
	rendering_rect = r;
	LoadVideo(fname);

	// Playing video ---------------------

	baseticks = SDL_GetTicks();
	want_to_play = true;

}

bool Video::Update(float dt)
{
	if (want_to_play && !quit && THEORAPLAY_isDecoding(decoder))
	{
		// Events ------------------------------------------------------
		if (App->input->GetKey(SDL_SCANCODE_ESCAPE) == key_down)
			quit = 1;

		// ----------------------------------------------------------------

		Uint32 now = SDL_GetTicks() - baseticks;

		if (!video)
			video = THEORAPLAY_getVideo(decoder);

		// Setting new texture --------------------------------------------

		if (!pause && video && (video->playms <= now))
		{
			if (framems && ((now - video->playms) >= framems))
			{
				const THEORAPLAY_VideoFrame *last = video;
				while ((video = THEORAPLAY_getVideo(decoder)) != NULL)
				{
					THEORAPLAY_freeVideo(last);
					last = video;
					if ((now - video->playms) < framems)
						break;
				}
				if (!video)
					video = last;
			}

			SDL_LockTexture(texture, NULL, &pixels, &pitch);
			const int w = video->width;
			const int h = video->height;
			const Uint8 *y = (const Uint8 *)video->pixels;
			const Uint8 *u = y + (w * h);
			const Uint8 *v = u + ((w / 2) * (h / 2));
			Uint8 *dst = (Uint8*)pixels;
			int i;

			for (i = 0; i < h; i++, y += w, dst += pitch)
				memcpy(dst, y, w);

			for (i = 0; i < h / 2; i++, u += w / 2, dst += pitch / 2)
				memcpy(dst, u, w / 2);

			for (i = 0; i < h / 2; i++, v += w / 2, dst += pitch / 2)
				memcpy(dst, v, w / 2);

			SDL_UnlockTexture(texture);

			THEORAPLAY_freeVideo(video);
			video = NULL;
		} // if
		else
		{
			SDL_Delay(10);
		}

		if ((audio = THEORAPLAY_getAudio(decoder)) != NULL && !pause)
			queue_audio(audio);

		// Render texture ------------------------------------------------

		//SDL_RenderClear(App->render->renderer);
		SDL_RenderCopy(App->render->renderer, texture, NULL, &rendering_rect);
		//SDL_RenderPresent(App->render->renderer);

	}

	if (!THEORAPLAY_isDecoding(decoder) && App->scene->scene_test->is_video_active == true)
	{
		quit = true;
	}

	if (quit)
	{
		ResetValues();
		App->scene->scene_test->is_video_active = false;
	}

	return true;
}

void Video::ResetValues()
{
	if (texture) SDL_DestroyTexture(texture);
	if (video) THEORAPLAY_freeVideo(video);
	if (audio) THEORAPLAY_freeAudio(audio);
	if (decoder) THEORAPLAY_stopDecode(decoder);

	decoder = NULL;
	video = NULL;
	audio = NULL;
	screen = NULL;
	texture = NULL;
	pixels = NULL;

	baseticks = 0;
	framems = 0;
	pause = false;
	init_failed = 0;
	quit = 0;
	pitch = 0;
	want_to_play = false;

	audio_queue = NULL;
	audio_queue_tail = NULL;
}
