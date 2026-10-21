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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

static qboolean	scr_initialized;		// ready to draw

cvar_t		*cl_timegraph;
static cvar_t		*cl_debuggraph;
static cvar_t		*cl_graphheight;
static cvar_t		*cl_graphscale;
static cvar_t		*cl_graphshift;

/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	assert( width != 0 );

	hShader = re.RegisterShader( picname );
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	float	xscale;
	float	yscale;

#if 0
		// adjust for wide screens
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
			*x += 0.5 * ( cls.glconfig.vidWidth - ( cls.glconfig.vidHeight * 640 / 480 ) );
		}
#endif

	// scale for screen sizes
	xscale = cls.glconfig.vidWidth / 640.0;
	yscale = cls.glconfig.vidHeight / 480.0;
	if ( x ) {
		*x *= xscale;
	}
	if ( y ) {
		*y *= yscale;
	}
	if ( w ) {
		*w *= xscale;
	}
	if ( h ) {
		*h *= yscale;
	}
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( float x, float y, float width, float height, const float *color ) {
	re.SetColor( color );

	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );

	re.SetColor( NULL );
}


/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar( int x, int y, float size, int ch ) {
	int row, col;
	float frow, fcol;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -size ) {
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;
	SCR_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	re.DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}


/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar( int x, int y, int ch ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -smallchar_height ) {
		return;
	}

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	re.DrawStretchPic( x, y, smallchar_width, smallchar_height,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}


/*
** SCR_DrawSmallString
** small string are drawn at native screen resolution
*/
void SCR_DrawSmallString( int x, int y, const char *s, int len ) {
	int row, col, ch, i;
	float frow, fcol;
	float size;

	if ( y < -smallchar_height ) {
		return;
	}

	size = 0.0625;

	for ( i = 0; i < len; i++ ) {
		ch = *s++ & 255;
		row = ch>>4;
		col = ch&15;

		frow = row*0.0625;
		fcol = col*0.0625;

		re.DrawStretchPic( x, y, smallchar_width, smallchar_height,
						   fcol, frow, fcol + size, frow + size, 
						   cls.charSetShader );

		x += smallchar_width;
	}
}


/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt( int x, int y, float size, const char *string, const float *setColor, qboolean forceColor,
		qboolean noColorEscape ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0.0;
	color[3] = setColor[3];
	re.SetColor( color );
	s = string;
	xx = x;
	while ( *s ) {
		if ( !noColorEscape && Q_IsColorString( s ) ) {
			s += 2;
			continue;
		}
		SCR_DrawChar( xx+2, y+2, size, *s );
		xx += size;
		s++;
	}


	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ ColorIndexFromChar( *(s+1) ) ], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			if ( !noColorEscape ) {
				s += 2;
				continue;
			}
		}
		SCR_DrawChar( xx, y, size, *s );
		xx += size;
		s++;
	}
	re.SetColor( NULL );
}


/*
==================
SCR_DrawBigString
==================
*/
void SCR_DrawBigString( int x, int y, const char *s, float alpha, qboolean noColorEscape ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qfalse, noColorEscape );
}


/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.
==================
*/
void SCR_DrawSmallStringExt( int x, int y, const char *string, const float *setColor, qboolean forceColor,
		qboolean noColorEscape ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ ColorIndexFromChar( *(s+1) ) ], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			if ( !noColorEscape ) {
				s += 2;
				continue;
			}
		}
		SCR_DrawSmallChar( xx, y, *s );
		xx += smallchar_width;
		s++;
	}
	re.SetColor( NULL );
}


/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}


/*
** SCR_GetBigStringWidth
*/ 
int SCR_GetBigStringWidth( const char *str ) {
	return SCR_Strlen( str ) * BIGCHAR_WIDTH;
}


