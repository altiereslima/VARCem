/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Platform main support module for Windows.
 *
 * Version:	@(#)win.c	1.0.9	2018/03/20
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Sarah Walker, <tommowalker@tommowalker.co.uk>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
 *		Copyright 2016-2018 Miran Grca.
 *		Copyright 2008-2018 Sarah Walker.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free  Software  Foundation; either  version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is  distributed in the hope that it will be useful, but
 * WITHOUT   ANY  WARRANTY;  without  even   the  implied  warranty  of
 * MERCHANTABILITY  or FITNESS  FOR A PARTICULAR  PURPOSE. See  the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *   Free Software Foundation, Inc.
 *   59 Temple Place - Suite 330
 *   Boston, MA 02111-1307
 *   USA.
 */
#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <wchar.h>
#include "../emu.h"
#include "../version.h"
#include "../config.h"
#include "../device.h"
#include "../mouse.h"
#include "../video/video.h"
#define GLOBAL
#include "../plat.h"
#include "../ui.h"
#ifdef USE_VNC
# include "../vnc.h"
#endif
#ifdef USE_RDP
# include "../rdp.h"
#endif
#ifdef USE_WX
# include "../wx/wx_ui.h"
#else
# include "win_ddraw.h"
# include "win_d3d.h"
#endif
#ifdef _MSC_VER
# include <io.h>    /* for _open_osfhandle() */
#endif
#include "win.h"


typedef struct {
    WCHAR str[512];
} rc_str_t;


/* Platform Public data, specific. */
HINSTANCE	hinstance;		/* application instance */
HANDLE		ghMutex;
LCID		lang_id;		/* current language ID used */
DWORD		dwSubLangID;


/* Local data. */
static HANDLE	thMain;
static rc_str_t	*lpRCstr2048,
		*lpRCstr4096,
		*lpRCstr4352,
		*lpRCstr4608,
		*lpRCstr5120,
		*lpRCstr5376,
		*lpRCstr5632,
		*lpRCstr5888,
		*lpRCstr6144,
		*lpRCstr7168;


static struct {
    char	*name;
    int		local;
    int		(*init)(void *);
    void	(*close)(void);
    void	(*resize)(int x, int y);
    int		(*pause)(void);
} vid_apis[2][4] = {
  {
#ifdef USE_WX
    {	"WxWidgets", 1, wx_init, wx_close, NULL, wx_pause		},
    {	"WxWidgets", 1, wx_init, wx_close, NULL, wx_pause		},
#else
    {	"DDraw", 1, (int(*)(void*))ddraw_init, ddraw_close, NULL, ddraw_pause		},
    {	"D3D", 1, (int(*)(void*))d3d_init, d3d_close, d3d_resize, d3d_pause		},
#endif
#ifdef USE_VNC
    {	"VNC", 0, vnc_init, vnc_close, vnc_resize, vnc_pause		},
#else
    {	NULL, 0, NULL, NULL, NULL, NULL					},
#endif
#ifdef USE_RDP
    {	"RDP", 0, rdp_init, rdp_close, rdp_resize, rdp_pause		}
#else
    {	NULL, 0, NULL, NULL, NULL, NULL					}
#endif
  },
  {
#ifdef USE_WX
    {	"WxWidgets", 1, wx_init, wx_close, NULL, wx_pause		},
    {	"WxWidgets", 1, wx_init, wx_close, NULL, wx_pause		},
#else
    {	"DDraw", 1, (int(*)(void*))ddraw_init_fs, ddraw_close, NULL, ddraw_pause	},
    {	"D3D", 1, (int(*)(void*))d3d_init_fs, d3d_close, NULL, d3d_pause		},
#endif
#ifdef USE_VNC
    {	"VNC", 0, vnc_init, vnc_close, vnc_resize, vnc_pause		},
#else
    {	NULL, 0, NULL, NULL, NULL, NULL					},
#endif
#ifdef USE_RDP
    {	"RDP", 0, rdp_init, rdp_close, rdp_resize, rdp_pause		}
#else
    {	NULL, 0, NULL, NULL, NULL, NULL					}
#endif
  },
};


static void
LoadCommonStrings(void)
{
    int i;

    lpRCstr2048 = (rc_str_t *)malloc(STR_NUM_2048*sizeof(rc_str_t));
    lpRCstr4096 = (rc_str_t *)malloc(STR_NUM_4096*sizeof(rc_str_t));
    lpRCstr4352 = (rc_str_t *)malloc(STR_NUM_4352*sizeof(rc_str_t));
    lpRCstr4608 = (rc_str_t *)malloc(STR_NUM_4608*sizeof(rc_str_t));
    lpRCstr5120 = (rc_str_t *)malloc(STR_NUM_5120*sizeof(rc_str_t));
    lpRCstr5376 = (rc_str_t *)malloc(STR_NUM_5376*sizeof(rc_str_t));
    lpRCstr5632 = (rc_str_t *)malloc(STR_NUM_5632*sizeof(rc_str_t));
    lpRCstr5888 = (rc_str_t *)malloc(STR_NUM_5888*sizeof(rc_str_t));
    lpRCstr6144 = (rc_str_t *)malloc(STR_NUM_6144*sizeof(rc_str_t));
    lpRCstr7168 = (rc_str_t *)malloc(STR_NUM_7168*sizeof(rc_str_t));

    for (i=0; i<STR_NUM_2048; i++)
	LoadString(hinstance, 2048+i, lpRCstr2048[i].str, 512);

    for (i=0; i<STR_NUM_4096; i++)
	LoadString(hinstance, 4096+i, lpRCstr4096[i].str, 512);

    for (i=0; i<STR_NUM_4352; i++)
	LoadString(hinstance, 4352+i, lpRCstr4352[i].str, 512);

    for (i=0; i<STR_NUM_4608; i++)
	LoadString(hinstance, 4608+i, lpRCstr4608[i].str, 512);

    for (i=0; i<STR_NUM_5120; i++)
	LoadString(hinstance, 5120+i, lpRCstr5120[i].str, 512);

    for (i=0; i<STR_NUM_5376; i++)
	LoadString(hinstance, 5376+i, lpRCstr5376[i].str, 512);

    for (i=0; i<STR_NUM_5632; i++)
	LoadString(hinstance, 5632+i, lpRCstr5632[i].str, 512);

    for (i=0; i<STR_NUM_5888; i++)
	LoadString(hinstance, 5888+i, lpRCstr5888[i].str, 512);

    for (i=0; i<STR_NUM_6144; i++)
	LoadString(hinstance, 6144+i, lpRCstr6144[i].str, 512);

    for (i=0; i<STR_NUM_7168; i++)
	LoadString(hinstance, 7168+i, lpRCstr7168[i].str, 512);
}


/* Set (or re-set) the language for the application. */
void
set_language(int id)
{
    LCID lcidNew = MAKELCID(id, dwSubLangID);

    if (lang_id != lcidNew) {
	/* Set our new language ID. */
	lang_id = lcidNew;

	SetThreadLocale(lang_id);

	/* Load the strings table for this ID. */
	LoadCommonStrings();

#if 0
	/* Update the menus for this ID. */
	MenuUpdate();
#endif
    }
}


wchar_t *
plat_get_string(int i)
{
    LPTSTR str;

    if ((i >= 2048) && (i <= 3071)) {
	str = lpRCstr2048[i-2048].str;
    } else if ((i >= 4096) && (i <= 4351)) {
	str = lpRCstr4096[i-4096].str;
    } else if ((i >= 4352) && (i <= 4607)) {
	str = lpRCstr4352[i-4352].str;
    } else if ((i >= 4608) && (i <= 5119)) {
	str = lpRCstr4608[i-4608].str;
    } else if ((i >= 5120) && (i <= 5375)) {
	str = lpRCstr5120[i-5120].str;
    } else if ((i >= 5376) && (i <= 5631)) {
	str = lpRCstr5376[i-5376].str;
    } else if ((i >= 5632) && (i <= 5887)) {
	str = lpRCstr5632[i-5632].str;
    } else if ((i >= 5888) && (i <= 6143)) {
	str = lpRCstr5888[i-5888].str;
    } else if ((i >= 6144) && (i <= 7167)) {
	str = lpRCstr6144[i-6144].str;
    } else {
	str = lpRCstr7168[i-7168].str;
    }

    return((wchar_t *)str);
}


#ifndef USE_WX
/* Create a console if we don't already have one. */
static void
CreateConsole(int init)
{
    HANDLE h;
    FILE *fp;
    fpos_t p;
    int i;

    if (! init) {
	FreeConsole();
	return;
    }

    /* Are we logging to a file? */
    p = 0;
    (void)fgetpos(stdout, &p);
    if (p != -1) return;

    /* Not logging to file, attach to console. */
    if (! AttachConsole(ATTACH_PARENT_PROCESS)) {
	/* Parent has no console, create one. */
	if (! AllocConsole()) {
#ifdef _DEBUG
		fp = fopen("error.txt", "w");
		fprintf(fp, "AllocConsole failed: %lu\n", GetLastError());
		fclose(fp);
#endif
		return;
	}
    }

    if ((h = GetStdHandle(STD_OUTPUT_HANDLE)) == NULL) {
#ifdef _DEBUG
	fp = fopen("error.txt", "w");
	fprintf(fp, "GetStdHandle(OUT) failed: %lu\n", GetLastError());
	fclose(fp);
#endif
	return;
    }
    i = _open_osfhandle((intptr_t)h, _O_TEXT);
    if (i != -1) {
	fp = _fdopen(i, "w");
	if (fp == NULL) {
#ifdef _DEBUG
		fp = fopen("error.txt", "w");
		fprintf(fp, "FdOpen(%i) failed: %lu\n", i, GetLastError());
		fclose(fp);
#endif
		return;
	}
	setvbuf(fp, NULL, _IONBF, 1);
	*stdout = *fp;
    } else {
#ifdef _DEBUG
	fp = fopen("error.txt", "w");
	fprintf(fp, "GetOSfHandle(%p) failed: %lu\n", h, GetLastError());
	fclose(fp);
#endif
    }

#if NOTUSED
    h = GetStdHandle(STD_ERROR_HANDLE);
    if (h != NULL) {
	i = _open_osfhandle((intptr_t)h, _O_TEXT);
	if (i != -1) {
		fp = _fdopen(i, "w");
		if (fp != NULL) {
			setvbuf(fp, NULL, _IONBF, 1);
			*stderr = *fp;
		}
	}
    }

    /* Set up stdin as well. */
    h = GetStdHandle(STD_INPUT_HANDLE);
    i = _open_osfhandle((intptr_t)h, _O_TEXT);
    fp = _fdopen(i, "r");
    setvbuf(fp, NULL, _IONBF, 128);
    *stdin = *fp;
#endif
}


/* Process the commandline, and create standard argc/argv array. */
static int
ProcessCommandLine(wchar_t ***argw)
{
    WCHAR *cmdline;
    wchar_t *argbuf;
    wchar_t **args;
    int argc_max;
    int i, q, argc;

    cmdline = GetCommandLine();
    i = wcslen(cmdline) + 1;
    argbuf = (wchar_t *)malloc(sizeof(wchar_t)*i);
    wcscpy(argbuf, cmdline);

    argc = 0;
    argc_max = 64;
    args = (wchar_t **)malloc(sizeof(wchar_t *) * argc_max);
    if (args == NULL) {
	free(argbuf);
	return(0);
    }

    /* parse commandline into argc/argv format */
    i = 0;
    while (argbuf[i]) {
	while (argbuf[i] == L' ')
		  i++;

	if (argbuf[i]) {
		if ((argbuf[i] == L'\'') || (argbuf[i] == L'"')) {
			q = argbuf[i++];
			if (!argbuf[i])
				break;
		} else
			q = 0;

		args[argc++] = &argbuf[i];

		if (argc >= argc_max) {
			argc_max += 64;
			args = realloc(args, sizeof(wchar_t *)*argc_max);
			if (args == NULL) {
				free(argbuf);
				return(0);
			}
		}

		while ((argbuf[i]) && ((q)
			? (argbuf[i]!=q) : (argbuf[i]!=L' '))) i++;

		if (argbuf[i]) {
			argbuf[i] = 0;
			i++;
		}
	}
    }

    args[argc] = NULL;
    *argw = args;

    return(argc);
}


/* For the Windows platform, this is the start of the application. */
int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpszArg, int nCmdShow)
{
    wchar_t **argw = NULL;
    int	argc, i;

    /* Set this to the default value (windowed mode). */
    video_fullscreen = 0;

    /* We need this later. */
    hinstance = hInst;

    /* Initialize the version data. CrashDump needs it early. */
    pc_version("Windows");

#ifdef USE_CRASHDUMP
    /* Enable crash dump services. */
    InitCrashDump();
#endif

    /* First, set our (default) language. */
    set_language(0x0409);

    /* Create console window. */
    CreateConsole(1);

    /* Process the command line for options. */
    argc = ProcessCommandLine(&argw);

    /* Pre-initialize the system, this loads the config file. */
    if (! pc_init(argc, argw)) {
	/* Detach from console. */
	CreateConsole(0);
	return(1);
    }

    /* Cleanup: we may no longer need the console. */
    if (! force_debug)
	CreateConsole(0);

    /* Handle our GUI. */
    i = ui_init(nCmdShow);

    return(i);
}
#endif	/*USE_WX*/


