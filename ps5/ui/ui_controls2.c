/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/*
=======================================================================

CONTROLS MENU

=======================================================================
*/


#include "ui_local.h"

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"


typedef struct {
	char	*command;
	char	*label;
	int		id;
	int		anim;
	int		defaultbind1;
	int		defaultbind2;
	int		bind1;
	int		bind2;
} bind_t;

typedef struct
{
	char*	name;
	float	defaultvalue;
	float	value;	
} configcvar_t;

#define SAVE_NOOP		0
#define SAVE_YES		1
#define SAVE_NO			2
#define SAVE_CANCEL		3

// control sections
#define C_MOVEMENT		0
#define C_LOOKING		1
#define C_WEAPONS		2
#define C_MISC			3
#define C_MAX			4

#define ID_MOVEMENT		100
#define ID_LOOKING		101
#define ID_WEAPONS		102
#define ID_MISC			103
#define ID_DEFAULTS		104
#define ID_BACK			105
#define ID_SAVEANDEXIT	106
#define ID_EXIT			107

// bindable actions
#define ID_SHOWSCORES	0
#define ID_USEITEM		1	
#define ID_SPEED		2	
#define ID_FORWARD		3	
#define ID_BACKPEDAL	4
#define ID_MOVELEFT		5
#define ID_MOVERIGHT	6
#define ID_MOVEUP		7	
#define ID_MOVEDOWN		8
#define ID_LEFT			9	
#define ID_RIGHT		10	
#define ID_STRAFE		11	
#define ID_LOOKUP		12	
#define ID_LOOKDOWN		13
#define ID_MOUSELOOK	14
#define ID_CENTERVIEW	15
#define ID_ZOOMVIEW		16
#define ID_WEAPON1		17	
#define ID_WEAPON2		18	
#define ID_WEAPON3		19	
#define ID_WEAPON4		20	
#define ID_WEAPON5		21	
#define ID_WEAPON6		22	
#define ID_WEAPON7		23	
#define ID_WEAPON8		24	
#define ID_WEAPON9		25	
#define ID_ATTACK		26
#define ID_WEAPPREV		27
#define ID_WEAPNEXT		28
#define ID_GESTURE		29
#define ID_CHAT			30
#define ID_CHAT2		31
#define ID_CHAT3		32
#define ID_CHAT4		33
#define ID_TOGGLEMENU	34
#define ID_PS5GYRO		35	// PS5: g_bindings is indexed by these ids

// all others
#define ID_FREELOOK		36
#define ID_INVERTMOUSE	37
#define ID_ALWAYSRUN	38
#define ID_AUTOSWITCH	39
#define ID_MOUSESPEED	40
#define ID_JOYENABLE	41
#define ID_JOYTHRESHOLD	42
#define ID_SMOOTHMOUSE	43
#define ID_LOOKSPEEDX	44	// PS5
#define ID_LOOKSPEEDY	45	// PS5
#define ID_PS5MOUSESPEED	46	// PS5
#define ID_PS5GYROAIM	47	// PS5
#define ID_PS5GYROSPEED	48	// PS5

#define ANIM_IDLE		0
#define ANIM_RUN		1
#define ANIM_WALK		2
#define ANIM_BACK		3
#define ANIM_JUMP		4
#define ANIM_CROUCH		5
#define ANIM_STEPLEFT	6
#define ANIM_STEPRIGHT	7
#define ANIM_TURNLEFT	8
#define ANIM_TURNRIGHT	9
#define ANIM_LOOKUP		10
#define ANIM_LOOKDOWN	11
#define ANIM_WEAPON1	12
#define ANIM_WEAPON2	13
#define ANIM_WEAPON3	14
#define ANIM_WEAPON4	15
#define ANIM_WEAPON5	16
#define ANIM_WEAPON6	17
#define ANIM_WEAPON7	18
#define ANIM_WEAPON8	19
#define ANIM_WEAPON9	20
#define ANIM_WEAPON10	21
#define ANIM_ATTACK		22
#define ANIM_GESTURE	23
#define ANIM_DIE		24
#define ANIM_CHAT		25

typedef struct
{
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;
	menubitmap_s		player;

	menutext_s			movement;
	menutext_s			looking;
	menutext_s			weapons;
	menutext_s			misc;

	menuaction_s		walkforward;
	menuaction_s		backpedal;
	menuaction_s		stepleft;
	menuaction_s		stepright;
	menuaction_s		moveup;
	menuaction_s		movedown;
	menuaction_s		turnleft;
	menuaction_s		turnright;
	menuaction_s		sidestep;
	menuaction_s		run;
	menuaction_s		machinegun;
	menuaction_s		chainsaw;
	menuaction_s		shotgun;
	menuaction_s		grenadelauncher;
	menuaction_s		rocketlauncher;
	menuaction_s		lightning;
	menuaction_s		railgun;
	menuaction_s		plasma;
	menuaction_s		bfg;
	menuaction_s		attack;
	menuaction_s		prevweapon;
	menuaction_s		nextweapon;
	menuaction_s		lookup;
	menuaction_s		lookdown;
	menuaction_s		mouselook;
	menuradiobutton_s	freelook;
	menuaction_s		centerview;
	menuaction_s		zoomview;
	menuaction_s		gesture;
	menuradiobutton_s	invertmouse;
	menuslider_s		sensitivity;
	menuslider_s		lookspeedx;	// PS5: m_yaw, in tenths of its default
	menuslider_s		lookspeedy;	// PS5: m_pitch, likewise
	menuslider_s		mousespeed;	// PS5: in_ps5MouseSpeed, in tenths
	menuradiobutton_s	gyroaim;	// PS5: in_ps5Gyro
	menuslider_s		gyrospeed;	// PS5: in_ps5GyroSpeed, in tenths
	menuaction_s		gyrotoggle;	// PS5: the ps5_gyro binding
	menuradiobutton_s	smoothmouse;
	menuradiobutton_s	alwaysrun;
	menuaction_s		showscores;
	menuradiobutton_s	autoswitch;
	menuaction_s		useitem;
	playerInfo_t		playerinfo;
	qboolean			changesmade;
	menuaction_s		chat;
	menuaction_s		chat2;
	menuaction_s		chat3;
	menuaction_s		chat4;
	menuaction_s		togglemenu;
	menuradiobutton_s	joyenable;
	menuslider_s		joythreshold;
	int					section;
	qboolean			waitingforkey;
	char				playerModel[64];
	vec3_t				playerViewangles;
	vec3_t				playerMoveangles;
	int					playerLegs;
	int					playerTorso;
	weapon_t			playerWeapon;
	qboolean			playerChat;

	menubitmap_s		back;
	menutext_s			name;
} controls_t; 	

static controls_t s_controls;

/*
PS5: the DualSense's buttons are the engine's gamepad keys (ps5/q3/ps5_input.c).
The numbers are Quake3e's (vendor/Quake3e/code/client/keycodes.h, K_PAD0_*):
ioquake3's keycodes.h, which this module is compiled with, numbers them otherwise.
*/
#define PS5_KEY_FIRST			244
#define PS5_KEY_LAST			275
#define PS5_CROSS				244	// K_PAD0_A
#define PS5_CIRCLE				245	// K_PAD0_B
#define PS5_SQUARE				246	// K_PAD0_X
#define PS5_TRIANGLE			247	// K_PAD0_Y
#define PS5_TOUCHPAD			248	// K_PAD0_BACK
#define PS5_L3					251	// K_PAD0_LEFTSTICK_CLICK
#define PS5_R3					252	// K_PAD0_RIGHTSTICK_CLICK
#define PS5_L1					253	// K_PAD0_LEFTSHOULDER
#define PS5_R1					254	// K_PAD0_RIGHTSHOULDER
#define PS5_DPAD_UP				255
#define PS5_DPAD_DOWN			256
#define PS5_DPAD_LEFT			257
#define PS5_DPAD_RIGHT			258
#define PS5_LSTICK_LEFT			259
#define PS5_LSTICK_RIGHT		260
#define PS5_L2					267	// K_PAD0_LEFTTRIGGER
#define PS5_R2					268	// K_PAD0_RIGHTTRIGGER
// Look sensitivity is two sliders, horizontal and vertical: m_yaw and m_pitch.
// The engine multiplies both by "sensitivity", which this menu holds at
// PS5_SENSITIVITY so that the sliders alone say how fast the stick turns. A slider
// steps by one: a step is a tenth of the engine's 0.022, so 10 is the default
// (with PS5_SENSITIVITY, the feel settled on at the console) and 30 three times it.
#define PS5_SENSITIVITY			4.45f
#define PS5_M_STEP				0.0022f
#define PS5_M_DEFAULT_STEPS		10
#define PS5_M_MAX_STEPS			30
// A mouse's own speed beside the stick's (the engine's in_ps5MouseSpeed, which
// scales a mouse's movement alone): a step is a tenth, 10 a mouse as it comes.
#define PS5_MOUSE_STEP			0.1f
// The gyro's speed (in_ps5GyroSpeed: the view turns that many times as far as the
// pad): a step is a tenth, 20 the default, 50 the most.
#define PS5_GYRO_STEP			0.1f
#define PS5_GYRO_DEFAULT_STEPS	20
#define PS5_GYRO_MAX_STEPS		50

