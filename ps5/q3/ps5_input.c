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
  the gyro       look too while in_ps5Gyro is on (IN_GyroTurn)
  touch pad      a tap levels the view (the gyro carries on from there), a double tap switches gyro aim
  OPTIONS        ESCAPE: the menu
  CIRCLE         ESCAPE too while connecting or loading: backs out

In a menu, the console or a chat line the pad is the keys those expect:

  D-pad arrows          CROSS ENTER            CIRCLE, OPTIONS ESCAPE
  SQUARE BACKSPACE (clears a binding)          R2 MOUSE1 (a click)
  TRIANGLE the console's on-screen keyboard (CROSS on a text field opens it too)
  both sticks    the pointer
  the gyro       the pointer too, in a menu whether gyro aim is on or not
  touch pad      a tap puts the pointer in the middle, a double tap switches gyro aim

While the controls menu waits for a button to bind (the cvar ui_ps5WaitingForKey,
set by ps5/ui/ui_controls2.c) the buttons are their gamepad keys again.
*/

static cvar_t *in_ps5LookSpeed;
static cvar_t *in_ps5MenuSpeed;
static cvar_t *in_ps5Mouse;
static cvar_t *in_ps5MouseSpeed;
static cvar_t *in_ps5Keyboard;
static cvar_t *in_ps5Gyro;
static cvar_t *in_ps5GyroSpeed;

static qboolean in_open;
static double in_last;
static float in_remainder_x, in_remainder_y;