/*
 * We do this here since there is platform-specific stuff
 * going on here, and we do it in a function separate from
 * main() so we can call it from the UI module as well.
 */
void
do_start(void)
{
    LARGE_INTEGER qpc;

    /* We have not stopped yet. */
    quited = 0;

    /* Initialize the high-precision timer. */
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&qpc);
    timer_freq = qpc.QuadPart;
    pclog("Main timer precision: %llu\n", timer_freq);

    /* Start the emulator, really. */
    thMain = thread_create(pc_thread, &quited);
    SetThreadPriority(thMain, THREAD_PRIORITY_HIGHEST);
}


/* Cleanly stop the emulator. */
void
do_stop(void)
{
    quited = 1;

    plat_delay_ms(100);

    pc_close(thMain);

    thMain = NULL;
}


void
plat_get_exe_name(wchar_t *bufp, int size)
{
    GetModuleFileName(hinstance, bufp, size);
}


int
plat_getcwd(wchar_t *bufp, int max)
{
    (void)_wgetcwd(bufp, max);

    return(0);
}


int
plat_chdir(wchar_t *path)
{
    return(_wchdir(path));
}


FILE *
plat_fopen(wchar_t *path, wchar_t *mode)
{
    return(_wfopen(path, mode));
}


void
plat_remove(wchar_t *path)
{
    _wremove(path);
}