// A key's name as the menu shows it: the console's own name for a pad key
static void Controls_KeyName( int key, char *name, int size )
{
	static const struct { const char *engine; const char *shown; } names[] = {
		{ "PAD0_A", "CROSS" }, { "PAD0_B", "CIRCLE" }, { "PAD0_X", "SQUARE" }, { "PAD0_Y", "TRIANGLE" },
		{ "PAD0_BACK", "TOUCH PAD" }, { "PAD0_START", "OPTIONS" },
		{ "PAD0_LEFTSTICK_CLICK", "L3" }, { "PAD0_RIGHTSTICK_CLICK", "R3" },
		{ "PAD0_LEFTSHOULDER", "L1" }, { "PAD0_RIGHTSHOULDER", "R1" },
		{ "PAD0_LEFTTRIGGER", "L2" }, { "PAD0_RIGHTTRIGGER", "R2" },
		{ "PAD0_DPAD_UP", "D-PAD UP" }, { "PAD0_DPAD_DOWN", "D-PAD DOWN" },
		{ "PAD0_DPAD_LEFT", "D-PAD LEFT" }, { "PAD0_DPAD_RIGHT", "D-PAD RIGHT" },
		{ "PAD0_LEFTSTICK_LEFT", "LEFT STICK LEFT" }, { "PAD0_LEFTSTICK_RIGHT", "LEFT STICK RIGHT" },
		{ "PAD0_LEFTSTICK_UP", "LEFT STICK UP" }, { "PAD0_LEFTSTICK_DOWN", "LEFT STICK DOWN" },
	};
	int i;

	trap_Key_KeynumToStringBuf( key, name, size );
	Q_strupr( name );
	for ( i = 0; i < ARRAY_LEN( names ); i++ ) {
		if ( !strcmp( name, names[i].engine ) ) {
			Q_strncpyz( name, names[i].shown, size );
			return;
		}
	}
}

// A pad key, or a mouse's or a keyboard's
static qboolean Controls_IsPadKey( int key )
{
	return key >= PS5_KEY_FIRST && key <= PS5_KEY_LAST;
}

// The engine sends the pad's own keys, not the menus', while a binding waits for one
static void Controls_SetWaiting( qboolean waiting )
{
	s_controls.waitingforkey = waiting;
	trap_Cvar_Set( "ui_ps5WaitingForKey", waiting ? "1" : "0" );
}

static vec4_t controls_binding_color  = {1.00f, 0.43f, 0.00f, 1.00f};

static bind_t g_bindings[] = 
{
	{"+scores",			"show scores",		ID_SHOWSCORES,	ANIM_IDLE,		PS5_TOUCHPAD,	-1,		-1, -1},
	{"+button2",		"use item",			ID_USEITEM,		ANIM_IDLE,		PS5_TRIANGLE,	-1,		-1, -1},
	{"+speed", 			"run / walk",		ID_SPEED,		ANIM_RUN,		-1,	-1,		-1,	-1},
	{"+forward", 		"walk forward",		ID_FORWARD,		ANIM_WALK,		PS5_DPAD_UP,	-1,		-1, -1},
	{"+back", 			"backpedal",		ID_BACKPEDAL,	ANIM_BACK,		PS5_DPAD_DOWN,	-1,		-1, -1},
	{"+moveleft", 		"step left",		ID_MOVELEFT,	ANIM_STEPLEFT,	PS5_LSTICK_LEFT,	-1,		-1, -1},
	{"+moveright", 		"step right",		ID_MOVERIGHT,	ANIM_STEPRIGHT,	PS5_LSTICK_RIGHT,	-1,		-1, -1},
	{"+moveup",			"up / jump",		ID_MOVEUP,		ANIM_JUMP,		PS5_CROSS,	PS5_L3,		-1, -1},
	{"+movedown",		"down / crouch",	ID_MOVEDOWN,	ANIM_CROUCH,	PS5_CIRCLE,	PS5_R3,		-1, -1},
	{"+left", 			"turn left",		ID_LEFT,		ANIM_TURNLEFT,	PS5_DPAD_LEFT,	-1,		-1, -1},
	{"+right", 			"turn right",		ID_RIGHT,		ANIM_TURNRIGHT,	PS5_DPAD_RIGHT,	-1,		-1, -1},
	{"+strafe", 		"sidestep / turn",	ID_STRAFE,		ANIM_IDLE,		-1,	-1,		-1, -1},
	{"+lookup", 		"look up",			ID_LOOKUP,		ANIM_LOOKUP,	-1,	-1,		-1, -1},
	{"+lookdown", 		"look down",		ID_LOOKDOWN,	ANIM_LOOKDOWN,	-1,	-1,		-1, -1},
	{"+mlook", 			"stick look",		ID_MOUSELOOK,	ANIM_IDLE,		-1,	-1,		-1, -1},
	{"centerview", 		"center view",		ID_CENTERVIEW,	ANIM_IDLE,		-1,	-1,		-1, -1},
	{"+zoom", 			"zoom view",		ID_ZOOMVIEW,	ANIM_IDLE,		PS5_L2,	-1,		-1, -1},
	{"weapon 1",		"gauntlet",			ID_WEAPON1,		ANIM_WEAPON1,	-1,	-1,		-1, -1},
	{"weapon 2",		"machinegun",		ID_WEAPON2,		ANIM_WEAPON2,	-1,	-1,		-1, -1},
	{"weapon 3",		"shotgun",			ID_WEAPON3,		ANIM_WEAPON3,	-1,	-1,		-1, -1},
	{"weapon 4",		"grenade launcher",	ID_WEAPON4,		ANIM_WEAPON4,	-1,	-1,		-1, -1},
	{"weapon 5",		"rocket launcher",	ID_WEAPON5,		ANIM_WEAPON5,	-1,	-1,		-1, -1},
	{"weapon 6",		"lightning",		ID_WEAPON6,		ANIM_WEAPON6,	-1,	-1,		-1, -1},
	{"weapon 7",		"railgun",			ID_WEAPON7,		ANIM_WEAPON7,	-1,	-1,		-1, -1},
	{"weapon 8",		"plasma gun",		ID_WEAPON8,		ANIM_WEAPON8,	-1,	-1,		-1, -1},
	{"weapon 9",		"BFG",				ID_WEAPON9,		ANIM_WEAPON9,	-1,	-1,		-1, -1},
	{"+attack", 		"attack",			ID_ATTACK,		ANIM_ATTACK,	PS5_R2,	PS5_SQUARE,		-1, -1},
	{"weapprev",		"prev weapon",		ID_WEAPPREV,	ANIM_IDLE,		PS5_L1,	-1,		-1, -1},
	{"weapnext", 		"next weapon",		ID_WEAPNEXT,	ANIM_IDLE,		PS5_R1,	-1,		-1, -1},
	{"+button3", 		"gesture",			ID_GESTURE,		ANIM_GESTURE,	-1,	-1,		-1, -1},
	{"messagemode", 	"chat",				ID_CHAT,		ANIM_CHAT,		-1,	-1,		-1, -1},
	{"messagemode2", 	"chat - team",		ID_CHAT2,		ANIM_CHAT,		-1,	-1,		-1, -1},
	{"messagemode3", 	"chat - target",	ID_CHAT3,		ANIM_CHAT,		-1,	-1,		-1, -1},
	{"messagemode4", 	"chat - attacker",	ID_CHAT4,		ANIM_CHAT,		-1,	-1,		-1, -1},
	{"togglemenu", 		"toggle menu",		ID_TOGGLEMENU,	ANIM_IDLE,		-1,	-1,		-1, -1},
	{"ps5_gyro", 		"gyro aim on / off",	ID_PS5GYRO,	ANIM_IDLE,		-1,	-1,		-1, -1},	// and a double tap on the touch pad
	{(char*)NULL,		(char*)NULL,		0,				0,				-1,				-1,		-1,	-1},
};

static configcvar_t g_configcvars[] =
{
	{"cl_run",			0,					0},
	{"m_pitch",			0,					0},
	{"cg_autoswitch",	0,					0},
	{"sensitivity",		0,					0},
	{"m_yaw",			0,					0},
	{"m_filter",		0,					0},
	{"in_ps5MouseSpeed",	0,				0},
	{"in_ps5Gyro",		0,					0},
	{"in_ps5GyroSpeed",	0,					0},
	{"cl_freelook",		0,					0},
	{NULL,				0,					0}
};

