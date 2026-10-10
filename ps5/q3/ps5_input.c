/*
===========================================================================
Quake3e for the PlayStation 5 - the pad as Quake's keys, mouse and joystick.
Replaces code/sdl/sdl_input.c.

Copyright (C) 1999-2005 Id Software, Inc. (the interfaces this implements)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
===========================================================================
*/
#include "client/client.h"

#include "platform.h"

/*
The DualSense, translated once, here.

In play each button is one of the engine's gamepad keys (K_PAD0_*), bound like
any key, so the controls menu can show and change what it does. The defaults
(IN_DefaultBinds, set once):

  CROSS, L3 jump        CIRCLE, R3 crouch      SQUARE, R2 fire      TRIANGLE use item
  L1 / R1 previous / next weapon               L2 zoom
  touch pad scores      D-pad walk and turn    left stick left / right: step

  left stick     forward and back are analog (the joystick axis), never rebound
  right stick    look (as mouse movement, so sensitivity, m_yaw, m_pitch and m_filter apply)
  OPTIONS        ESCAPE: the menu
  CIRCLE         ESCAPE too while connecting or loading: backs out

In a menu, the console or a chat line the pad is the keys those expect:

  D-pad arrows          CROSS ENTER            CIRCLE, OPTIONS ESCAPE
  SQUARE BACKSPACE (clears a binding)          R2 MOUSE1 (a click)
  TRIANGLE the console's on-screen keyboard (CROSS on a text field opens it too)
  both sticks    the pointer

While the controls menu waits for a button to bind (the cvar ui_ps5WaitingForKey,
set by ps5/ui/ui_controls2.c) the buttons are their gamepad keys again.
*/

static cvar_t *in_ps5LookSpeed;
static cvar_t *in_ps5MenuSpeed;

static qboolean in_open;
static double in_last;
static float in_remainder_x, in_remainder_y;

enum {
	B_CROSS, B_CIRCLE, B_SQUARE, B_TRIANGLE, B_L1, B_R1, B_L2, B_R2, B_L3, B_R3,
	B_OPTIONS, B_TOUCH, B_UP, B_DOWN, B_LEFT, B_RIGHT, B_STRAFE_LEFT, B_STRAFE_RIGHT,
	B_COUNT
};
// the key each control sent down, so the same key goes up whatever the menu state is by then
static int in_down[B_COUNT];
static int in_axis_forward;


/*
The console's on-screen keyboard (libSceImeDialog), for the text fields a pad
cannot type into: the player's name, a server address, a chat line.
*/
#define SCE_SYSMODULE_IME_DIALOG 0x0096
#define IME_TEXT_LENGTH 63

struct ime_dialog_param {
	int32_t user;
	int32_t type;
	uint64_t languages;
	int32_t enter_label;
	int32_t input_method;
	void *filter;
	uint32_t option;
	uint32_t max_text_length;
	uint16_t *text;
	float x, y;
	int32_t horizontal_alignment, vertical_alignment;
	const uint16_t *placeholder;
	const uint16_t *title;
	int8_t reserved[16];
};
_Static_assert( sizeof( struct ime_dialog_param ) == 96, "the IME dialog's parameters are 96 bytes" );
struct ime_dialog_result {
	int32_t end_status; // 0: confirmed
	int8_t reserved[12];
};

int sceSysmoduleLoadModule( uint32_t id );
int sceUserServiceGetInitialUser( int32_t *user );
int sceImeDialogInit( const struct ime_dialog_param *param, const void *extended );
int sceImeDialogGetStatus( void ); // 1: running, 2: finished
int sceImeDialogGetResult( struct ime_dialog_result *result );
int sceImeDialogTerm( void );

static qboolean ime_open;
static uint16_t ime_text[IME_TEXT_LENGTH + 1];