/* Make sure a path ends with a trailing (back)slash. */
void
plat_append_slash(wchar_t *path)
{
    if ((path[wcslen(path)-1] != L'\\') &&
	(path[wcslen(path)-1] != L'/')) {
	wcscat(path, L"\\");
    }
}


/* Check if the given path is absolute or not. */
int
plat_path_abs(wchar_t *path)
{
    if ((path[1] == L':') || (path[0] == L'\\') || (path[0] == L'/'))
	return(1);

    return(0);
}


/* Return the last element of a pathname. */
wchar_t *
plat_get_basename(wchar_t *path)
{
    int c = wcslen(path);

    while (c > 0) {
	if (path[c] == L'/' || path[c] == L'\\')
	   return(&path[c]);
       c--;
    }

    return(path);
}


wchar_t *
plat_get_filename(wchar_t *path)
{
    int c = wcslen(path) - 1;

    while (c > 0) {
	if (path[c] == L'/' || path[c] == L'\\')
	   return(&path[c+1]);
       c--;
    }

    return(path);
}


wchar_t *
plat_get_extension(wchar_t *path)
{
    int c = wcslen(path) - 1;

    if (c <= 0)
	return(path);

    while (c && path[c] != L'.')
		c--;

    if (!c)
	return(&path[wcslen(path)]);

    return(&path[c+1]);
}