static menucommon_s *g_movement_controls[] =
{
	(menucommon_s *)&s_controls.alwaysrun,     
	(menucommon_s *)&s_controls.run,            
	(menucommon_s *)&s_controls.walkforward,
	(menucommon_s *)&s_controls.backpedal,
	(menucommon_s *)&s_controls.stepleft,      
	(menucommon_s *)&s_controls.stepright,     
	(menucommon_s *)&s_controls.moveup,        
	(menucommon_s *)&s_controls.movedown,      
	(menucommon_s *)&s_controls.turnleft,      
	(menucommon_s *)&s_controls.turnright,     
	(menucommon_s *)&s_controls.sidestep,
	NULL
};

static menucommon_s *g_weapons_controls[] = {
	(menucommon_s *)&s_controls.attack,           
	(menucommon_s *)&s_controls.nextweapon,
	(menucommon_s *)&s_controls.prevweapon,
	(menucommon_s *)&s_controls.autoswitch,    
	(menucommon_s *)&s_controls.chainsaw,         
	(menucommon_s *)&s_controls.machinegun,
	(menucommon_s *)&s_controls.shotgun,          
	(menucommon_s *)&s_controls.grenadelauncher,
	(menucommon_s *)&s_controls.rocketlauncher,   
	(menucommon_s *)&s_controls.lightning,   
	(menucommon_s *)&s_controls.railgun,          
	(menucommon_s *)&s_controls.plasma,           
	(menucommon_s *)&s_controls.bfg,              
	NULL,
};

static menucommon_s *g_looking_controls[] = {
	(menucommon_s *)&s_controls.lookspeedx,
	(menucommon_s *)&s_controls.lookspeedy,
	(menucommon_s *)&s_controls.gyroaim,
	(menucommon_s *)&s_controls.gyrospeed,
	(menucommon_s *)&s_controls.gyrotoggle,
	(menucommon_s *)&s_controls.mousespeed,
	(menucommon_s *)&s_controls.smoothmouse,
	(menucommon_s *)&s_controls.invertmouse,
	(menucommon_s *)&s_controls.lookup,
	(menucommon_s *)&s_controls.lookdown,
	(menucommon_s *)&s_controls.mouselook,
	(menucommon_s *)&s_controls.freelook,
	(menucommon_s *)&s_controls.centerview,
	(menucommon_s *)&s_controls.zoomview,
	NULL,
};

static menucommon_s *g_misc_controls[] = {
	(menucommon_s *)&s_controls.showscores, 
	(menucommon_s *)&s_controls.useitem,
	(menucommon_s *)&s_controls.gesture,
	(menucommon_s *)&s_controls.chat,
	(menucommon_s *)&s_controls.chat2,
	(menucommon_s *)&s_controls.chat3,
	(menucommon_s *)&s_controls.chat4,
	NULL,
};

static menucommon_s **g_controls[] = {
	g_movement_controls,
	g_looking_controls,
	g_weapons_controls,
	g_misc_controls,
};

/*
=================
Controls_InitCvars
=================
*/
static void Controls_InitCvars( void )
{
	configcvar_t*	cvarptr;

	cvarptr = g_configcvars;
	for (;;cvarptr++)
	{
		if (!cvarptr->name)
			break;

		// get current value
		cvarptr->value = trap_Cvar_VariableValue( cvarptr->name );

		// get default value
		trap_Cvar_Reset( cvarptr->name );
		cvarptr->defaultvalue = trap_Cvar_VariableValue( cvarptr->name );

		// restore current value
		trap_Cvar_SetValue( cvarptr->name, cvarptr->value );
	}
}

/*
=================
Controls_GetCvarDefault
=================
*/
static float Controls_GetCvarDefault( char* name )
{
	configcvar_t*	cvarptr;

	cvarptr = g_configcvars;
	for (;;cvarptr++)
	{
		if (!cvarptr->name)
			return (0);

		if (!strcmp(cvarptr->name,name))
			break;
	}

	return (cvarptr->defaultvalue);
}

/*
=================
Controls_GetCvarValue
=================
*/
static float Controls_GetCvarValue( char* name )
{
	configcvar_t*	cvarptr;

	cvarptr = g_configcvars;
	for (;;cvarptr++)
	{
		if (!cvarptr->name)
			return (0);

		if (!strcmp(cvarptr->name,name))
			break;
	}

	return (cvarptr->value);
}


/*
=================
Controls_UpdateModel
=================
*/
static void Controls_UpdateModel( int anim ) {
	VectorClear( s_controls.playerViewangles );
	VectorClear( s_controls.playerMoveangles );
	s_controls.playerViewangles[YAW] = 180 - 30;
	s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW];
	s_controls.playerLegs		     = LEGS_IDLE;
	s_controls.playerTorso			 = TORSO_STAND;
	s_controls.playerWeapon			 = WP_NUM_WEAPONS;
	s_controls.playerChat			 = qfalse;

	switch( anim ) {
	case ANIM_RUN:	
		s_controls.playerLegs = LEGS_RUN;
		break;

	case ANIM_WALK:	
		s_controls.playerLegs = LEGS_WALK;
		break;

	case ANIM_BACK:	
		s_controls.playerLegs = LEGS_BACK;
		break;

	case ANIM_JUMP:	
		s_controls.playerLegs = LEGS_JUMP;
		break;

	case ANIM_CROUCH:	
		s_controls.playerLegs = LEGS_IDLECR;
		break;

	case ANIM_TURNLEFT:
		s_controls.playerViewangles[YAW] += 90;
		break;

	case ANIM_TURNRIGHT:
		s_controls.playerViewangles[YAW] -= 90;
		break;

	case ANIM_STEPLEFT:
		s_controls.playerLegs = LEGS_WALK;
		s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW] + 90;
		break;

	case ANIM_STEPRIGHT:
		s_controls.playerLegs = LEGS_WALK;
		s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW] - 90;
		break;

	case ANIM_LOOKUP:
		s_controls.playerViewangles[PITCH] = -45;
		break;

	case ANIM_LOOKDOWN:
		s_controls.playerViewangles[PITCH] = 45;
		break;

	case ANIM_WEAPON1:
		s_controls.playerWeapon = WP_GAUNTLET;
		break;

	case ANIM_WEAPON2:
		s_controls.playerWeapon = WP_MACHINEGUN;
		break;

	case ANIM_WEAPON3:
		s_controls.playerWeapon = WP_SHOTGUN;
		break;

	case ANIM_WEAPON4:
		s_controls.playerWeapon = WP_GRENADE_LAUNCHER;
		break;

	case ANIM_WEAPON5:
		s_controls.playerWeapon = WP_ROCKET_LAUNCHER;
		break;

	case ANIM_WEAPON6:
		s_controls.playerWeapon = WP_LIGHTNING;
		break;

	case ANIM_WEAPON7:
		s_controls.playerWeapon = WP_RAILGUN;
		break;

	case ANIM_WEAPON8:
		s_controls.playerWeapon = WP_PLASMAGUN;
		break;

	case ANIM_WEAPON9:
		s_controls.playerWeapon = WP_BFG;
		break;

	case ANIM_WEAPON10:
		s_controls.playerWeapon = WP_GRAPPLING_HOOK;
		break;

	case ANIM_ATTACK:
		s_controls.playerTorso = TORSO_ATTACK;
		break;

	case ANIM_GESTURE:
		s_controls.playerTorso = TORSO_GESTURE;
		break;

	case ANIM_DIE:
		s_controls.playerLegs = BOTH_DEATH1;
		s_controls.playerTorso = BOTH_DEATH1;
		s_controls.playerWeapon = WP_NONE;
		break;

	case ANIM_CHAT:
		s_controls.playerChat = qtrue;
		break;

	default:
		break;
	}

	UI_PlayerInfo_SetInfo( &s_controls.playerinfo, s_controls.playerLegs, s_controls.playerTorso, s_controls.playerViewangles, s_controls.playerMoveangles, s_controls.playerWeapon, s_controls.playerChat );
}


