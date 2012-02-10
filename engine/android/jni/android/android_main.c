/*
 *
 * Notes:
 *
 * zlib is a must since all asset are compressed in an Android apk.
 * ndk-build and ndk-stack are invaluable tools.
 * Eclipse and build system detect automatically when the library has been recompiled.
 * printf are delayed so don't rely on them to debug. Use ndk-stack and -sym.
 * EDIT: printf are not delayed but IGNORED !!! Be very careful to have the #define printf everywhere so you don't
 *       call the stdio printf: If you fall for this you won't see any trace.
 * I cannot get the assetManage and AAssetManager_openDir to work, all I can see is three files in "/": androidManifest.xml, classes.dex and ressources.arsc
 * It seems AAssetManager_openDir only list files. So "" returned nothings and so did "assets", "assets/data" did return
 * my two files.
 *
 * Getting closer to completion now. It seems the native AAssetManager_open refuses to load PNGs since they are already compressed.
 * http://ponystyle.com/blog/2010/03/26/dealing-with-asset-compression-in-android-apps/, aapt (Android Asset Packaging Tool) is messing them up.
 *
 * I was completely wrong about AAssetManager_open it can load anything, we just have to omit "assets" at the beginning and start with "data" right away.
 *
*/
#include <stdio.h>
#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include "../../../src/dEngine.h"

#include "android_display.h"

#define  LOG_TAG    		"net.fabiensanglard.native"
#define  LOGI(...)  		__android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  		__android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  		__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define printf(fmt,args...) __android_log_print(ANDROID_LOG_INFO  ,LOG_TAG, fmt, ##args)

int gameOn = 0;

#include "android/asset_manager.h"
#include "android/native_activity.h"
void ListDirectory(AAssetManager*      assetManager, const char* dirName){
	int fileCount=0;
	//Make sure the directory where we will be writing (logs, replays) exists.
			const char* assetURL;
			AAssetDir* directory = AAssetManager_openDir(assetManager,dirName);

			AAssetDir_rewind(directory);

			while((assetURL = AAssetDir_getNextFileName(directory)) != 0)
			{
				fileCount++;
				LOGE(assetURL);
			}
			//fflush(stdout);

			AAssetDir_close(directory);

			printf("Directory \"%s\" has %d entries.\n",dirName,fileCount);
}



static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
        return 1;
}

static void engine_handle_cmd(struct android_app* state, int32_t cmd) {
    //struct engine* engine = (struct engine*)app->userData;

    ANativeActivity*    activity  ;
    AAssetManager*      assetManager;
    activity = state->activity;
    assetManager = activity->assetManager;

    switch (cmd) {
        case APP_CMD_SAVE_STATE:
        	printf("APP_CMD_SAVE_STATE");
            break;
        case APP_CMD_INIT_WINDOW:
        	printf("APP_CMD_INIT_WINDOW");
        	window = state->window;
        	engine_init_display();
        	engine_draw_frame();

            break;
        case APP_CMD_TERM_WINDOW:
        	printf("APP_CMD_TERM_WINDOW");
            // The window is being hidden or closed, clean it up.
        	engine_term_display();
        	gameOn = 0;
            break;
        case APP_CMD_GAINED_FOCUS:
        	printf("APP_CMD_GAINED_FOCUS");
            break;
        case APP_CMD_LOST_FOCUS:
        	printf("APP_CMD_LOST_FOCUS");
        	engine_term_display();
        	gameOn = 0;
            break;
    }
}


void FS_AndroidPreInitFileSystem(struct android_app* application);


int32_t getTickCount() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec*1000000000LL + now.tv_nsec;
}

#define FRAMES_PER_SECOND 60
#define  SKIP_TICKS (1000 / FRAMES_PER_SECOND)

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {

	app_dummy();

	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;



	LOGE("[FS_AndroidPreInitFileSystem]\n");



	FS_AndroidPreInitFileSystem(state);

	//Init everything except for the rendering system.

	renderer.materialQuality = MATERIAL_QUALITY_HIGH;
	dEngine_Init();

	gameOn = 1;

	int32_t next_game_tick = getTickCount();
	int sleep_time = 0;
	//Loop for ever, pump event and dispatch them.
	   while (gameOn) {
	        // Read all pending events.
	        int ident;
	        int events;
	        struct android_poll_source* source;

	        // If not animating, we will block forever waiting for events.
	        // If animating, we loop until all events are read, then continue
	        // to draw the next frame of animation.
	        while ((ident=ALooper_pollAll(0 , NULL, &events, (void**)&source)) >= 0) {

	            // Process this event.
	            if (source != NULL) {
	                source->process(state, source);
	            }

	            engine_draw_frame();
	            next_game_tick += SKIP_TICKS;
	            sleep_time = next_game_tick - getTickCount();
	            if( sleep_time >= 0 ) {
	                 sleep( sleep_time );
	            }

	            // Check if we are exiting.
	            if (state->destroyRequested != 0) {

	                break;
	                gameOn = 0;
	            }
	        }

	    }


}