//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
static void SCR_DrawDemoRecording( void ) {
	char	string[sizeof(clc.recordNameShort)+32];
	int		pos;

	if ( !clc.demorecording ) {
		return;
	}
	if ( clc.spDemoRecording ) {
		return;
	}

	pos = FS_FTell( clc.recordfile );

	if (cl_drawRecording->integer == 1) {
		sprintf(string, "RECORDING %s: %ik", clc.recordNameShort, pos / 1024);
		SCR_DrawStringExt(320 - strlen(string) * 4, 20, 8, string, g_color_table[ColorIndex(COLOR_WHITE)], qtrue, qfalse);
	} else if (cl_drawRecording->integer == 2) {
		sprintf(string, "RECORDING: %ik", pos / 1024);
		SCR_DrawStringExt(320 - strlen(string) * 4, 20, 8, string, g_color_table[ColorIndex(COLOR_WHITE)], qtrue, qfalse);
	}
}


#ifdef USE_VOIP
/*
=================
SCR_DrawVoipMeter
=================
*/
static void SCR_DrawVoipMeter( void ) {
	char	buffer[16];
	char	string[256];
	int limit, i;

	if (!cl_voipShowMeter->integer)
		return;  // player doesn't want to show meter at all.
	else if (!cl_voipSend->integer)
		return;  // not recording at the moment.
	else if (clc.state != CA_ACTIVE)
		return;  // not connected to a server.
	else if (!clc.voipEnabled)
		return;  // server doesn't support VoIP.
	else if (clc.demoplaying)
		return;  // playing back a demo.
	else if (!cl_voip->integer)
		return;  // client has VoIP support disabled.

	limit = (int) (clc.voipPower * 10.0f);
	if (limit > 10)
		limit = 10;

	for (i = 0; i < limit; i++)
		buffer[i] = '*';
	while (i < 10)
		buffer[i++] = ' ';
	buffer[i] = '\0';

	sprintf( string, "VoIP: [%s]", buffer );
	SCR_DrawStringExt( 320 - strlen( string ) * 4, 10, 8, string, g_color_table[ ColorIndex( COLOR_WHITE ) ], qtrue, qfalse );
}
#endif


/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

static	int			current;
static	float		values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph( float value )
{
	values[current] = value;
	current = (current + 1) % ARRAY_LEN(values);
}