void
plat_append_filename(wchar_t *dest, wchar_t *s1, wchar_t *s2)
{
    wcscat(dest, s1);
    wcscat(dest, s2);
}


int
plat_dir_check(wchar_t *path)
{
    DWORD dwAttrib = GetFileAttributes(path);

    return(((dwAttrib != INVALID_FILE_ATTRIBUTES &&
	   (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))) ? 1 : 0);
}


int
plat_dir_create(wchar_t *path)
{
    return((int)CreateDirectory(path, NULL));
}


uint64_t
plat_timer_read(void)
{
    LARGE_INTEGER li;

    QueryPerformanceCounter(&li);

    return(li.QuadPart);
}


uint32_t
plat_get_ticks(void)
{
    return(GetTickCount());
}


void
plat_delay_ms(uint32_t count)
{
    Sleep(count);
}


/* Return the VIDAPI number for the given name. */
int
plat_vidapi(char *name)
{
    int i;

    if (!strcasecmp(name, "default") || !strcasecmp(name, "system")) return(1);

    for (i=0; i<4; i++) {
	if (vid_apis[0][i].name &&
	    !strcasecmp(vid_apis[0][i].name, name)) return(i);
    }

    /* Default value. */
    return(1);
}


/* Return the VIDAPI name for the given number. */
char *
plat_vidapi_name(int api)
{
    char *name = "default";

    switch(api) {
#if USE_WX
	case 0:
		break;

	case 1:
		name = "wxwidgets";
		break;
#else
	case 0:
		name = "ddraw";
		break;

	case 1:
#if 0
		/* Direct3D is default. */
		name = "d3d";
#endif
		break;
#endif

#ifdef USE_VNC
	case 2:
		name = "vnc";
		break;

#endif
#ifdef USE_RDP
	case 3:
		name = "rdp";
		break;
#endif
    }

    return(name);
}