/*
=================
Controls_Update
=================
*/
static void Controls_Update( void ) {
	int		i;
	int		j;
	int		y;
	menucommon_s	**controls;
	menucommon_s	*control;

	// disable all controls in all groups
	for( i = 0; i < C_MAX; i++ ) {
		controls = g_controls[i];
		for( j = 0;  (control = controls[j]) ; j++ ) {
			control->flags |= (QMF_HIDDEN|QMF_INACTIVE);
		}
	}

	controls = g_controls[s_controls.section];

	// enable controls in active group (and count number of items for vertical centering)
	for( j = 0;  (control = controls[j]) ; j++ ) {
		control->flags &= ~(QMF_GRAYED|QMF_HIDDEN|QMF_INACTIVE);
	}

	// position controls
	y = ( SCREEN_HEIGHT - j * SMALLCHAR_HEIGHT ) / 2;
	for( j = 0;	(control = controls[j]) ; j++, y += SMALLCHAR_HEIGHT ) {
		control->x      = 320;
		control->y      = y;
		control->left   = 320 - 19*SMALLCHAR_WIDTH;
		control->right  = 320 + 21*SMALLCHAR_WIDTH;
		control->top    = y;
		control->bottom = y + SMALLCHAR_HEIGHT;
	}

	if( s_controls.waitingforkey ) {
		// disable everybody
		for( i = 0; i < s_controls.menu.nitems; i++ ) {
			((menucommon_s*)(s_controls.menu.items[i]))->flags |= QMF_GRAYED;
		}

		// enable action item
		((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->flags &= ~QMF_GRAYED;

		// don't gray out player's name
		s_controls.name.generic.flags &= ~QMF_GRAYED;

		return;
	}

	// enable everybody
	for( i = 0; i < s_controls.menu.nitems; i++ ) {
		((menucommon_s*)(s_controls.menu.items[i]))->flags &= ~QMF_GRAYED;
	}

	// makes sure flags are right on the group selection controls
	s_controls.looking.generic.flags  &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.movement.generic.flags &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.weapons.generic.flags  &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.misc.generic.flags     &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);

	s_controls.looking.generic.flags  |= QMF_PULSEIFFOCUS;
	s_controls.movement.generic.flags |= QMF_PULSEIFFOCUS;
	s_controls.weapons.generic.flags  |= QMF_PULSEIFFOCUS;
	s_controls.misc.generic.flags     |= QMF_PULSEIFFOCUS;

	// set buttons
	switch( s_controls.section ) {
	case C_MOVEMENT:
		s_controls.movement.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.movement.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	
	case C_LOOKING:
		s_controls.looking.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.looking.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	
	case C_WEAPONS:
		s_controls.weapons.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.weapons.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;		

	case C_MISC:
		s_controls.misc.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.misc.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	}
}


/*
=================
Controls_DrawKeyBinding
=================
*/
static void Controls_DrawKeyBinding( void *self )
{
	menuaction_s*	a;
	int				x;
	int				y;
	int				b1;
	int				b2;
	qboolean		c;
	char			name[64];
	char			name2[32];
	const char		*touch;

	a = (menuaction_s*) self;

	x =	a->generic.x;
	y = a->generic.y;

	c = (Menu_ItemAtCursor( a->generic.parent ) == a);

	b1 = g_bindings[a->generic.id].bind1;
	if (b1 == -1)
		strcpy(name,"NOT SET");
	else
	{
		Controls_KeyName( b1, name, 32 );

		b2 = g_bindings[a->generic.id].bind2;
		if (b2 != -1)
		{
			Controls_KeyName( b2, name2, 32 );

			strcat( name, " or " );
			strcat( name, name2 );
		}
	}

	// PS5: what the touch pad does beside any button bound (ps5/q3/ps5_input.c)
	touch = a->generic.id == ID_CENTERVIEW ? "TAP" : a->generic.id == ID_PS5GYRO ? "DOUBLE TAP" : NULL;
	if (touch)
	{
		if (b1 == -1)
			Q_strncpyz( name, touch, sizeof(name) );
		else
		{
			Q_strncpyz( name2, name, sizeof(name2) );
			Com_sprintf( name, sizeof(name), "%s or %s", touch, name2 );
		}
	}

	if (c)
	{
		UI_FillRect( a->generic.left, a->generic.top, a->generic.right-a->generic.left+1, a->generic.bottom-a->generic.top+1, listbar_color ); 

		UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, text_color_highlight );
		UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_SMALLFONT|UI_PULSE, text_color_highlight );

		if (s_controls.waitingforkey)
		{
			UI_DrawChar( x, y, '=', UI_CENTER|UI_BLINK|UI_SMALLFONT, text_color_highlight);
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Press a button ... OPTIONS to cancel", UI_SMALLFONT|UI_CENTER|UI_PULSE, colorWhite );
		}
		else
		{
			UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, text_color_highlight);
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.78, "CROSS to change", UI_SMALLFONT|UI_CENTER, colorWhite );
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.82, "SQUARE to clear", UI_SMALLFONT|UI_CENTER, colorWhite );
		}
	}
	else
	{
		if (a->generic.flags & QMF_GRAYED)
		{
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, text_color_disabled );
			UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_SMALLFONT, text_color_disabled );
		}
		else
		{
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_SMALLFONT, controls_binding_color );
			UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_SMALLFONT, controls_binding_color );
		}
	}
}

/*
=================
Controls_StatusBar
=================
*/
static void Controls_StatusBar( void *self )
{
	UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "D-PAD LEFT and RIGHT, or CROSS, to change", UI_SMALLFONT|UI_CENTER, colorWhite );
}


/*
=================
Controls_MenuDraw

PS5: a double tap on the touch pad switches gyro aim while the menu is up; show it
=================
*/
static void Controls_MenuDraw( void )
{
	s_controls.gyroaim.curvalue = trap_Cvar_VariableValue( "in_ps5Gyro" ) != 0;
	Menu_Draw( &s_controls.menu );
}


/*
=================
Controls_DrawPlayer
=================
*/
static void Controls_DrawPlayer( void *self ) {
	menubitmap_s	*b;
	char			buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );
	if ( strcmp( buf, s_controls.playerModel ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_controls.playerinfo, buf );
		strcpy( s_controls.playerModel, buf );
		Controls_UpdateModel( ANIM_IDLE );
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_controls.playerinfo, uis.realtime/2 );
}


/*
=================
Controls_GetKeyAssignment
=================
*/
static void Controls_GetKeyAssignment (char *command, int *twokeys)
{
	int		j;
	char	b[256];
	int		pad[2], other[2];
	int		pads, others;

	twokeys[0] = twokeys[1] = -1;
	pads = others = 0;

	// PS5: a pad key first, and a mouse or keyboard key beside it where the
	// command has both; two of one kind where it has only that kind. The binds
	// that do not fit in the two places are left alone and never shown.
	for ( j = 0; j <= PS5_KEY_LAST; j++ )
	{
		trap_Key_GetBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_stricmp( b, command ) ) {
			if ( Controls_IsPadKey( j ) ) {
				if ( pads < 2 )
					pad[pads++] = j;
			} else if ( others < 2 ) {
				other[others++] = j;
			}
		}
	}
	if ( pads && others ) {
		twokeys[0] = pad[0];
		twokeys[1] = other[0];
	} else {
		for ( j = 0; j < pads; j++ )
			twokeys[j] = pad[j];
		for ( j = 0; j < others; j++ )
			twokeys[j] = other[j];
	}
}

/*
=================
Controls_GetConfig
=================
*/
static void Controls_GetConfig( void )
{
	int		twokeys[2];
	bind_t*	bindptr;

	// put the bindings into a local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (;;bindptr++)
	{
		if (!bindptr->label)
			break;

		Controls_GetKeyAssignment(bindptr->command, twokeys);

		bindptr->bind1 = twokeys[0];
		bindptr->bind2 = twokeys[1];
	}

	s_controls.invertmouse.curvalue  = Controls_GetCvarValue( "m_pitch" ) < 0;
	s_controls.smoothmouse.curvalue  = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "m_filter" ) );
	s_controls.alwaysrun.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_run" ) );
	s_controls.autoswitch.curvalue   = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cg_autoswitch" ) );
	s_controls.lookspeedx.curvalue   = UI_ClampCvar( 1, PS5_M_MAX_STEPS, (int)( fabs( Controls_GetCvarValue( "m_yaw" ) ) * Controls_GetCvarValue( "sensitivity" ) / ( PS5_M_STEP * PS5_SENSITIVITY ) + 0.5f ) );
	s_controls.mousespeed.curvalue   = UI_ClampCvar( 1, PS5_M_MAX_STEPS, (int)( Controls_GetCvarValue( "in_ps5MouseSpeed" ) / PS5_MOUSE_STEP + 0.5f ) );
	s_controls.lookspeedy.curvalue   = UI_ClampCvar( 1, PS5_M_MAX_STEPS, (int)( fabs( Controls_GetCvarValue( "m_pitch" ) ) * Controls_GetCvarValue( "sensitivity" ) / ( PS5_M_STEP * PS5_SENSITIVITY ) + 0.5f ) );
	s_controls.freelook.curvalue     = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_freelook" ) );
	s_controls.gyroaim.curvalue      = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "in_ps5Gyro" ) );
	s_controls.gyrospeed.curvalue    = UI_ClampCvar( 1, PS5_GYRO_MAX_STEPS, (int)( Controls_GetCvarValue( "in_ps5GyroSpeed" ) / PS5_GYRO_STEP + 0.5f ) );
}