static void IN_OpenKeyboard( void )
{
	static qboolean loaded;
	static const uint16_t title[] = { 'Q','u','a','k','e',' ','I','I','I',' ','A','r','e','n','a',0 };
	struct ime_dialog_param param;
	int result;

	if ( ime_open )
		return;
	if ( !loaded ) {
		result = sceSysmoduleLoadModule( SCE_SYSMODULE_IME_DIALOG );
		Com_Printf( "PS5 keyboard: module load gave 0x%08x\n", (unsigned)result );
		if ( result < 0 )
			return;
		loaded = qtrue;
	}
	memset( &param, 0, sizeof( param ) );
	memset( ime_text, 0, sizeof( ime_text ) );
	sceUserServiceGetInitialUser( &param.user );
	param.max_text_length = IME_TEXT_LENGTH;
	param.text = ime_text;
	param.x = 960.0f;
	param.y = 540.0f;
	param.horizontal_alignment = 1; // centred
	param.vertical_alignment = 1;
	param.title = title;
	result = sceImeDialogInit( &param, NULL );
	if ( result < 0 ) {
		Com_Printf( "PS5 keyboard: sceImeDialogInit gave 0x%08x\n", (unsigned)result );
		return;
	}
	ime_open = qtrue;
}


// true while the keyboard is up: the pad is the keyboard's then
static qboolean IN_PollKeyboard( void )
{
	struct ime_dialog_result result;
	const int now = Sys_Milliseconds();
	int i;

	if ( !ime_open )
		return qfalse;
	if ( sceImeDialogGetStatus() != 2 )
		return qtrue;

	memset( &result, 0, sizeof( result ) );
	sceImeDialogGetResult( &result );
	sceImeDialogTerm();
	ime_open = qfalse;
	if ( result.end_status == 0 ) {
		// what was typed replaces the field's text: ctrl-c clears a field whole,
		// wherever its cursor is (the menus' fields and the console's alike)
		Sys_QueEvent( now, SE_CHAR, 'c' - 'a' + 1, 0, 0, NULL );
		for ( i = 0; i < IME_TEXT_LENGTH && ime_text[i]; i++ ) {
			if ( ime_text[i] >= ' ' && ime_text[i] < 127 )
				Sys_QueEvent( now, SE_CHAR, ime_text[i], 0, 0, NULL );
		}
	}
	return qfalse;
}


static void IN_Key( int control, qboolean down, int key )
{
	const int now = Sys_Milliseconds();

	if ( down ) {
		if ( in_down[control] )
			return;
		in_down[control] = key;
		Sys_QueEvent( now, SE_KEY, key, qtrue, 0, NULL );
		if ( key == K_ENTER )
			Sys_QueEvent( now, SE_CHAR, '\r', 0, 0, NULL );
	} else if ( in_down[control] ) {
		Sys_QueEvent( now, SE_KEY, in_down[control], qfalse, 0, NULL );
		in_down[control] = 0;
	}
}


static void IN_ReleaseAll( void )
{
	int i;

	for ( i = 0; i < B_COUNT; i++ )
		IN_Key( i, qfalse, 0 );
	if ( in_axis_forward ) {
		in_axis_forward = 0;
		Sys_QueEvent( Sys_Milliseconds(), SE_JOYSTICK_AXIS, AXIS_FORWARD, 0, 0, NULL );
	}
}