int
plat_setvid(int api)
{
    int i;

    pclog("Initializing VIDAPI: api=%d\n", api);
    startblit();
    video_wait_for_blit();

    /* Close the (old) API. */
    vid_apis[0][vid_api].close();
//#ifdef USE_WX
//    ui_check_menu_item(IDM_View_WX+vid_api, 0);
//#endif
    vid_api = api;

#ifndef USE_WX
    if (vid_apis[0][vid_api].local)
	ShowWindow(hwndRender, SW_SHOW);
      else
	ShowWindow(hwndRender, SW_HIDE);
#endif

    /* Initialize the (new) API. */
#ifdef USE_WX
//    ui_check_menu_item(IDM_View_WX+vid_api, 1);
    i = vid_apis[0][vid_api].init(NULL);
#else
    i = vid_apis[0][vid_api].init((void *)hwndRender);
#endif
    endblit();
    if (! i) return(0);

    device_force_redraw();

    return(1);
}


/* Tell the renderers about a new screen resolution. */
void
plat_vidsize(int x, int y)
{
    if (! vid_apis[video_fullscreen][vid_api].resize) return;

    startblit();
    video_wait_for_blit();
    vid_apis[video_fullscreen][vid_api].resize(x, y);
    endblit();
}


int
get_vidpause(void)
{
    return(vid_apis[video_fullscreen][vid_api].pause());
}


void
plat_setfullscreen(int on)
{
    HWND *hw;

    /* Want off and already off? */
    if (!on && !video_fullscreen) return;

    /* Want on and already on? */
    if (on && video_fullscreen) return;

    if (on && video_fullscreen_first) {
	video_fullscreen_first = 0;
	ui_msgbox(MBX_INFO, (wchar_t *)IDS_2107);
    }

    /* OK, claim the video. */
    startblit();
    video_wait_for_blit();

    win_mouse_close();

    /* Close the current mode, and open the new one. */
    vid_apis[video_fullscreen][vid_api].close();
    video_fullscreen = on;
    hw = (video_fullscreen) ? &hwndMain : &hwndRender;
    vid_apis[video_fullscreen][vid_api].init((void *) *hw);

#ifdef USE_WX
    wx_set_fullscreen(on);
#endif

    win_mouse_init();

    /* Release video and make it redraw the screen. */
    endblit();
    device_force_redraw();

    /* Finally, handle the host's mouse cursor. */
    /* pclog("%s full screen, %s cursor\n", on ? "enter" : "leave", on ? "hide" : "show"); */
    show_cursor(video_fullscreen ? 0 : -1);
}


void
take_screenshot(void)
{
    wchar_t path[1024], fn[128];
    struct tm *info;
    time_t now;

    pclog("Screenshot: video API is: %i\n", vid_api);
    if ((vid_api < 0) || (vid_api > 1)) return;

    memset(fn, 0, sizeof(fn));
    memset(path, 0, sizeof(path));

    (void)time(&now);
    info = localtime(&now);

    plat_append_filename(path, usr_path, SCREENSHOT_PATH);

    if (! plat_dir_check(path))
	plat_dir_create(path);

    wcscat(path, L"\\");

    wcsftime(fn, 128, L"%Y%m%d_%H%M%S.png", info);
    wcscat(path, fn);

    switch(vid_api) {
#ifdef USE_WX
	case 0:
	case 1:
		wx_screenshot(path);
		break;
#else
	case 0:		/* ddraw */
		ddraw_take_screenshot(path);
		break;

	case 1:		/* d3d9 */
		d3d_take_screenshot(path);
		break;
#endif

#ifdef USE_VNC
	case 2:		/* vnc */
		vnc_take_screenshot(path);
		break;
#endif
    }
}


void	/* plat_ */
startblit(void)
{
    WaitForSingleObject(ghMutex, INFINITE);
}


void	/* plat_ */
endblit(void)
{
    ReleaseMutex(ghMutex);
}