/*
==============
SCR_DrawDebugGraph
==============
*/
static void SCR_DrawDebugGraph( void )
{
	int		a, x, y, w, i, h;
	float	v;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re.SetColor( g_color_table[ ColorIndex( COLOR_BLACK ) ] );
	re.DrawStretchPic(x, y - cl_graphheight->integer, 
		w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for (a=0 ; a<w ; a++)
	{
		i = (ARRAY_LEN(values)+current-1-(a % ARRAY_LEN(values))) % ARRAY_LEN(values);
		v = values[i];
		v = v * cl_graphscale->integer + cl_graphshift->integer;
		
		if (v < 0)
			v += cl_graphheight->integer * (1+(int)(-v / cl_graphheight->integer));
		h = (int)v % cl_graphheight->integer;
		re.DrawStretchPic( x+w-1-a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
static void SCR_ClearScoreboard( void );

void SCR_Init( void ) {
	// PS5: split screen, the scoreboard's pictures and scores are this renderer's and map's
	SCR_ClearScoreboard();

	cl_timegraph = Cvar_Get ("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph = Cvar_Get ("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight = Cvar_Get ("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get ("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get ("graphshift", "0", CVAR_CHEAT);

	scr_initialized = qtrue;
}


/*
==================
SCR_Done
==================
*/
void SCR_Done( void ) {
	scr_initialized = qfalse;
}


//=======================================================

/*
==================
SCR_DrawCenteredText

PS5: split screen. A line of text centred on x, in pixels, size pixels a letter.
==================
*/
static void SCR_DrawCenteredText( int x, int y, int size, const char *text, const float *color ) {
	const int length = strlen( text );
	float row, col;
	int i, ch;

	x -= length * size / 2;
	re.SetColor( color );
	for ( i = 0; i < length; i++, x += size ) {
		ch = text[i] & 255;
		if ( ch == ' ' ) {
			continue;
		}
		row = ( ch >> 4 ) * 0.0625f;
		col = ( ch & 15 ) * 0.0625f;
		re.DrawStretchPic( x, y, size, size, col, row, col + 0.0625f, row + 0.0625f, cls.charSetShader );
	}
	re.SetColor( NULL );
}


/*
==================
SCR_DrawPlayerJoining

PS5: split screen. The current player's part of the screen while they join.
==================
*/
static void SCR_DrawPlayerJoining( void ) {
	static const vec4_t grey = { 0.6f, 0.6f, 0.6f, 1.0f };
	int x, y, width, height, size;

	CL_PlayerView( cl_localPlayer, &x, &y, &width, &height );
	size = height / 16;
	SCR_DrawCenteredText( x + width / 2, y + height / 2 - size, size, va( "PLAYER %i", cl_localPlayer + 1 ), g_color_table[ ColorIndex( COLOR_WHITE ) ] );
	size /= 3;
	SCR_DrawCenteredText( x + width / 2, y + height / 2 + size, size, clc.serverMessage[0] ? clc.serverMessage : "Joining...", grey );
}


/*
=================================================================

PS5: SPLIT SCREEN SCOREBOARD

Three players leave a quarter of the screen: it shows the scoreboard, as the
game draws its own (the same pictures and columns in a 640x480 layout, moved
left by SB_LEFT so a name has sixteen letters), for everyone. Player 1's connection asks for the scores once a second
(CL_SplitScores) and the game's answer, the "scores" command, comes here
too (SCR_ParseScores); the names, teams, models and bots' skills are the
configstrings'.

=================================================================
*/

#define SB_HEADER			86
#define SB_TOP				( SB_HEADER + 32 )
#define SB_STATUSBAR		420
#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16
#define SB_RATING_WIDTH		( 6 * BIGCHAR_WIDTH )
#define SB_LEFT				40
#define SB_BOTICON_X		( 32 + SB_RATING_WIDTH / 2 - SB_LEFT )
#define SB_HEAD_X			( 64 + SB_RATING_WIDTH / 2 - SB_LEFT )
#define SB_SCORELINE_X		( 112 - SB_LEFT )
#define SB_SCORE_X			( SB_SCORELINE_X + BIGCHAR_WIDTH )
#define SB_PING_X			( SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8 )
#define SB_TIME_X			( SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8 )
#define SB_NAME_X			( SB_SCORELINE_X + 22 * BIGCHAR_WIDTH )

static struct {
	int			count;
	int			teamScores[2];
	struct {
		int		client, score, ping, time;
	} scores[MAX_CLIENTS];

	qhandle_t	score, ping, time, name;
	qhandle_t	skills[5];
	qhandle_t	icons[MAX_CLIENTS];
	char		models[MAX_CLIENTS][MAX_QPATH];	// the model each icon is

	float		x, y, scaleX, scaleY;	// the quarter, and its 640x480's scale
} scr_board;


static void SCR_ClearScoreboard( void ) {
	Com_Memset( &scr_board, 0, sizeof( scr_board ) );
}


/*
==================
SCR_ParseScores

The game's "scores" command: the count, the teams' scores, then fourteen
numbers a player, sorted, of which the client, score, ping and time are used.
==================
*/
void SCR_ParseScores( const char *s ) {
	int numbers[3 + MAX_CLIENTS * 14];
	int count, i;
	char *end;

	s += strlen( "scores" );
	for ( count = 0; count < ARRAY_LEN( numbers ); count++ ) {
		numbers[count] = strtol( s, &end, 10 );
		if ( end == s ) {
			break;
		}
		s = end;
	}
	if ( count < 3 ) {
		return;
	}

	scr_board.count = MIN( numbers[0], ( count - 3 ) / 14 );
	scr_board.teamScores[0] = numbers[1];
	scr_board.teamScores[1] = numbers[2];
	for ( i = 0; i < scr_board.count; i++ ) {
		scr_board.scores[i].client = numbers[3 + i * 14];
		scr_board.scores[i].score = numbers[3 + i * 14 + 1];
		scr_board.scores[i].ping = numbers[3 + i * 14 + 2];
		scr_board.scores[i].time = numbers[3 + i * 14 + 3];
	}
}


// the quarter's 640x480 to pixels, stretched as the game's own screens are
static void SCR_BoardPic( float x, float y, float w, float h, qhandle_t shader ) {
	re.DrawStretchPic( scr_board.x + x * scr_board.scaleX, scr_board.y + y * scr_board.scaleY,
		w * scr_board.scaleX, h * scr_board.scaleY, 0, 0, 1, 1, shader );
}


static void SCR_BoardFill( float x, float y, float w, float h, const float *color ) {
	re.SetColor( color );
	SCR_BoardPic( x, y, w, h, cls.whiteShader );
	re.SetColor( NULL );
}


// a line in the big font with its drop shadow and colours, as CG_DrawBigString
static void SCR_BoardString( int x, int y, const char *string ) {
	const float shadow[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	vec4_t color;
	const char *s;
	float row, col;
	int pass, xx, ch;

	for ( pass = 0; pass < 2; pass++ ) {
		re.SetColor( pass ? g_color_table[ ColorIndex( COLOR_WHITE ) ] : shadow );
		for ( s = string, xx = x + ( pass ? 0 : 2 ); *s; s++ ) {
			if ( Q_IsColorString( s ) ) {
				if ( pass ) {
					Com_Memcpy( color, g_color_table[ ColorIndexFromChar( s[1] ) ], sizeof( color ) );
					re.SetColor( color );
				}
				s++;
				continue;
			}
			ch = *s & 255;
			if ( ch != ' ' ) {
				row = ( ch >> 4 ) * 0.0625f;
				col = ( ch & 15 ) * 0.0625f;
				re.DrawStretchPic( scr_board.x + xx * scr_board.scaleX, scr_board.y + ( y + ( pass ? 0 : 2 ) ) * scr_board.scaleY,
					BIGCHAR_WIDTH * scr_board.scaleX, BIGCHAR_HEIGHT * scr_board.scaleY,
					col, row, col + 0.0625f, row + 0.0625f, cls.charSetShader );
			}
			xx += BIGCHAR_WIDTH;
		}
	}
	re.SetColor( NULL );
}


static void SCR_BoardCentered( int y, const char *string ) {
	char clean[MAX_STRING_CHARS];

	Q_strncpyz( clean, string, sizeof( clean ) );
	Q_CleanStr( clean );
	SCR_BoardString( ( 640 - (int)strlen( clean ) * BIGCHAR_WIDTH ) / 2, y, string );
}


// a client's icon: its model's (models/players/<model>/icon_<skin>), loaded once a model
static qhandle_t SCR_BoardIcon( int client, const char *model ) {
	char base[MAX_QPATH], *skin;

	if ( scr_board.icons[client] && !Q_stricmp( scr_board.models[client], model ) ) {
		return scr_board.icons[client];
	}
	Q_strncpyz( scr_board.models[client], model, sizeof( scr_board.models[client] ) );
	Q_strncpyz( base, *model ? model : "sarge", sizeof( base ) );
	skin = strchr( base, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	scr_board.icons[client] = re.RegisterShaderNoMip( va( "models/players/%s/icon_%s", base, skin && *skin ? skin : "default" ) );
	return scr_board.icons[client];
}


// one player's line; returns qfalse for a player not on the team asked for
static qboolean SCR_BoardLine( int i, int team, int y, int lineHeight ) {
	static const float localColors[MAX_LOCAL_PLAYERS][4] = {
		{ 0.0f, 0.0f, 0.7f, 0.7f }, { 0.7f, 0.0f, 0.0f, 0.7f }, { 0.0f, 0.55f, 0.0f, 0.7f }, { 0.7f, 0.0f, 0.55f, 0.7f } };
	const int client = scr_board.scores[i].client;
	const char *info;
	int player, skill, size;

	if ( (unsigned)client >= MAX_CLIENTS ) {
		return qfalse;
	}
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_PLAYERS + client ];
	if ( atoi( Info_ValueForKey( info, "t" ) ) != team ) {
		return qfalse;
	}

	// the players on the console, each in their own colour, as P1, P2...
	for ( player = 0; player < CL_SplitViews(); player++ ) {
		if ( CL_PlayerState( player ) >= CA_PRIMED && cl_localPlayers[ player ].connection.clientNum == client ) {
			SCR_BoardFill( SB_SCORELINE_X + BIGCHAR_WIDTH + SB_RATING_WIDTH / 2, y,
				640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT + 1, localColors[ player ] );
			SCR_BoardString( SB_BOTICON_X, y, va( "P%i", player + 1 ) );
			break;
		}
	}

	size = lineHeight == SB_NORMAL_HEIGHT ? 32 : 16;
	skill = atoi( Info_ValueForKey( info, "skill" ) );
	if ( skill >= 1 && skill <= 5 ) {
		SCR_BoardPic( SB_BOTICON_X, size == 32 ? y - ( 32 - BIGCHAR_HEIGHT ) / 2 : y, size, size, scr_board.skills[ skill - 1 ] );
	}
	size = lineHeight == SB_NORMAL_HEIGHT ? 48 : 16;
	SCR_BoardPic( SB_HEAD_X, size == 48 ? y - ( 48 - BIGCHAR_HEIGHT ) / 2 : y, size, size, SCR_BoardIcon( client, Info_ValueForKey( info, "model" ) ) );

	if ( scr_board.scores[i].ping == -1 ) {
		SCR_BoardString( SB_SCORELINE_X + SB_RATING_WIDTH / 2, y, va( " connecting    %s", Info_ValueForKey( info, "n" ) ) );
	} else if ( team == TEAM_SPECTATOR ) {
		SCR_BoardString( SB_SCORELINE_X + SB_RATING_WIDTH / 2, y, va( " SPECT %3i %4i %s", scr_board.scores[i].ping, scr_board.scores[i].time, Info_ValueForKey( info, "n" ) ) );
	} else {
		SCR_BoardString( SB_SCORELINE_X + SB_RATING_WIDTH / 2, y, va( "%5i %4i %4i %s", scr_board.scores[i].score, scr_board.scores[i].ping, scr_board.scores[i].time, Info_ValueForKey( info, "n" ) ) );
	}
	return qtrue;
}


// a team's lines, on its colour in a team game; returns how many
static int SCR_BoardTeam( int team, int y, int lineHeight, int *room ) {
	static const float red[4] = { 1.0f, 0.0f, 0.0f, 0.33f }, blue[4] = { 0.0f, 0.0f, 1.0f, 0.33f };
	int i, n;

	for ( i = n = 0; i < scr_board.count && n < *room; i++ ) {
		if ( SCR_BoardLine( i, team, y + n * lineHeight, lineHeight ) ) {
			n++;
		}
	}
	if ( n && ( team == TEAM_RED || team == TEAM_BLUE ) ) {
		SCR_BoardFill( 0, y - ( lineHeight == SB_NORMAL_HEIGHT ? 16 : 8 ), 640, n * lineHeight + 16, team == TEAM_RED ? red : blue );
	}
	*room -= n;
	return n;
}


/*
==================
SCR_DrawSplitScoreboard

The scoreboard in a quarter of the screen with player 1 current.
==================
*/
static void SCR_DrawSplitScoreboard( int quarter ) {
	static const char *const gametypes[] = { "Free For All", "Tournament", "Single Player", "Team Deathmatch", "Capture the Flag" };
	const char *serverInfo = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	const int gametype = atoi( Info_ValueForKey( serverInfo, "g_gametype" ) );
	int x, y, w, h, i, lineHeight, room;
	const char *s;

	if ( !scr_board.score ) {
		scr_board.score = re.RegisterShaderNoMip( "menu/tab/score.tga" );
		scr_board.ping = re.RegisterShaderNoMip( "menu/tab/ping.tga" );
		scr_board.time = re.RegisterShaderNoMip( "menu/tab/time.tga" );
		scr_board.name = re.RegisterShaderNoMip( "menu/tab/name.tga" );
		for ( i = 0; i < 5; i++ ) {
			scr_board.skills[i] = re.RegisterShaderNoMip( va( "menu/art/skill%i", i + 1 ) );
		}
	}

	CL_PlayerView( quarter, &x, &y, &w, &h );
	scr_board.x = x;
	scr_board.y = y;
	scr_board.scaleX = w / 640.0f;
	scr_board.scaleY = h / 480.0f;

	// the game, and its limit or the teams' scores
	SCR_BoardCentered( 28, gametype >= 0 && gametype < ARRAY_LEN( gametypes ) ? gametypes[ gametype ] : "Scores" );
	if ( gametype >= GT_TEAM ) {
		if ( scr_board.teamScores[0] == scr_board.teamScores[1] ) {
			s = va( "Teams are tied at %i", scr_board.teamScores[0] );
		} else if ( scr_board.teamScores[0] > scr_board.teamScores[1] ) {
			s = va( S_COLOR_RED "Red" S_COLOR_WHITE " leads %i to %i", scr_board.teamScores[0], scr_board.teamScores[1] );
		} else {
			s = va( S_COLOR_BLUE "Blue" S_COLOR_WHITE " leads %i to %i", scr_board.teamScores[1], scr_board.teamScores[0] );
		}
	} else if ( atoi( Info_ValueForKey( serverInfo, "fraglimit" ) ) > 0 ) {
		s = va( "Frag limit %s", Info_ValueForKey( serverInfo, "fraglimit" ) );
	} else {
		s = "";
	}
	SCR_BoardCentered( 52, s );

	SCR_BoardPic( SB_SCORE_X + SB_RATING_WIDTH / 2, SB_HEADER, 64, 32, scr_board.score );
	SCR_BoardPic( SB_PING_X - SB_RATING_WIDTH / 2, SB_HEADER, 64, 32, scr_board.ping );
	SCR_BoardPic( SB_TIME_X - SB_RATING_WIDTH / 2, SB_HEADER, 64, 32, scr_board.time );
	SCR_BoardPic( SB_NAME_X - SB_RATING_WIDTH / 2, SB_HEADER, 64, 32, scr_board.name );

	// as the game: forty units a line, or sixteen for more than fit
	if ( scr_board.count > ( SB_STATUSBAR - SB_TOP ) / SB_NORMAL_HEIGHT ) {
		lineHeight = SB_INTER_HEIGHT;
		room = ( SB_STATUSBAR - SB_TOP ) / SB_INTER_HEIGHT - 1;
	} else {
		lineHeight = SB_NORMAL_HEIGHT;
		room = ( SB_STATUSBAR - SB_TOP ) / SB_NORMAL_HEIGHT;
	}

	y = SB_TOP;
	if ( gametype >= GT_TEAM ) {
		const int first = scr_board.teamScores[0] >= scr_board.teamScores[1] ? TEAM_RED : TEAM_BLUE;
		y += lineHeight / 2;
		y += SCR_BoardTeam( first, y, lineHeight, &room ) * lineHeight + BIGCHAR_HEIGHT;
		y += SCR_BoardTeam( first == TEAM_RED ? TEAM_BLUE : TEAM_RED, y, lineHeight, &room ) * lineHeight + BIGCHAR_HEIGHT;
	} else {
		y += SCR_BoardTeam( TEAM_FREE, y, lineHeight, &room ) * lineHeight + BIGCHAR_HEIGHT;
	}
	SCR_BoardTeam( TEAM_SPECTATOR, y, lineHeight, &room );
}


/*
==================
SCR_DrawPlayerViews

PS5: split screen. Each local player's part of the screen, on black: the gaps
between the parts are lines.
==================
*/
static void SCR_DrawPlayerViews( stereoFrame_t stereoFrame ) {
	const int views = CL_SplitViews();
	int player;

	re.SetColor( g_color_table[ ColorIndex( COLOR_BLACK ) ] );
	re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for ( player = 0; player < views; player++ ) {
		CL_SetLocalPlayer( player );
		switch ( cls.state ) {
		case CA_LOADING:
		case CA_PRIMED:
		case CA_ACTIVE:
			// the game information screen and loading progress, or the game
			if ( cgvm ) {
				CL_CGameRendering( stereoFrame );
				break;
			}
			// fall through
		default:
			SCR_DrawPlayerJoining();
			break;
		}
	}
	CL_SetLocalPlayer( 0 );

	// three players leave the last quarter to the scoreboard
	if ( views == 3 && cls.state == CA_ACTIVE ) {
		SCR_DrawSplitScoreboard( 3 );
	}

	SCR_DrawDemoRecording();
}


/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
static void SCR_DrawScreenField( stereoFrame_t stereoFrame ) {
	qboolean uiFullscreen;
	const int previous = CL_SetLocalPlayer( 0 ); // PS5: split screen, the screen is player 1's

	re.BeginFrame( stereoFrame );

	uiFullscreen = (uivm && VM_Call( uivm, 0, UI_IS_FULLSCREEN ));

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if ( uiFullscreen || cls.state < CA_LOADING ) {
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
			// draw vertical bars on sides for legacy mods
			const int w = (cls.glconfig.vidWidth - ((cls.glconfig.vidHeight * 640) / 480)) /2;
			re.SetColor( g_color_table[ ColorIndex( COLOR_BLACK ) ] );
			re.DrawStretchPic( 0, 0, w, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.DrawStretchPic( cls.glconfig.vidWidth - w, 0, w, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if ( uivm && !uiFullscreen && CL_SplitViews() > 1 && cls.state >= CA_LOADING ) {
		// PS5: split screen, once player 1 has started loading
		SCR_DrawPlayerViews( stereoFrame );
	} else if ( uivm && !uiFullscreen ) {
		switch( cls.state ) {
		default:
			Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad cls.state" );
			break;
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			VM_Call( uivm, 1, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			VM_Call( uivm, 1, UI_REFRESH, cls.realtime );
			VM_Call( uivm, 1, UI_DRAW_CONNECT_SCREEN, qfalse );
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			if ( cgvm ) {
				CL_CGameRendering( stereoFrame );
			}
			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			VM_Call( uivm, 1, UI_REFRESH, cls.realtime );
			VM_Call( uivm, 1, UI_DRAW_CONNECT_SCREEN, qtrue );
			break;
		case CA_ACTIVE:
			// always supply STEREO_CENTER as vieworg offset is now done by the engine.
			CL_CGameRendering( stereoFrame );
			SCR_DrawDemoRecording();
#ifdef USE_VOIP
			SCR_DrawVoipMeter();
#endif
			break;
		}
	}

	// the menu draws next
	if ( Key_GetCatcher( ) & KEYCATCH_UI && uivm ) {
		VM_Call( uivm, 1, UI_REFRESH, cls.realtime );
	}

	// console draws next
	Con_DrawConsole ();

	// debug graph can be drawn on top of anything
	if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer ) {
		SCR_DrawDebugGraph ();
	}

	CL_SetLocalPlayer( previous );
}


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void ) {
	static int recursive;
	static int framecount;
	static int next_frametime;

	if ( !scr_initialized )
		return; // not initialized yet

	if ( framecount == cls.framecount ) {
		int ms = Sys_Milliseconds();
		if ( next_frametime && ms - next_frametime < 0 ) {
			re.ThrottleBackend();
		} else {
			next_frametime = ms + 16; // limit to 60 FPS
		}
	} else {
		next_frametime = 0;
		framecount = cls.framecount;
	}

	if ( ++recursive > 2 ) {
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	// If there is no VM, there are also no rendering commands issued. Stop the renderer in
	// that case.
	if ( uivm )
	{
		// XXX
		int in_anaglyphMode = Cvar_VariableIntegerValue("r_anaglyphMode");
		// if running in stereo, we need to draw the frame twice
		if ( cls.glconfig.stereoEnabled || in_anaglyphMode) {
			SCR_DrawScreenField( STEREO_LEFT );
			SCR_DrawScreenField( STEREO_RIGHT );
		} else {
			SCR_DrawScreenField( STEREO_CENTER );
		}

		if ( com_speeds->integer ) {
			re.EndFrame( &time_frontend, &time_backend );
		} else {
			re.EndFrame( NULL, NULL );
		}
	}

	recursive = 0;
}