/*
=================
Controls_SetConfig
=================
*/
static void Controls_SetConfig( void )
{
	bind_t*	bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (;;bindptr++)
	{
		if (!bindptr->label)
			break;

		if (bindptr->bind1 != -1)
		{	
			trap_Key_SetBinding( bindptr->bind1, bindptr->command );

			if (bindptr->bind2 != -1)
				trap_Key_SetBinding( bindptr->bind2, bindptr->command );
		}
	}

	// PS5: the two look speeds are m_yaw and m_pitch; the sign of m_pitch is the inversion
	trap_Cvar_SetValue( "m_yaw", s_controls.lookspeedx.curvalue * PS5_M_STEP );
	trap_Cvar_SetValue( "in_ps5MouseSpeed", s_controls.mousespeed.curvalue * PS5_MOUSE_STEP );
	if ( s_controls.invertmouse.curvalue )
		trap_Cvar_SetValue( "m_pitch", -s_controls.lookspeedy.curvalue * PS5_M_STEP );
	else
		trap_Cvar_SetValue( "m_pitch", s_controls.lookspeedy.curvalue * PS5_M_STEP );

	trap_Cvar_SetValue( "m_filter", s_controls.smoothmouse.curvalue );
	trap_Cvar_SetValue( "cl_run", s_controls.alwaysrun.curvalue );
	trap_Cvar_SetValue( "cg_autoswitch", s_controls.autoswitch.curvalue );
	trap_Cvar_SetValue( "sensitivity", PS5_SENSITIVITY );
	trap_Cvar_SetValue( "cl_freelook", s_controls.freelook.curvalue );
	trap_Cvar_SetValue( "in_ps5Gyro", s_controls.gyroaim.curvalue );
	trap_Cvar_SetValue( "in_ps5GyroSpeed", s_controls.gyrospeed.curvalue * PS5_GYRO_STEP );
	trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
static void Controls_SetDefaults( void )
{
	bind_t*	bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, set its default binding
	for (;;bindptr++)
	{
		if (!bindptr->label)
			break;

		bindptr->bind1 = bindptr->defaultbind1;
		bindptr->bind2 = bindptr->defaultbind2;
	}

	s_controls.invertmouse.curvalue  = Controls_GetCvarDefault( "m_pitch" ) < 0;
	s_controls.smoothmouse.curvalue  = Controls_GetCvarDefault( "m_filter" );
	s_controls.alwaysrun.curvalue    = Controls_GetCvarDefault( "cl_run" );
	s_controls.autoswitch.curvalue   = Controls_GetCvarDefault( "cg_autoswitch" );
	s_controls.lookspeedx.curvalue   = PS5_M_DEFAULT_STEPS;
	s_controls.lookspeedy.curvalue   = PS5_M_DEFAULT_STEPS;
	s_controls.mousespeed.curvalue   = PS5_M_DEFAULT_STEPS;
	s_controls.freelook.curvalue     = Controls_GetCvarDefault( "cl_freelook" );
	s_controls.gyroaim.curvalue      = Controls_GetCvarDefault( "in_ps5Gyro" );
	s_controls.gyrospeed.curvalue    = PS5_GYRO_DEFAULT_STEPS;
	trap_Cvar_SetValue( "in_ps5Gyro", s_controls.gyroaim.curvalue ); // Controls_MenuDraw shows the cvar
}

/*
=================
Controls_MenuKey
=================
*/
static sfxHandle_t Controls_MenuKey( int key )
{
	int			id;
	qboolean	found;
	bind_t*		bindptr;
	found = qfalse;

	if (!s_controls.waitingforkey)
	{
		switch (key)
		{
			case K_BACKSPACE:
			case K_DEL:
			case K_KP_DEL:
				key = -1;
				break;
		
			case K_MOUSE2:
			case K_ESCAPE:
				if (s_controls.changesmade)
					Controls_SetConfig();
				goto ignorekey;	

			default:
				goto ignorekey;
		}
	}
	else
	{
		if (key & K_CHAR_FLAG)
			goto ignorekey;

		switch (key)
		{
			case K_ESCAPE:
				Controls_SetWaiting( qfalse );
				Controls_Update();
				return (menu_out_sound);
	
			case '`':
				goto ignorekey;
		}
	}

	s_controls.changesmade = qtrue;
	
	if (key != -1)
	{
		// remove from any other bind
		bindptr = g_bindings;
		for (;;bindptr++)
		{
			if (!bindptr->label)	
				break;

			if (bindptr->bind2 == key)
				bindptr->bind2 = -1;

			if (bindptr->bind1 == key)
			{
				bindptr->bind1 = bindptr->bind2;	
				bindptr->bind2 = -1;
			}
		}
	}

	// assign key to local store
	id      = ((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->id;
	bindptr = g_bindings;
	for (;;bindptr++)
	{
		if (!bindptr->label)	
			break;
		
		if (bindptr->id == id)
		{
			found = qtrue;
			if (key == -1)
			{
				if( bindptr->bind1 != -1 ) {
					trap_Key_SetBinding( bindptr->bind1, "" );
					bindptr->bind1 = -1;
				}
				if( bindptr->bind2 != -1 ) {
					trap_Key_SetBinding( bindptr->bind2, "" );
					bindptr->bind2 = -1;
				}
			}
			else if (bindptr->bind1 == -1) {
				bindptr->bind1 = key;
			}
			else if (bindptr->bind1 != key && bindptr->bind2 == -1) {
				bindptr->bind2 = key;
			}
			else if (bindptr->bind1 != key && bindptr->bind2 != key)
			{
				// PS5: both places taken. The key takes the place of one of its own
				// kind (a pad key's, or a mouse or keyboard key's), so a pad's bind
				// and a mouse's stay side by side; the second place failing that.
				if ( Controls_IsPadKey( bindptr->bind2 ) != Controls_IsPadKey( key )
					&& Controls_IsPadKey( bindptr->bind1 ) == Controls_IsPadKey( key ) ) {
					trap_Key_SetBinding( bindptr->bind1, "" );
					bindptr->bind1 = key;
				} else {
					trap_Key_SetBinding( bindptr->bind2, "" );
					bindptr->bind2 = key;
				}
			}						
			break;
		}
	}				
		
	Controls_SetWaiting( qfalse );

	if (found)
	{	
		Controls_Update();
		return (menu_out_sound);
	}

ignorekey:
	return Menu_DefaultKey( &s_controls.menu, key );
}

/*
=================
Controls_ResetDefaults_Action
=================
*/
static void Controls_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}

	s_controls.changesmade = qtrue;
	Controls_SetDefaults();
	Controls_Update();
}

/*
=================
Controls_ResetDefaults_Draw
=================
*/
static void Controls_ResetDefaults_Draw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset all", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "controls to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
}

/*
=================
Controls_MenuEvent
=================
*/
static void Controls_MenuEvent( void* ptr, int event )
{
	switch (((menucommon_s*)ptr)->id)
	{
		case ID_MOVEMENT:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_MOVEMENT; 
				Controls_Update();
			}
			break;

		case ID_LOOKING:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_LOOKING; 
				Controls_Update();
			}
			break;

		case ID_WEAPONS:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_WEAPONS; 
				Controls_Update();
			}
			break;

		case ID_MISC:
			if (event == QM_ACTIVATED)
			{
				s_controls.section = C_MISC; 
				Controls_Update();
			}
			break;

		case ID_DEFAULTS:
			if (event == QM_ACTIVATED)
			{
				UI_ConfirmMenu( "SET TO DEFAULTS?", Controls_ResetDefaults_Draw, Controls_ResetDefaults_Action );
			}
			break;

		case ID_BACK:
			if (event == QM_ACTIVATED)
			{
				if (s_controls.changesmade)
					Controls_SetConfig();
				UI_PopMenu();
			}
			break;

		case ID_SAVEANDEXIT:
			if (event == QM_ACTIVATED)
			{
				Controls_SetConfig();
				UI_PopMenu();
			}
			break;

		case ID_EXIT:
			if (event == QM_ACTIVATED)
			{
				UI_PopMenu();
			}
			break;

		case ID_PS5GYROAIM:
			// PS5: set at once, as a double tap on the touch pad sets it (Controls_MenuDraw)
			if (event == QM_ACTIVATED)
			{
				trap_Cvar_SetValue( "in_ps5Gyro", s_controls.gyroaim.curvalue );
				s_controls.changesmade = qtrue;
			}
			break;

		case ID_FREELOOK:
		case ID_MOUSESPEED:
		case ID_INVERTMOUSE:
		case ID_SMOOTHMOUSE:
		case ID_ALWAYSRUN:
		case ID_AUTOSWITCH:
		case ID_LOOKSPEEDX:
		case ID_LOOKSPEEDY:
		case ID_PS5MOUSESPEED:
		case ID_PS5GYROSPEED:
			if (event == QM_ACTIVATED)
			{
				s_controls.changesmade = qtrue;
			}
			break;		
	}
}

/*
=================
Controls_ActionEvent
=================
*/
static void Controls_ActionEvent( void* ptr, int event )
{
	if (event == QM_LOSTFOCUS)
	{
		Controls_UpdateModel( ANIM_IDLE );
	}
	else if (event == QM_GOTFOCUS)
	{
		Controls_UpdateModel( g_bindings[((menucommon_s*)ptr)->id].anim );
	}
	else if ((event == QM_ACTIVATED) && !s_controls.waitingforkey)
	{
		Controls_SetWaiting( qtrue );
		Controls_Update();
	}
}