enum {
	B_CROSS, B_CIRCLE, B_SQUARE, B_TRIANGLE, B_L1, B_R1, B_L2, B_R2, B_L3, B_R3,
	B_OPTIONS, B_TOUCH, B_UP, B_DOWN, B_LEFT, B_RIGHT, B_STRAFE_LEFT, B_STRAFE_RIGHT,
	B_MOUSE1, B_MOUSE2, B_MOUSE3, B_MOUSE4, B_MOUSE5,
	B_KEY_CTRL, B_KEY_SHIFT, B_KEY_ALT, B_KEY_SUPER,
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

static qboolean in_keyboard_connected;
static int in_cross_time;

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


static void IN_KeyboardRelease( void );

static void IN_ReleaseAll( void )
{
	int i;

	for ( i = 0; i < B_COUNT; i++ )
		IN_Key( i, qfalse, 0 );
	IN_KeyboardRelease();
	if ( in_axis_forward ) {
		in_axis_forward = 0;
		Sys_QueEvent( Sys_Milliseconds(), SE_JOYSTICK_AXIS, AXIS_FORWARD, 0, 0, NULL );
	}
}


/*
A mouse, if the console gives the title one: its movement as it comes (the
engine's sensitivity scales it, as it does the right stick's), its buttons
MOUSE1 to MOUSE5 and its wheel, in play and in a menu alike.
*/
static void IN_Mouse( void )
{
	static const uint32_t buttons[] = { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_BUTTON4, MOUSE_BUTTON5 };
	static float mouse_remainder_x, mouse_remainder_y;
	struct mouse mouse;
	const int now = Sys_Milliseconds();
	int i, key, dx, dy;

	mouse_poll( &mouse );
	if ( !in_ps5Mouse->integer )
		memset( &mouse, 0, sizeof( mouse ) );
	for ( i = 0; i < ARRAY_LEN( buttons ); i++ )
		IN_Key( B_MOUSE1 + i, ( mouse.buttons & buttons[i] ) != 0, K_MOUSE1 + i );
	// the mouse's own speed, beside the sensitivities it shares with the stick
	mouse_remainder_x += (float)mouse.dx * in_ps5MouseSpeed->value;
	mouse_remainder_y += (float)mouse.dy * in_ps5MouseSpeed->value;
	dx = (int)mouse_remainder_x;
	dy = (int)mouse_remainder_y;
	mouse_remainder_x -= (float)dx;
	mouse_remainder_y -= (float)dy;
	if ( dx || dy )
		Sys_QueEvent( now, SE_MOUSE, dx, dy, 0, NULL );
	key = mouse.wheel > 0 ? K_MWHEELUP : K_MWHEELDOWN;
	for ( i = abs( mouse.wheel ); i > 0; i-- ) {
		Sys_QueEvent( now, SE_KEY, key, qtrue, 0, NULL );
		Sys_QueEvent( now, SE_KEY, key, qfalse, 0, NULL );
	}
}


/*
A keyboard, if the console gives the title one. Its keys are Quake's own, in
play and in a menu alike, bound by default.cfg as on a desktop; what it types
goes to the console, a chat line and the menus' text fields. The characters
are the US layout's (a UK keyboard's two extra keys type # and \).
*/
#define KEY_REPEAT_DELAY 400
#define KEY_REPEAT_EVERY 33

static int in_key_down[256]; // the key each usage sent down, as in_down
static int in_repeat_usage, in_repeat_time;
static qboolean in_caps_lock;

static int IN_KeyOfUsage( int usage )
{
	static const short keys[] = {
		/* 0x28 */ K_ENTER, K_ESCAPE, K_BACKSPACE, K_TAB, K_SPACE, '-', '=', '[', ']', '\\', '#', ';', '\'', K_CONSOLE, ',', '.', '/',
		/* 0x39 */ K_CAPSLOCK, K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12,
		/* 0x46 */ K_PRINT, K_SCROLLOCK, K_PAUSE, K_INS, K_HOME, K_PGUP, K_DEL, K_END, K_PGDN,
		/* 0x4f */ K_RIGHTARROW, K_LEFTARROW, K_DOWNARROW, K_UPARROW,
		/* 0x53 */ K_KP_NUMLOCK, K_KP_SLASH, K_KP_STAR, K_KP_MINUS, K_KP_PLUS, K_KP_ENTER,
		/* 0x59 */ K_KP_END, K_KP_DOWNARROW, K_KP_PGDN, K_KP_LEFTARROW, K_KP_5, K_KP_RIGHTARROW,
		/* 0x5f */ K_KP_HOME, K_KP_UPARROW, K_KP_PGUP, K_KP_INS, K_KP_DEL, '\\', K_MENU, 0, K_KP_EQUALS,
	};

	if ( usage >= 0x04 && usage <= 0x1d )
		return 'a' + usage - 0x04;
	if ( usage >= 0x1e && usage <= 0x27 )
		return "1234567890"[usage - 0x1e];
	if ( usage >= 0x28 && usage < 0x28 + ARRAY_LEN( keys ) )
		return keys[usage - 0x28];
	return 0;
}


// what a key types, 0 for a key that types nothing
static int IN_CharOfUsage( int usage, int modifiers )
{
	static const char plain[] = "1234567890\r\0\0\0 -=[]\\#;'\0,./";
	static const char shifted[] = "!@#$%^&*()\r\0\0\0 _+{}|~:\"\0<>?";
	static const char keypad[] = "/*-+\r1234567890.";
	const qboolean shift = ( modifiers & ( KEYBOARD_LEFT_SHIFT | KEYBOARD_RIGHT_SHIFT ) ) != 0;
	const qboolean ctrl = ( modifiers & ( KEYBOARD_LEFT_CTRL | KEYBOARD_RIGHT_CTRL ) ) != 0;

	if ( usage >= 0x04 && usage <= 0x1d ) {
		if ( ctrl )
			return usage - 0x04 + 1; // ctrl-a is 1
		return ( shift != in_caps_lock ? 'A' : 'a' ) + usage - 0x04;
	}
	if ( ctrl )
		return 0;
	if ( usage == 0x2a )
		return 'h' - 'a' + 1; // backspace, as the engine's fields take it
	if ( usage >= 0x1e && usage <= 0x38 )
		return ( shift ? shifted : plain )[usage - 0x1e];
	if ( usage >= 0x54 && usage <= 0x63 )
		return keypad[usage - 0x54];
	if ( usage == 0x64 )
		return shift ? '|' : '\\';
	return 0;
}


static void IN_KeyboardRelease( void )
{
	const int now = Sys_Milliseconds();
	int i;

	for ( i = 0; i < ARRAY_LEN( in_key_down ); i++ ) {
		if ( in_key_down[i] ) {
			Sys_QueEvent( now, SE_KEY, in_key_down[i], qfalse, 0, NULL );
			in_key_down[i] = 0;
		}
	}
	in_repeat_usage = 0;
}


static void IN_Keyboard( void )
{
	struct keyboard keyboard;
	const int now = Sys_Milliseconds();
	int i, usage, key, character;

	in_keyboard_connected = keyboard_poll( &keyboard ) && in_ps5Keyboard->integer;
	if ( !in_keyboard_connected ) {
		memset( &keyboard, 0, sizeof( keyboard ) );
		IN_KeyboardRelease();
	}
	IN_Key( B_KEY_CTRL, ( keyboard.modifiers & ( KEYBOARD_LEFT_CTRL | KEYBOARD_RIGHT_CTRL ) ) != 0, K_CTRL );
	IN_Key( B_KEY_SHIFT, ( keyboard.modifiers & ( KEYBOARD_LEFT_SHIFT | KEYBOARD_RIGHT_SHIFT ) ) != 0, K_SHIFT );
	IN_Key( B_KEY_ALT, ( keyboard.modifiers & ( KEYBOARD_LEFT_ALT | KEYBOARD_RIGHT_ALT ) ) != 0, K_ALT );
	IN_Key( B_KEY_SUPER, ( keyboard.modifiers & ( KEYBOARD_LEFT_GUI | KEYBOARD_RIGHT_GUI ) ) != 0, K_SUPER );

	for ( i = 0; i < keyboard.event_count; i++ ) {
		usage = keyboard.events[i].usage;
		if ( !keyboard.events[i].down ) {
			if ( in_key_down[usage] ) {
				Sys_QueEvent( now, SE_KEY, in_key_down[usage], qfalse, 0, NULL );
				in_key_down[usage] = 0;
			}
			if ( in_repeat_usage == usage )
				in_repeat_usage = 0;
			continue;
		}
		key = IN_KeyOfUsage( usage );
		if ( !key || in_key_down[usage] )
			continue;
		in_key_down[usage] = key;
		if ( key == K_CAPSLOCK )
			in_caps_lock = !in_caps_lock;
		Sys_QueEvent( now, SE_KEY, key, qtrue, 0, NULL );
		// the console's key opens and closes the console, and types nothing into it
		character = key == K_CONSOLE ? 0 : IN_CharOfUsage( usage, keyboard.modifiers );
		if ( character )
			Sys_QueEvent( now, SE_CHAR, character, 0, 0, NULL );
		in_repeat_usage = usage;
		in_repeat_time = now + KEY_REPEAT_DELAY;
	}

	// a key held down repeats, as a desktop's does (the engine does not act on a
	// bound key's repeats)
	if ( in_repeat_usage && in_key_down[in_repeat_usage] && now >= in_repeat_time ) {
		in_repeat_time = now + KEY_REPEAT_EVERY;
		Sys_QueEvent( now, SE_KEY, in_key_down[in_repeat_usage], qtrue, 0, NULL );
		character = in_key_down[in_repeat_usage] == K_CONSOLE ? 0 : IN_CharOfUsage( in_repeat_usage, keyboard.modifiers );
		if ( character )
			Sys_QueEvent( now, SE_CHAR, character, 0, 0, NULL );
	}
}


// The menus' text fields ask for the on-screen keyboard at ENTER or a click
// (ps5/ui/ui_mfield.c): with a keyboard plugged in, only the pad's CROSS opens it
static void IN_OpenKeyboard_f( void )
{
	if ( in_keyboard_connected && Sys_Milliseconds() - in_cross_time > 250 )
		return;
	IN_OpenKeyboard();
}


/*
The gyro: while in_ps5Gyro is on, the DualSense's turns in play are mouse
movement that turns the view in_ps5GyroSpeed times as far as the pad turned
(sensitivity, m_yaw and m_pitch are divided out, so the stick's look speeds
leave it alone; zooming slows it as it does the stick). A menu's pointer follows
the pad always, GYRO_MENU_STEPS of the menu's 640 by 480 for each degree, gyro
aim on or off.

The touch pad works the gyro the same in a menu and in play. A tap (a touch
shorter than TAP_US, without a click) is the reset: hold the pad where it is
comfortable and tap, and the pointer goes to the middle of the menu, or the view
levels in play (centerview); the gyro carries on from the pad's place then. A
double tap (the second tap within TAP_GAP_US of the first, which reset already)
is ps5_gyro: gyro aim on or off. ps5_gyro can be bound to a button too (none by
default).

"Player space", as JoyShockMapper turns a pad: the yaw is the turn about the
pad's up as gravity shows it (low-passed, carried through turns by the gyro),
so the view turns the same with the pad held flat or tilted toward the player;
the pitch is the turn about the pad's right-hand axis. Each half second the pad
lies still measures the gyro's bias, and turns slower than GYRO_NOISE_GATE are
scaled toward nothing, so a pad at rest never drifts.
*/
#define GYRO_NOISE_GATE		0.026f	// rad/s (1.5 degrees a second)
#define GYRO_REST_SD		0.0175f	// rad/s (1 degree a second)
#define GYRO_REST_SECONDS	0.5f
#define GYRO_YAW_RELAX		1.41f
#define GYRO_MENU_STEPS		16.0f	// pointer steps a degree (40 degrees across the menu)
#define TAP_US				250000
#define TAP_GAP_US			300000

static struct {
	uint64_t last_us;
	float up[3];
	qboolean up_valid;
	float bias[3];
	int rests;
	// the half second being measured for a rest
	double sum[3], sum_sq[3];
	float accel_min, accel_max, window;
	int samples;
} in_gyro;

static void IN_GyroRestWindow( const float gyro[3], float accel_length, float dt )
{
	qboolean rest;
	float mean[3];
	int k;

	if ( in_gyro.samples == 0 )
		in_gyro.accel_min = in_gyro.accel_max = accel_length;
	for ( k = 0; k < 3; k++ ) {
		in_gyro.sum[k] += gyro[k];
		in_gyro.sum_sq[k] += (double)gyro[k] * gyro[k];
	}
	in_gyro.accel_min = MIN( in_gyro.accel_min, accel_length );
	in_gyro.accel_max = MAX( in_gyro.accel_max, accel_length );
	in_gyro.window += dt;
	in_gyro.samples++;
	if ( in_gyro.window < GYRO_REST_SECONDS || in_gyro.samples < 8 )
		return;

	rest = in_gyro.accel_max - in_gyro.accel_min < 0.05f;
	for ( k = 0; k < 3 && rest; k++ ) {
		const double m = in_gyro.sum[k] / in_gyro.samples;
		const double variance = in_gyro.sum_sq[k] / in_gyro.samples - m * m;
		mean[k] = (float)m;
		rest = sqrt( variance > 0.0 ? variance : 0.0 ) < GYRO_REST_SD;
		// a steady slow turn is not bias: later rests move it by 3 degrees a second at most
		if ( rest && in_gyro.rests )
			rest = fabsf( mean[k] - in_gyro.bias[k] ) < 0.05f;
	}
	if ( rest ) {
		for ( k = 0; k < 3; k++ )
			in_gyro.bias[k] += ( mean[k] - in_gyro.bias[k] ) * ( in_gyro.rests ? 0.3f : 1.0f );
		in_gyro.rests++;
	}
	memset( in_gyro.sum, 0, sizeof( in_gyro.sum ) );
	memset( in_gyro.sum_sq, 0, sizeof( in_gyro.sum_sq ) );
	in_gyro.window = 0.0f;
	in_gyro.samples = 0;
}


static struct {
	qboolean down, clicked;
	uint64_t down_us, up_us;
	int taps;
} in_touch;

// One reading's touch: 1 when it ends a tap, 2 when that tap is the second of a double tap
static int IN_TouchTap( const struct pad_reading *r )
{
	const qboolean touching = r->touches > 0;

	if ( touching && !in_touch.down ) {
		if ( r->timestamp_us - in_touch.up_us > TAP_GAP_US )
			in_touch.taps = 0;
		in_touch.down = qtrue;
		in_touch.clicked = qfalse;
		in_touch.down_us = r->timestamp_us;
	}
	if ( touching && ( r->buttons & PAD_TOUCH_PAD ) )
		in_touch.clicked = qtrue;
	if ( !touching && in_touch.down ) {
		in_touch.down = qfalse;
		in_touch.up_us = r->timestamp_us;
		if ( in_touch.clicked || r->timestamp_us - in_touch.down_us > TAP_US ) {
			in_touch.taps = 0;
		} else if ( ++in_touch.taps == 2 ) {
			in_touch.taps = 0;
			return 2;
		} else {
			return 1;
		}
	}
	return 0;
}


// The pad's turn over the readings of this poll, in degrees: yaw left and pitch up positive,
// and the touch pad's taps (IN_TouchTap's 1 or 2, 0 for none).
// Every poll calls it, so the bias and gravity stay current while the gyro is off.
static void IN_GyroTurn( float *yaw, float *pitch, int *taps )
{
	const struct pad_reading *readings;
	const int count = pad_readings( &readings );
	float rate[3], turn[3], length, dt, world_yaw, horizontal, y, p, speed;
	int i, k, tap;

	*yaw = *pitch = 0.0f;
	*taps = 0;
	for ( i = 0; i < count; i++ ) {
		const struct pad_reading *r = &readings[i];

		// the console repeats a reading between the pad's reports
		if ( !r->connected || ( r->buttons & PAD_INTERCEPTED ) || r->timestamp_us == in_gyro.last_us )
			continue;
		dt = in_gyro.last_us && r->timestamp_us > in_gyro.last_us ? (float)( r->timestamp_us - in_gyro.last_us ) * 1e-6f : 0.0f;
		dt = MIN( dt, 0.02f );
		in_gyro.last_us = r->timestamp_us;
		tap = IN_TouchTap( r );
		if ( tap > *taps )
			*taps = tap;
		length = sqrtf( r->accel[0] * r->accel[0] + r->accel[1] * r->accel[1] + r->accel[2] * r->accel[2] );
		if ( !( length == length ) || !( r->gyro[0] == r->gyro[0] && r->gyro[1] == r->gyro[1] && r->gyro[2] == r->gyro[2] ) )
			continue; // not a number
		for ( k = 0; k < 3; k++ )
			rate[k] = r->gyro[k] - in_gyro.bias[k];

		// up: carried through the turn, then drawn toward the accelerometer near 1 g
		if ( !in_gyro.up_valid ) {
			if ( length > 0.5f ) {
				for ( k = 0; k < 3; k++ )
					in_gyro.up[k] = r->accel[k] / length;
				in_gyro.up_valid = qtrue;
			}
		} else {
			CrossProduct( rate, in_gyro.up, turn );
			for ( k = 0; k < 3; k++ )
				in_gyro.up[k] -= turn[k] * dt;
			if ( length > 0.8f && length < 1.2f ) {
				for ( k = 0; k < 3; k++ )
					in_gyro.up[k] += ( r->accel[k] / length - in_gyro.up[k] ) * ( dt / ( 0.5f + dt ) );
			}
			if ( VectorNormalize( in_gyro.up ) < 1e-3f )
				VectorSet( in_gyro.up, 0.0f, 1.0f, 0.0f );
		}
		IN_GyroRestWindow( r->gyro, length, dt );
		if ( !in_gyro.up_valid || dt <= 0.0f )
			continue;

		world_yaw = rate[1] * in_gyro.up[1] + rate[2] * in_gyro.up[2];
		horizontal = sqrtf( rate[1] * rate[1] + rate[2] * rate[2] );
		y = world_yaw * GYRO_YAW_RELAX;
		y = Com_Clamp( -horizontal, horizontal, y );
		p = rate[0];
		speed = sqrtf( y * y + p * p );
		if ( speed < GYRO_NOISE_GATE ) {
			y *= speed / GYRO_NOISE_GATE;
			p *= speed / GYRO_NOISE_GATE;
		}
		*yaw += y * dt;
		*pitch += p * dt;
	}
	*yaw = RAD2DEG( *yaw );
	*pitch = RAD2DEG( *pitch );
}


static void IN_ToggleGyro_f( void )
{
	Cvar_Set( "in_ps5Gyro", in_ps5Gyro->integer ? "0" : "1" );
	Com_Printf( "Gyro aim %s\n", in_ps5Gyro->integer ? "on" : "off" );
}


// The menu's pointer to the middle of the screen: to its top left corner, then half the
// menu's width (with its widescreen margin, as UI_MouseEvent clamps to it) and height on.
// UI_MouseEvent is called at once, as the event queue would merge the two moves.
static void IN_CenterPointer( void )
{
	const float width = cls.glconfig.vidWidth, height = cls.glconfig.vidHeight;
	int margin = 0;

	if ( !uivm || height <= 0.0f )
		return;
	if ( width * 480.0f > height * 640.0f )
		margin = (int)( 0.5f * ( width - height * ( 640.0f / 480.0f ) ) / ( height / 480.0f ) );
	CL_MouseEvent( -100000, -100000 );
	CL_MouseEvent( 320 + margin, 240 );
}


void HandleEvents( void )
{
	struct pad pad;
	const double now = now_seconds();
	double dt = now - in_last;
	qboolean menu, binding;
	float lx, ly, rx, ry, speed, gyro_yaw, gyro_pitch, sensitivity;
	int forward, dx, dy, taps;

	in_last = now;
	if ( !in_open )
		return;
	if ( dt < 0.0 || dt > 0.1 )
		dt = 0.1;

	pad_poll( &pad );
	IN_GyroTurn( &gyro_yaw, &gyro_pitch, &taps );
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
	if ( binding )
		taps = 0;
	if ( taps == 2 )
		IN_ToggleGyro_f();

	IN_Mouse();
	IN_Keyboard();
	if ( pad.pressed & PAD_CROSS )
		in_cross_time = Sys_Milliseconds();
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
	// the gyro, in the mouse counts that turn the view by its turn times in_ps5GyroSpeed
	// (CL_MouseMove: yaw -= m_yaw * sensitivity * dx, pitch += m_pitch * sensitivity * dy)
	if ( taps && ( Key_GetCatcher() & KEYCATCH_UI ) ) {
		// the reset: the pointer to the middle, and this poll's movement dropped
		in_remainder_x = in_remainder_y = 0.0f;
		IN_CenterPointer();
		return;
	}
	if ( taps && !menu ) {
		// the reset: the view levels, and the gyro turns it from there
		Cbuf_AddText( "centerview\n" );
		gyro_yaw = gyro_pitch = 0.0f;
	}
	sensitivity = Cvar_VariableValue( "sensitivity" );
	if ( Key_GetCatcher() & KEYCATCH_UI ) {
		// a menu's pointer, gyro aim on or off
		in_remainder_x -= gyro_yaw * GYRO_MENU_STEPS;
		in_remainder_y -= gyro_pitch * GYRO_MENU_STEPS;
	} else if ( !menu && in_ps5Gyro->integer && sensitivity > 0.0f ) {
		const float m_yaw = Cvar_VariableValue( "m_yaw" ), m_pitch = Cvar_VariableValue( "m_pitch" );
		if ( m_yaw != 0.0f )
			in_remainder_x -= gyro_yaw * in_ps5GyroSpeed->value / ( m_yaw * sensitivity );
		if ( m_pitch != 0.0f )
			in_remainder_y -= gyro_pitch * in_ps5GyroSpeed->value / ( m_pitch * sensitivity );
	}
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
  3, 4: a test build (3, 2026-10-21) made the D-pad's up center the view and its
     down switch the gyro; 4 gives them forward and back again where it did (the
     touch pad works the gyro now)
*/
#define PS5_BINDS_VERSION 4
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
	if ( done->integer == 3 ) {
		const char *up = Key_GetBinding( K_PAD0_DPAD_UP ), *down = Key_GetBinding( K_PAD0_DPAD_DOWN );
		if ( up && !Q_stricmp( up, "centerview" ) )
			Key_SetBinding( K_PAD0_DPAD_UP, "+forward" );
		if ( down && !Q_stricmp( down, "ps5_gyro" ) )
			Key_SetBinding( K_PAD0_DPAD_DOWN, "+back" );
	}
	Cvar_Set( "in_ps5Binds", va( "%d", PS5_BINDS_VERSION ) );
	Com_Printf( "PS5 pad: defaults set\n" );
}


qboolean Key_CapsLockOn( void )
{
	return in_caps_lock;
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

	in_ps5Mouse = Cvar_Get( "in_ps5Mouse", "1", CVAR_ARCHIVE );
	Cvar_SetDescription( in_ps5Mouse, "PS5: take a USB or Bluetooth mouse's movement, buttons and wheel." );

	in_ps5MouseSpeed = Cvar_Get( "in_ps5MouseSpeed", "1", CVAR_ARCHIVE );
	Cvar_CheckRange( in_ps5MouseSpeed, "0.1", "10", CV_FLOAT );
	Cvar_SetDescription( in_ps5MouseSpeed, "PS5: a mouse's movement is multiplied by this (the right stick's is not)." );
	in_ps5Keyboard = Cvar_Get( "in_ps5Keyboard", "1", CVAR_ARCHIVE );
	Cvar_SetDescription( in_ps5Keyboard, "PS5: take a USB or Bluetooth keyboard's keys." );
	in_ps5Gyro = Cvar_Get( "in_ps5Gyro", "0", CVAR_ARCHIVE );
	Cvar_CheckRange( in_ps5Gyro, "0", "1", CV_INTEGER );
	Cvar_SetDescription( in_ps5Gyro, "PS5: the DualSense's gyro aims too (ps5_gyro switches it)." );
	in_ps5GyroSpeed = Cvar_Get( "in_ps5GyroSpeed", "2", CVAR_ARCHIVE );
	Cvar_CheckRange( in_ps5GyroSpeed, "0.1", "5", CV_FLOAT );
	Cvar_SetDescription( in_ps5GyroSpeed, "PS5: the view turns this many times as far as the DualSense." );

	if ( !in_open ) {
		// the menus' text fields ask for the keyboard with this (ps5/ui/ui_mfield.c)
		Cmd_AddCommand( "ps5_keyboard", IN_OpenKeyboard_f );
		// gyro aim on or off: a double tap on the touch pad, or any button bound to it
		Cmd_AddCommand( "ps5_gyro", IN_ToggleGyro_f );
		// the controls menu sets this while a binding waits for a button
		Cvar_Get( "ui_ps5WaitingForKey", "0", CVAR_TEMP );
		IN_DefaultBinds();
		in_open = pad_open() ? qtrue : qfalse;
		Com_Printf( "PS5 pad: %s\n", in_open ? "open" : "none" );
		Com_Printf( "PS5 mouse: %s\n", mouse_open() ? "open" : "none" );
		Com_Printf( "PS5 keyboard: %s\n", keyboard_open() ? "open" : "none" );
	}
	in_last = now_seconds();
}


void IN_Shutdown( void )
{
	// the pad stays open across renderer restarts
	if ( in_open )
		IN_ReleaseAll();
}