void HandleEvents( void )
{
	struct pad pad;
	const double now = now_seconds();
	double dt = now - in_last;
	qboolean menu, binding;
	float lx, ly, rx, ry, speed;
	int forward, dx, dy;

	in_last = now;
	if ( !in_open )
		return;
	if ( dt < 0.0 || dt > 0.1 )
		dt = 0.1;

	pad_poll( &pad );
	if ( IN_PollKeyboard() ) {
		IN_ReleaseAll();
		return;
	}
	if ( pad.held & PAD_INTERCEPTED ) {
		// the shell has the pad (the home screen, a system dialog)
		IN_ReleaseAll();
		return;
	}
	menu = ( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) != 0;
	// the controls menu is waiting for a button to bind: give it the pad's own keys
	binding = menu && Cvar_VariableIntegerValue( "ui_ps5WaitingForKey" ) != 0;
	if ( !menu && Cvar_VariableIntegerValue( "ui_ps5WaitingForKey" ) != 0 )
		Cvar_Set( "ui_ps5WaitingForKey", "0" ); // the menu went away mid-binding

	IN_Key( B_OPTIONS, ( pad.held & PAD_OPTIONS ) != 0, K_ESCAPE );
	if ( menu && !binding ) {
		if ( pad.pressed & PAD_TRIANGLE ) {
			IN_OpenKeyboard();
			return;
		}
		IN_Key( B_CROSS, ( pad.held & PAD_CROSS ) != 0, K_ENTER );
		IN_Key( B_CIRCLE, ( pad.held & PAD_CIRCLE ) != 0, K_ESCAPE );
		IN_Key( B_SQUARE, ( pad.held & PAD_SQUARE ) != 0, K_BACKSPACE );
		IN_Key( B_R2, pad.r2 > 0.5f, K_MOUSE1 );
		IN_Key( B_UP, ( pad.held & PAD_UP ) != 0, K_UPARROW );
		IN_Key( B_DOWN, ( pad.held & PAD_DOWN ) != 0, K_DOWNARROW );
		IN_Key( B_LEFT, ( pad.held & PAD_LEFT ) != 0, K_LEFTARROW );
		IN_Key( B_RIGHT, ( pad.held & PAD_RIGHT ) != 0, K_RIGHTARROW );
		// what play holds down is let go in a menu
		IN_Key( B_TRIANGLE, qfalse, 0 );
		IN_Key( B_L1, qfalse, 0 );
		IN_Key( B_R1, qfalse, 0 );
		IN_Key( B_L2, qfalse, 0 );
		IN_Key( B_L3, qfalse, 0 );
		IN_Key( B_R3, qfalse, 0 );
		IN_Key( B_TOUCH, qfalse, 0 );
	} else {
		// CIRCLE backs out of connecting, downloading or loading (ESCAPE there
		// disconnects); it is its own key once the game is running, and for a binding
		IN_Key( B_CIRCLE, ( pad.held & PAD_CIRCLE ) != 0, !binding && cls.state != CA_ACTIVE ? K_ESCAPE : K_PAD0_B );
		IN_Key( B_CROSS, ( pad.held & PAD_CROSS ) != 0, K_PAD0_A );
		IN_Key( B_SQUARE, ( pad.held & PAD_SQUARE ) != 0, K_PAD0_X );
		IN_Key( B_TRIANGLE, ( pad.held & PAD_TRIANGLE ) != 0, K_PAD0_Y );
		IN_Key( B_L1, ( pad.held & PAD_L1 ) != 0, K_PAD0_LEFTSHOULDER );
		IN_Key( B_R1, ( pad.held & PAD_R1 ) != 0, K_PAD0_RIGHTSHOULDER );
		IN_Key( B_L2, pad.l2 > 0.5f, K_PAD0_LEFTTRIGGER );
		IN_Key( B_R2, pad.r2 > 0.5f, K_PAD0_RIGHTTRIGGER );
		IN_Key( B_L3, ( pad.held & PAD_L3 ) != 0, K_PAD0_LEFTSTICK_CLICK );
		IN_Key( B_R3, ( pad.held & PAD_R3 ) != 0, K_PAD0_RIGHTSTICK_CLICK );
		IN_Key( B_TOUCH, ( pad.held & PAD_TOUCH_PAD ) != 0, K_PAD0_BACK );
		IN_Key( B_UP, ( pad.held & PAD_UP ) != 0, K_PAD0_DPAD_UP );
		IN_Key( B_DOWN, ( pad.held & PAD_DOWN ) != 0, K_PAD0_DPAD_DOWN );
		IN_Key( B_LEFT, ( pad.held & PAD_LEFT ) != 0, K_PAD0_DPAD_LEFT );
		IN_Key( B_RIGHT, ( pad.held & PAD_RIGHT ) != 0, K_PAD0_DPAD_RIGHT );
	}

	// the left stick: walking, analog forward and back, strafing as keys
	lx = pad.left_x;
	ly = pad.left_y;
	IN_Key( B_STRAFE_LEFT, ( !menu || binding ) && lx < -0.4f, K_PAD0_LEFTSTICK_LEFT );
	IN_Key( B_STRAFE_RIGHT, ( !menu || binding ) && lx > 0.4f, K_PAD0_LEFTSTICK_RIGHT );
	forward = menu ? 0 : (int)( -ly * 127.0f );
	if ( forward != in_axis_forward ) {
		in_axis_forward = forward;
		Sys_QueEvent( Sys_Milliseconds(), SE_JOYSTICK_AXIS, AXIS_FORWARD, forward, 0, NULL );
	}

	// the right stick (and the left, in a menu) as mouse movement, faster the further it is pushed
	rx = pad.right_x;
	ry = pad.right_y;
	if ( menu ) {
		rx += lx;
		ry += ly;
		speed = in_ps5MenuSpeed->value;
	} else {
		speed = in_ps5LookSpeed->value;
	}
	rx = Com_Clamp( -1.0f, 1.0f, rx );
	ry = Com_Clamp( -1.0f, 1.0f, ry );
	in_remainder_x += rx * fabsf( rx ) * speed * (float)dt;
	in_remainder_y += ry * fabsf( ry ) * speed * (float)dt;
	dx = (int)in_remainder_x;
	dy = (int)in_remainder_y;
	in_remainder_x -= (float)dx;
	in_remainder_y -= (float)dy;
	if ( dx || dy )
		Sys_QueEvent( Sys_Milliseconds(), SE_MOUSE, dx, dy, 0, NULL );
}