/*
=================
Controls_InitModel
=================
*/
static void Controls_InitModel( void )
{
	memset( &s_controls.playerinfo, 0, sizeof(playerInfo_t) );

	UI_PlayerInfo_SetModel( &s_controls.playerinfo, UI_Cvar_VariableString( "model" ) );

	Controls_UpdateModel( ANIM_IDLE );
}

/*
=================
Controls_InitWeapons
=================
*/
static void Controls_InitWeapons( void ) {
	gitem_t *	item;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType != IT_WEAPON ) {
			continue;
		}
		trap_R_RegisterModel( item->world_model[0] );
	}
}

/*
=================
Controls_MenuInit
=================
*/
static void Controls_MenuInit( void )
{
	static char playername[32];

	// zero set all our globals
	memset( &s_controls, 0 ,sizeof(controls_t) );

	Controls_Cache();

	s_controls.menu.key        = Controls_MenuKey;
	s_controls.menu.draw       = Controls_MenuDraw;
	s_controls.menu.wrapAround = qtrue;
	s_controls.menu.fullscreen = qtrue;

	s_controls.banner.generic.type	= MTYPE_BTEXT;
	s_controls.banner.generic.flags	= QMF_CENTER_JUSTIFY;
	s_controls.banner.generic.x		= 320;
	s_controls.banner.generic.y		= 16;
	s_controls.banner.string		= "CONTROLS";
	s_controls.banner.color			= color_white;
	s_controls.banner.style			= UI_CENTER;

	s_controls.framel.generic.type  = MTYPE_BITMAP;
	s_controls.framel.generic.name  = ART_FRAMEL;
	s_controls.framel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_controls.framel.generic.x     = 0;
	s_controls.framel.generic.y     = 78;
	s_controls.framel.width  	    = 256;
	s_controls.framel.height  	    = 329;

	s_controls.framer.generic.type  = MTYPE_BITMAP;
	s_controls.framer.generic.name  = ART_FRAMER;
	s_controls.framer.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_controls.framer.generic.x     = 376;
	s_controls.framer.generic.y     = 76;
	s_controls.framer.width  	    = 256;
	s_controls.framer.height  	    = 334;

	s_controls.looking.generic.type     = MTYPE_PTEXT;
	s_controls.looking.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.looking.generic.id	    = ID_LOOKING;
	s_controls.looking.generic.callback	= Controls_MenuEvent;
	s_controls.looking.generic.x	    = 152;
	s_controls.looking.generic.y	    = 240 - 2 * PROP_HEIGHT;
	s_controls.looking.string			= "LOOK";
	s_controls.looking.style			= UI_RIGHT;
	s_controls.looking.color			= color_red;

	s_controls.movement.generic.type     = MTYPE_PTEXT;
	s_controls.movement.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.movement.generic.id	     = ID_MOVEMENT;
	s_controls.movement.generic.callback = Controls_MenuEvent;
	s_controls.movement.generic.x	     = 152;
	s_controls.movement.generic.y	     = 240 - PROP_HEIGHT;
	s_controls.movement.string			= "MOVE";
	s_controls.movement.style			= UI_RIGHT;
	s_controls.movement.color			= color_red;

	s_controls.weapons.generic.type	    = MTYPE_PTEXT;
	s_controls.weapons.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.weapons.generic.id	    = ID_WEAPONS;
	s_controls.weapons.generic.callback	= Controls_MenuEvent;
	s_controls.weapons.generic.x	    = 152;
	s_controls.weapons.generic.y	    = 240;
	s_controls.weapons.string			= "SHOOT";
	s_controls.weapons.style			= UI_RIGHT;
	s_controls.weapons.color			= color_red;

	s_controls.misc.generic.type	 = MTYPE_PTEXT;
	s_controls.misc.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.misc.generic.id	     = ID_MISC;
	s_controls.misc.generic.callback = Controls_MenuEvent;
	s_controls.misc.generic.x		 = 152;
	s_controls.misc.generic.y		 = 240 + PROP_HEIGHT;
	s_controls.misc.string			= "MISC";
	s_controls.misc.style			= UI_RIGHT;
	s_controls.misc.color			= color_red;

	s_controls.back.generic.type	 = MTYPE_BITMAP;
	s_controls.back.generic.name     = ART_BACK0;
	s_controls.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.back.generic.x		 = 0;
	s_controls.back.generic.y		 = 480-64;
	s_controls.back.generic.id	     = ID_BACK;
	s_controls.back.generic.callback = Controls_MenuEvent;
	s_controls.back.width  		     = 128;
	s_controls.back.height  		 = 64;
	s_controls.back.focuspic         = ART_BACK1;

	s_controls.player.generic.type      = MTYPE_BITMAP;
	s_controls.player.generic.flags     = QMF_INACTIVE;
	s_controls.player.generic.ownerdraw = Controls_DrawPlayer;
	s_controls.player.generic.x	        = 400;
	s_controls.player.generic.y	        = -40;
	s_controls.player.width	            = 32*10;
	s_controls.player.height            = 56*10;

	s_controls.walkforward.generic.type	     = MTYPE_ACTION;
	s_controls.walkforward.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.walkforward.generic.callback  = Controls_ActionEvent;
	s_controls.walkforward.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.walkforward.generic.id 	     = ID_FORWARD;

	s_controls.backpedal.generic.type	   = MTYPE_ACTION;
	s_controls.backpedal.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.backpedal.generic.callback  = Controls_ActionEvent;
	s_controls.backpedal.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.backpedal.generic.id 	   = ID_BACKPEDAL;

	s_controls.stepleft.generic.type	  = MTYPE_ACTION;
	s_controls.stepleft.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepleft.generic.callback  = Controls_ActionEvent;
	s_controls.stepleft.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.stepleft.generic.id 		  = ID_MOVELEFT;

	s_controls.stepright.generic.type	   = MTYPE_ACTION;
	s_controls.stepright.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepright.generic.callback  = Controls_ActionEvent;
	s_controls.stepright.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.stepright.generic.id        = ID_MOVERIGHT;

	s_controls.moveup.generic.type	    = MTYPE_ACTION;
	s_controls.moveup.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.moveup.generic.callback  = Controls_ActionEvent;
	s_controls.moveup.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.moveup.generic.id        = ID_MOVEUP;

	s_controls.movedown.generic.type	  = MTYPE_ACTION;
	s_controls.movedown.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.movedown.generic.callback  = Controls_ActionEvent;
	s_controls.movedown.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.movedown.generic.id        = ID_MOVEDOWN;

	s_controls.turnleft.generic.type	  = MTYPE_ACTION;
	s_controls.turnleft.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.turnleft.generic.callback  = Controls_ActionEvent;
	s_controls.turnleft.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.turnleft.generic.id        = ID_LEFT;

	s_controls.turnright.generic.type	   = MTYPE_ACTION;
	s_controls.turnright.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.turnright.generic.callback  = Controls_ActionEvent;
	s_controls.turnright.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.turnright.generic.id        = ID_RIGHT;

	s_controls.sidestep.generic.type	  = MTYPE_ACTION;
	s_controls.sidestep.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.sidestep.generic.callback  = Controls_ActionEvent;
	s_controls.sidestep.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.sidestep.generic.id        = ID_STRAFE;

	s_controls.run.generic.type	     = MTYPE_ACTION;
	s_controls.run.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.run.generic.callback  = Controls_ActionEvent;
	s_controls.run.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.run.generic.id        = ID_SPEED;

	s_controls.chainsaw.generic.type	  = MTYPE_ACTION;
	s_controls.chainsaw.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chainsaw.generic.callback  = Controls_ActionEvent;
	s_controls.chainsaw.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chainsaw.generic.id        = ID_WEAPON1;

	s_controls.machinegun.generic.type	    = MTYPE_ACTION;
	s_controls.machinegun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.machinegun.generic.callback  = Controls_ActionEvent;
	s_controls.machinegun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.machinegun.generic.id        = ID_WEAPON2;

	s_controls.shotgun.generic.type	     = MTYPE_ACTION;
	s_controls.shotgun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.shotgun.generic.callback  = Controls_ActionEvent;
	s_controls.shotgun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.shotgun.generic.id        = ID_WEAPON3;

	s_controls.grenadelauncher.generic.type	     = MTYPE_ACTION;
	s_controls.grenadelauncher.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.grenadelauncher.generic.callback  = Controls_ActionEvent;
	s_controls.grenadelauncher.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.grenadelauncher.generic.id        = ID_WEAPON4;

	s_controls.rocketlauncher.generic.type	    = MTYPE_ACTION;
	s_controls.rocketlauncher.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.rocketlauncher.generic.callback  = Controls_ActionEvent;
	s_controls.rocketlauncher.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.rocketlauncher.generic.id        = ID_WEAPON5;

	s_controls.lightning.generic.type	   = MTYPE_ACTION;
	s_controls.lightning.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lightning.generic.callback  = Controls_ActionEvent;
	s_controls.lightning.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.lightning.generic.id        = ID_WEAPON6;

	s_controls.railgun.generic.type	     = MTYPE_ACTION;
	s_controls.railgun.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.railgun.generic.callback  = Controls_ActionEvent;
	s_controls.railgun.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.railgun.generic.id        = ID_WEAPON7;

	s_controls.plasma.generic.type	    = MTYPE_ACTION;
	s_controls.plasma.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.plasma.generic.callback  = Controls_ActionEvent;
	s_controls.plasma.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.plasma.generic.id        = ID_WEAPON8;

	s_controls.bfg.generic.type	     = MTYPE_ACTION;
	s_controls.bfg.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.bfg.generic.callback  = Controls_ActionEvent;
	s_controls.bfg.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.bfg.generic.id        = ID_WEAPON9;

	s_controls.attack.generic.type	    = MTYPE_ACTION;
	s_controls.attack.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.attack.generic.callback  = Controls_ActionEvent;
	s_controls.attack.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.attack.generic.id        = ID_ATTACK;

	s_controls.prevweapon.generic.type	    = MTYPE_ACTION;
	s_controls.prevweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.prevweapon.generic.callback  = Controls_ActionEvent;
	s_controls.prevweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.prevweapon.generic.id        = ID_WEAPPREV;

	s_controls.nextweapon.generic.type	    = MTYPE_ACTION;
	s_controls.nextweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.nextweapon.generic.callback  = Controls_ActionEvent;
	s_controls.nextweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.nextweapon.generic.id        = ID_WEAPNEXT;

	s_controls.lookup.generic.type	    = MTYPE_ACTION;
	s_controls.lookup.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lookup.generic.callback  = Controls_ActionEvent;
	s_controls.lookup.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.lookup.generic.id        = ID_LOOKUP;

	s_controls.lookdown.generic.type	  = MTYPE_ACTION;
	s_controls.lookdown.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lookdown.generic.callback  = Controls_ActionEvent;
	s_controls.lookdown.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.lookdown.generic.id        = ID_LOOKDOWN;

	s_controls.mouselook.generic.type	   = MTYPE_ACTION;
	s_controls.mouselook.generic.flags     = QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.mouselook.generic.callback  = Controls_ActionEvent;
	s_controls.mouselook.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.mouselook.generic.id        = ID_MOUSELOOK;

	s_controls.freelook.generic.type		= MTYPE_RADIOBUTTON;
	s_controls.freelook.generic.flags		= QMF_SMALLFONT;
	s_controls.freelook.generic.x			= SCREEN_WIDTH/2;
	s_controls.freelook.generic.name		= "free look";
	s_controls.freelook.generic.id			= ID_FREELOOK;
	s_controls.freelook.generic.callback	= Controls_MenuEvent;
	s_controls.freelook.generic.statusbar	= Controls_StatusBar;

	s_controls.centerview.generic.type	    = MTYPE_ACTION;
	s_controls.centerview.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.centerview.generic.callback  = Controls_ActionEvent;
	s_controls.centerview.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.centerview.generic.id        = ID_CENTERVIEW;

	s_controls.zoomview.generic.type	  = MTYPE_ACTION;
	s_controls.zoomview.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.zoomview.generic.callback  = Controls_ActionEvent;
	s_controls.zoomview.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.zoomview.generic.id        = ID_ZOOMVIEW;

	s_controls.useitem.generic.type	     = MTYPE_ACTION;
	s_controls.useitem.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.useitem.generic.callback  = Controls_ActionEvent;
	s_controls.useitem.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.useitem.generic.id        = ID_USEITEM;

	s_controls.showscores.generic.type	    = MTYPE_ACTION;
	s_controls.showscores.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.showscores.generic.callback  = Controls_ActionEvent;
	s_controls.showscores.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.showscores.generic.id        = ID_SHOWSCORES;

	s_controls.invertmouse.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.invertmouse.generic.flags	 = QMF_SMALLFONT;
	s_controls.invertmouse.generic.x	     = SCREEN_WIDTH/2;
	s_controls.invertmouse.generic.name	     = "invert look";
	s_controls.invertmouse.generic.id        = ID_INVERTMOUSE;
	s_controls.invertmouse.generic.callback  = Controls_MenuEvent;
	s_controls.invertmouse.generic.statusbar = Controls_StatusBar;

	s_controls.smoothmouse.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.smoothmouse.generic.flags	 = QMF_SMALLFONT;
	s_controls.smoothmouse.generic.x	     = SCREEN_WIDTH/2;
	s_controls.smoothmouse.generic.name	     = "smooth look";
	s_controls.smoothmouse.generic.id        = ID_SMOOTHMOUSE;
	s_controls.smoothmouse.generic.callback  = Controls_MenuEvent;
	s_controls.smoothmouse.generic.statusbar = Controls_StatusBar;

	s_controls.alwaysrun.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.alwaysrun.generic.flags	   = QMF_SMALLFONT;
	s_controls.alwaysrun.generic.x	       = SCREEN_WIDTH/2;
	s_controls.alwaysrun.generic.name	   = "always run";
	s_controls.alwaysrun.generic.id        = ID_ALWAYSRUN;
	s_controls.alwaysrun.generic.callback  = Controls_MenuEvent;
	s_controls.alwaysrun.generic.statusbar = Controls_StatusBar;

	s_controls.autoswitch.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.autoswitch.generic.flags	    = QMF_SMALLFONT;
	s_controls.autoswitch.generic.x	        = SCREEN_WIDTH/2;
	s_controls.autoswitch.generic.name	    = "autoswitch weapons";
	s_controls.autoswitch.generic.id        = ID_AUTOSWITCH;
	s_controls.autoswitch.generic.callback  = Controls_MenuEvent;
	s_controls.autoswitch.generic.statusbar = Controls_StatusBar;

	s_controls.sensitivity.generic.type	     = MTYPE_SLIDER;
	s_controls.sensitivity.generic.x		 = SCREEN_WIDTH/2;
	s_controls.sensitivity.generic.flags	 = QMF_SMALLFONT;
	s_controls.sensitivity.generic.name	     = "look sensitivity";
	s_controls.sensitivity.generic.id 	     = ID_MOUSESPEED;
	s_controls.sensitivity.generic.callback  = Controls_MenuEvent;
	s_controls.sensitivity.minvalue		     = 2;
	s_controls.sensitivity.maxvalue		     = 30;
	s_controls.sensitivity.generic.statusbar = Controls_StatusBar;

	s_controls.lookspeedx.generic.type	     = MTYPE_SLIDER;
	s_controls.lookspeedx.generic.x		     = SCREEN_WIDTH/2;
	s_controls.lookspeedx.generic.flags	     = QMF_SMALLFONT;
	s_controls.lookspeedx.generic.name	     = "horizontal sensitivity";
	s_controls.lookspeedx.generic.id 	     = ID_LOOKSPEEDX;
	s_controls.lookspeedx.generic.callback   = Controls_MenuEvent;
	s_controls.lookspeedx.minvalue		     = 1;
	s_controls.lookspeedx.maxvalue		     = PS5_M_MAX_STEPS;
	s_controls.lookspeedx.generic.statusbar  = Controls_StatusBar;

	s_controls.lookspeedy.generic.type	     = MTYPE_SLIDER;
	s_controls.lookspeedy.generic.x		     = SCREEN_WIDTH/2;
	s_controls.lookspeedy.generic.flags	     = QMF_SMALLFONT;
	s_controls.lookspeedy.generic.name	     = "vertical sensitivity";
	s_controls.lookspeedy.generic.id 	     = ID_LOOKSPEEDY;
	s_controls.lookspeedy.generic.callback   = Controls_MenuEvent;
	s_controls.lookspeedy.minvalue		     = 1;
	s_controls.lookspeedy.maxvalue		     = PS5_M_MAX_STEPS;
	s_controls.lookspeedy.generic.statusbar  = Controls_StatusBar;

	s_controls.mousespeed.generic.type	     = MTYPE_SLIDER;
	s_controls.mousespeed.generic.x		     = SCREEN_WIDTH/2;
	s_controls.mousespeed.generic.flags	     = QMF_SMALLFONT;
	s_controls.mousespeed.generic.name	     = "mouse speed";
	s_controls.mousespeed.generic.id 	     = ID_PS5MOUSESPEED;
	s_controls.mousespeed.generic.callback   = Controls_MenuEvent;
	s_controls.mousespeed.minvalue		     = 1;
	s_controls.mousespeed.maxvalue		     = PS5_M_MAX_STEPS;
	s_controls.mousespeed.generic.statusbar  = Controls_StatusBar;

	s_controls.gyroaim.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.gyroaim.generic.flags	 = QMF_SMALLFONT;
	s_controls.gyroaim.generic.x	     = SCREEN_WIDTH/2;
	s_controls.gyroaim.generic.name	     = "gyro aim";
	s_controls.gyroaim.generic.id        = ID_PS5GYROAIM;
	s_controls.gyroaim.generic.callback  = Controls_MenuEvent;
	s_controls.gyroaim.generic.statusbar = Controls_StatusBar;

	s_controls.gyrospeed.generic.type	     = MTYPE_SLIDER;
	s_controls.gyrospeed.generic.x		     = SCREEN_WIDTH/2;
	s_controls.gyrospeed.generic.flags	     = QMF_SMALLFONT;
	s_controls.gyrospeed.generic.name	     = "gyro speed";
	s_controls.gyrospeed.generic.id 	     = ID_PS5GYROSPEED;
	s_controls.gyrospeed.generic.callback    = Controls_MenuEvent;
	s_controls.gyrospeed.minvalue		     = 1;
	s_controls.gyrospeed.maxvalue		     = PS5_GYRO_MAX_STEPS;
	s_controls.gyrospeed.generic.statusbar   = Controls_StatusBar;

	s_controls.gyrotoggle.generic.type	    = MTYPE_ACTION;
	s_controls.gyrotoggle.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.gyrotoggle.generic.callback  = Controls_ActionEvent;
	s_controls.gyrotoggle.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.gyrotoggle.generic.id        = ID_PS5GYRO;

	s_controls.gesture.generic.type	     = MTYPE_ACTION;
	s_controls.gesture.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.gesture.generic.callback  = Controls_ActionEvent;
	s_controls.gesture.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.gesture.generic.id        = ID_GESTURE;

	s_controls.chat.generic.type	  = MTYPE_ACTION;
	s_controls.chat.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat.generic.callback  = Controls_ActionEvent;
	s_controls.chat.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat.generic.id        = ID_CHAT;

	s_controls.chat2.generic.type	   = MTYPE_ACTION;
	s_controls.chat2.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat2.generic.callback  = Controls_ActionEvent;
	s_controls.chat2.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat2.generic.id        = ID_CHAT2;

	s_controls.chat3.generic.type	   = MTYPE_ACTION;
	s_controls.chat3.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat3.generic.callback  = Controls_ActionEvent;
	s_controls.chat3.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat3.generic.id        = ID_CHAT3;

	s_controls.chat4.generic.type	   = MTYPE_ACTION;
	s_controls.chat4.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat4.generic.callback  = Controls_ActionEvent;
	s_controls.chat4.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat4.generic.id        = ID_CHAT4;

	s_controls.togglemenu.generic.type		= MTYPE_ACTION;
	s_controls.togglemenu.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.togglemenu.generic.callback  = Controls_ActionEvent;
	s_controls.togglemenu.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.togglemenu.generic.id        = ID_TOGGLEMENU;

	s_controls.joyenable.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.joyenable.generic.flags	   = QMF_SMALLFONT;
	s_controls.joyenable.generic.x	       = SCREEN_WIDTH/2;
	s_controls.joyenable.generic.name	   = "joystick";
	s_controls.joyenable.generic.id        = ID_JOYENABLE;
	s_controls.joyenable.generic.callback  = Controls_MenuEvent;
	s_controls.joyenable.generic.statusbar = Controls_StatusBar;

	s_controls.joythreshold.generic.type	  = MTYPE_SLIDER;
	s_controls.joythreshold.generic.x		  = SCREEN_WIDTH/2;
	s_controls.joythreshold.generic.flags	  = QMF_SMALLFONT;
	s_controls.joythreshold.generic.name	  = "joystick threshold";
	s_controls.joythreshold.generic.id 	      = ID_JOYTHRESHOLD;
	s_controls.joythreshold.generic.callback  = Controls_MenuEvent;
	s_controls.joythreshold.minvalue		  = 0.05f;
	s_controls.joythreshold.maxvalue		  = 0.75f;
	s_controls.joythreshold.generic.statusbar = Controls_StatusBar;

	s_controls.name.generic.type	= MTYPE_PTEXT;
	s_controls.name.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_controls.name.generic.x		= 320;
	s_controls.name.generic.y		= 440;
	s_controls.name.string			= playername;
	s_controls.name.style			= UI_CENTER;
	s_controls.name.color			= text_color_normal;

	Menu_AddItem( &s_controls.menu, &s_controls.banner );
	Menu_AddItem( &s_controls.menu, &s_controls.framel );
	Menu_AddItem( &s_controls.menu, &s_controls.framer );
	Menu_AddItem( &s_controls.menu, &s_controls.player );
	Menu_AddItem( &s_controls.menu, &s_controls.name );

	Menu_AddItem( &s_controls.menu, &s_controls.looking );
	Menu_AddItem( &s_controls.menu, &s_controls.movement );
	Menu_AddItem( &s_controls.menu, &s_controls.weapons );
	Menu_AddItem( &s_controls.menu, &s_controls.misc );

	Menu_AddItem( &s_controls.menu, &s_controls.lookspeedx );
	Menu_AddItem( &s_controls.menu, &s_controls.lookspeedy );
	Menu_AddItem( &s_controls.menu, &s_controls.gyroaim );
	Menu_AddItem( &s_controls.menu, &s_controls.gyrospeed );
	Menu_AddItem( &s_controls.menu, &s_controls.gyrotoggle );
	Menu_AddItem( &s_controls.menu, &s_controls.mousespeed );
	Menu_AddItem( &s_controls.menu, &s_controls.smoothmouse );
	Menu_AddItem( &s_controls.menu, &s_controls.invertmouse );
	Menu_AddItem( &s_controls.menu, &s_controls.lookup );
	Menu_AddItem( &s_controls.menu, &s_controls.lookdown );
	Menu_AddItem( &s_controls.menu, &s_controls.mouselook );
	Menu_AddItem( &s_controls.menu, &s_controls.freelook );
	Menu_AddItem( &s_controls.menu, &s_controls.centerview );
	Menu_AddItem( &s_controls.menu, &s_controls.zoomview );

	Menu_AddItem( &s_controls.menu, &s_controls.alwaysrun );
	Menu_AddItem( &s_controls.menu, &s_controls.run );
	Menu_AddItem( &s_controls.menu, &s_controls.walkforward );
	Menu_AddItem( &s_controls.menu, &s_controls.backpedal );
	Menu_AddItem( &s_controls.menu, &s_controls.stepleft );
	Menu_AddItem( &s_controls.menu, &s_controls.stepright );
	Menu_AddItem( &s_controls.menu, &s_controls.moveup );
	Menu_AddItem( &s_controls.menu, &s_controls.movedown );
	Menu_AddItem( &s_controls.menu, &s_controls.turnleft );
	Menu_AddItem( &s_controls.menu, &s_controls.turnright );
	Menu_AddItem( &s_controls.menu, &s_controls.sidestep );

	Menu_AddItem( &s_controls.menu, &s_controls.attack );
	Menu_AddItem( &s_controls.menu, &s_controls.nextweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.prevweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.autoswitch );
	Menu_AddItem( &s_controls.menu, &s_controls.chainsaw );
	Menu_AddItem( &s_controls.menu, &s_controls.machinegun );
	Menu_AddItem( &s_controls.menu, &s_controls.shotgun );
	Menu_AddItem( &s_controls.menu, &s_controls.grenadelauncher );
	Menu_AddItem( &s_controls.menu, &s_controls.rocketlauncher );
	Menu_AddItem( &s_controls.menu, &s_controls.lightning );
	Menu_AddItem( &s_controls.menu, &s_controls.railgun );
	Menu_AddItem( &s_controls.menu, &s_controls.plasma );
	Menu_AddItem( &s_controls.menu, &s_controls.bfg );

	Menu_AddItem( &s_controls.menu, &s_controls.showscores );
	Menu_AddItem( &s_controls.menu, &s_controls.useitem );
	Menu_AddItem( &s_controls.menu, &s_controls.gesture );
	Menu_AddItem( &s_controls.menu, &s_controls.chat );
	Menu_AddItem( &s_controls.menu, &s_controls.chat2 );
	Menu_AddItem( &s_controls.menu, &s_controls.chat3 );
	Menu_AddItem( &s_controls.menu, &s_controls.chat4 );

	Menu_AddItem( &s_controls.menu, &s_controls.back );

	trap_Cvar_VariableStringBuffer( "name", s_controls.name.string, 16 );
	Q_CleanStr( s_controls.name.string );

	// initialize the configurable cvars
	Controls_InitCvars();

	// initialize the current config
	Controls_GetConfig();

	// intialize the model
	Controls_InitModel();

	// intialize the weapons
	Controls_InitWeapons ();

	// initial default section
	s_controls.section = C_LOOKING;

	// update the ui
	Controls_Update();
}


/*
=================
Controls_Cache
=================
*/
void Controls_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
=================
UI_ControlsMenu
=================
*/
void UI_ControlsMenu( void ) {
	Controls_MenuInit();
	UI_PushMenu( &s_controls.menu );
}