/*
The pad's defaults, each given once (in_ps5Binds remembers how far, so a button
the player unbinds stays unbound and a sensitivity they choose stays chosen).
The keyboard binds of default.cfg stay beside the pad's.
  1: the binds
  2: the look sensitivity for the right stick, 4.45: what was settled on at the
     console (2026-10-10), in place of the engine's 5 for a mouse
*/
#define PS5_BINDS_VERSION 2
#define PS5_SENSITIVITY "4.45"

static void IN_DefaultBinds( void )
{
	static const struct { int key; const char *command; } binds[] = {
		{ K_PAD0_A, "+moveup" },
		{ K_PAD0_B, "+movedown" },
		{ K_PAD0_X, "+attack" },
		{ K_PAD0_Y, "+button2" },
		{ K_PAD0_LEFTSHOULDER, "weapprev" },
		{ K_PAD0_RIGHTSHOULDER, "weapnext" },
		{ K_PAD0_LEFTTRIGGER, "+zoom" },
		{ K_PAD0_RIGHTTRIGGER, "+attack" },
		{ K_PAD0_LEFTSTICK_CLICK, "+moveup" },
		{ K_PAD0_RIGHTSTICK_CLICK, "+movedown" },
		{ K_PAD0_BACK, "+scores" },
		{ K_PAD0_DPAD_UP, "+forward" },
		{ K_PAD0_DPAD_DOWN, "+back" },
		{ K_PAD0_DPAD_LEFT, "+left" },
		{ K_PAD0_DPAD_RIGHT, "+right" },
		{ K_PAD0_LEFTSTICK_LEFT, "+moveleft" },
		{ K_PAD0_LEFTSTICK_RIGHT, "+moveright" },
	};
	cvar_t *done = Cvar_Get( "in_ps5Binds", "0", CVAR_ARCHIVE );
	int i;

	if ( done->integer >= PS5_BINDS_VERSION )
		return;
	if ( done->integer < 1 ) {
		for ( i = 0; i < ARRAY_LEN( binds ); i++ )
			Key_SetBinding( binds[i].key, binds[i].command );
	}
	// only where the engine's own default still stands
	if ( done->integer < 2 && Cvar_VariableValue( "sensitivity" ) == 5.0f )
		Cvar_Set( "sensitivity", PS5_SENSITIVITY );
	Cvar_Set( "in_ps5Binds", va( "%d", PS5_BINDS_VERSION ) );
	Com_Printf( "PS5 pad: defaults set\n" );
}


qboolean Key_CapsLockOn( void )
{
	return qfalse;
}


void IN_Frame( void )
{
}


void IN_Init( void )
{
	in_ps5LookSpeed = Cvar_Get( "in_ps5LookSpeed", "2400", CVAR_ARCHIVE );
	Cvar_SetDescription( in_ps5LookSpeed, "PS5: the right stick at full tilt, in mouse counts a second." );
	in_ps5MenuSpeed = Cvar_Get( "in_ps5MenuSpeed", "500", CVAR_ARCHIVE );
	Cvar_SetDescription( in_ps5MenuSpeed, "PS5: the sticks at full tilt in a menu, in pointer steps a second." );

	if ( !in_open ) {
		// the menus' text fields ask for the keyboard with this (ps5/ui/ui_mfield.c)
		Cmd_AddCommand( "ps5_keyboard", IN_OpenKeyboard );
		// the controls menu sets this while a binding waits for a button
		Cvar_Get( "ui_ps5WaitingForKey", "0", CVAR_TEMP );
		IN_DefaultBinds();
		in_open = pad_open() ? qtrue : qfalse;
		Com_Printf( "PS5 pad: %s\n", in_open ? "open" : "none" );
	}
	in_last = now_seconds();
}


void IN_Shutdown( void )
{
	// the pad stays open across renderer restarts
	if ( in_open )
		IN_ReleaseAll();
}
