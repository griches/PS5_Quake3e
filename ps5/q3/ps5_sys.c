/*
===========================================================================
Quake3e for the PlayStation 5 - the system layer: launch, the main loop,
the console's log, and the exit through the shell.

Replaces code/unix/unix_main.c. Based on it:
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
===========================================================================
*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#include <ps5platform/fp.h>

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "client/client.h"

#include "platform.h"

void IN_Frame( void );

// the CRT's hook (ps5/src/platform.c): asks the shell to close the title, never returns
void catchReturnFromMain( int status );

static qboolean sys_exiting;


void NORETURN Sys_Exit( int code )
{
	say( "ends: status %d", code );
	fflush( NULL );
	// PS5: a title never calls exit(); the shell closes it
	catchReturnFromMain( code );
	for ( ;; )
		usleep( 100000 );
}


void NORETURN Sys_Quit( void )
{
	if ( !sys_exiting ) {
		sys_exiting = qtrue;
#ifndef DEDICATED
		CL_Shutdown( "", qtrue );
#endif
	}
	Sys_Exit( 0 );
}


void Sys_Init( void )
{
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", "player" );
}


void NORETURN FORMAT_PRINTF(1, 2) QDECL Sys_Error( const char *format, ... )
{
	va_list argptr;
	char text[1024];

	va_start( argptr, format );
	Q_vsnprintf( text, sizeof( text ), format, argptr );
	va_end( argptr );

	say( "Sys_Error: %s", text );

	if ( !sys_exiting ) {
		sys_exiting = qtrue;
#ifndef DEDICATED
		CL_Shutdown( text, qtrue );
#endif
	}
	Sys_Exit( 1 );
}


// PS5: klog takes lines (each gets the title's prefix), so print whole lines
void Sys_Print( const char *msg )
{
	static char line[1024];
	static int used;

	for ( ; *msg; msg++ ) {
		const char c = *msg;
		if ( Q_IsColorString( msg ) ) {
			msg++;
			continue;
		}
		if ( c == '\n' || used == (int)sizeof( line ) - 1 ) {
			line[used] = '\0';
			say( "%s", line );
			used = 0;
			if ( c == '\n' )
				continue;
		}
		if ( c >= ' ' || c == '\t' )
			line[used++] = c;
	}
}


void QDECL Sys_SetStatus( const char *format, ... )
{
}


char *Sys_ConsoleInput( void )
{
	return NULL;
}


char *Sys_GetClipboardData( void )
{
	return NULL;
}


void Sys_SendKeyEvents( void )
{
#ifndef DEDICATED
	HandleEvents();
#endif
}


void Sys_Sleep( int msec )
{
	struct timespec req;

	if ( msec < 0 ) {
		// wait for a packet; the pad is polled every frame regardless
		NET_Sleep( 10 * 1000 );
		return;
	}
	if ( msec == 0 )
		return;
	req.tv_sec = msec / 1000;
	req.tv_nsec = ( msec % 1000 ) * 1000000;
	nanosleep( &req, NULL );
}


qboolean Sys_LowPhysicalMemory( void )
{
	return qfalse;
}


void Sys_BeginProfiling( void )
{
}


const char *Sys_DefaultBasePath( void )
{
	return "/app0";
}


char *Sys_DefaultAppPath( void )
{
	static char path[] = "/app0";
	return path;
}


#ifdef USE_AFFINITY_MASK
uint64_t Sys_GetAffinityMask( void )
{
	return 0;
}


qboolean Sys_SetAffinityMask( const uint64_t mask )
{
	return qfalse;
}
#endif


// What the engine calls that no module a title loads exports, under the names
// ps5/CMakeLists.txt gives those calls.

// net_ip.c asks for an interface's index when it joins the IPv6 multicast group.
// 0 is "no such interface".
unsigned int ps5q3_if_nametoindex( const char *name )
{
	return 0;
}


// net_ip.c only ever asks for the numeric form of an address (NI_NUMERICHOST)
int ps5q3_getnameinfo( const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen,
	char *serv, size_t servlen, int flags )
{
	const void *address;

	if ( serv && servlen )
		serv[0] = '\0';
	if ( !host || !hostlen )
		return 0;
	if ( sa->sa_family == AF_INET )
		address = &( (const struct sockaddr_in *)sa )->sin_addr;
	else if ( sa->sa_family == AF_INET6 )
		address = &( (const struct sockaddr_in6 *)sa )->sin6_addr;
	else
		return EAI_FAMILY;
	return inet_ntop( sa->sa_family, address, host, (socklen_t)hostlen ) ? 0 : EAI_FAIL;
}


// net_ip.c makes its sockets non-blocking with ioctl(FIONBIO), which the console
// refuses a title ("Permission denied", first run on the console: no socket opened,
// no multiplayer). fcntl does it; the console's own socket option is the fallback.
#define PS5_SO_NBIO 0x1200
int ps5q3_ioctl( int fd, unsigned long request, ... )
{
	va_list args;
	int *value;
	int flags;

	if ( request != FIONBIO ) {
		errno = EINVAL;
		return -1;
	}
	va_start( args, request );
	value = va_arg( args, int * );
	va_end( args );

	flags = fcntl( fd, F_GETFL, 0 );
	if ( flags != -1 && fcntl( fd, F_SETFL, *value ? ( flags | O_NONBLOCK ) : ( flags & ~O_NONBLOCK ) ) != -1 )
		return 0;
	return setsockopt( fd, SOL_SOCKET, PS5_SO_NBIO, value, sizeof( *value ) );
}


const char *ps5q3_gai_strerror( int code )
{
	switch ( code ) {
	case EAI_NONAME: return "the name is not known";
	case EAI_FAMILY: return "no address of that family";
	case EAI_MEMORY: return "out of memory";
	case EAI_AGAIN: return "the console's resolver did not start";
	default: return "the name lookup failed";
	}
}


// Name lookup. The platform layer's getaddrinfo answers EAI_FAIL to everything
// (second run on the console: no master server resolved), so: a numeric address
// is parsed here, and a name goes to the console's own resolver (libSceNet),
// which gives one IPv4 address.
int sceNetInit( void );
int sceNetPoolCreate( const char *name, int size, int flags );
int sceNetPoolDestroy( int pool );
int sceNetResolverCreate( const char *name, int pool, int flags );
int sceNetResolverStartNtoa( int resolver, const char *hostname, struct in_addr *address, int timeout, int retry, int flags );
int sceNetResolverDestroy( int resolver );

static int ps5q3_resolve( const char *name, struct in_addr *address )
{
	static qboolean net_started;
	int pool, resolver, result;

	if ( !net_started ) {
		sceNetInit(); // an error here is "already initialised"
		net_started = qtrue;
	}
	pool = sceNetPoolCreate( "q3resolver", 64 * 1024, 0 );
	if ( pool < 0 ) {
		say( "name lookup: sceNetPoolCreate gave 0x%08x", (unsigned)pool );
		return EAI_AGAIN;
	}
	resolver = sceNetResolverCreate( "q3resolver", pool, 0 );
	if ( resolver < 0 ) {
		say( "name lookup: sceNetResolverCreate gave 0x%08x", (unsigned)resolver );
		sceNetPoolDestroy( pool );
		return EAI_AGAIN;
	}
	result = sceNetResolverStartNtoa( resolver, name, address, 0, 0, 0 );
	if ( result < 0 )
		say( "name lookup: %s: sceNetResolverStartNtoa gave 0x%08x", name, (unsigned)result );
	sceNetResolverDestroy( resolver );
	sceNetPoolDestroy( pool );
	return result < 0 ? EAI_NONAME : 0;
}


int ps5q3_getaddrinfo( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **result )
{
	const int family = hints ? hints->ai_family : AF_UNSPEC;
	struct in_addr v4;
	struct in6_addr v6;
	qboolean is6 = qfalse;
	struct addrinfo *info;
	size_t size;

	if ( !result )
		return EAI_FAIL;
	*result = NULL;
	if ( !node || !*node )
		return EAI_NONAME;

	if ( inet_pton( AF_INET, node, &v4 ) == 1 ) {
		if ( family == AF_INET6 )
			return EAI_FAMILY;
	} else if ( inet_pton( AF_INET6, node, &v6 ) == 1 ) {
		if ( family == AF_INET )
			return EAI_FAMILY;
		is6 = qtrue;
	} else {
		int error;
		if ( family == AF_INET6 )
			return EAI_NONAME; // the resolver gives IPv4 addresses only
		error = ps5q3_resolve( node, &v4 );
		if ( error )
			return error;
	}

	size = sizeof( *info ) + ( is6 ? sizeof( struct sockaddr_in6 ) : sizeof( struct sockaddr_in ) );
	info = calloc( 1, size );
	if ( !info )
		return EAI_MEMORY;
	info->ai_family = is6 ? AF_INET6 : AF_INET;
	info->ai_socktype = hints ? hints->ai_socktype : 0;
	info->ai_protocol = hints ? hints->ai_protocol : 0;
	info->ai_addr = (struct sockaddr *)( info + 1 );
	if ( is6 ) {
		struct sockaddr_in6 *a = (struct sockaddr_in6 *)info->ai_addr;
		a->sin6_len = sizeof( *a );
		a->sin6_family = AF_INET6;
		a->sin6_addr = v6;
		info->ai_addrlen = sizeof( *a );
	} else {
		struct sockaddr_in *a = (struct sockaddr_in *)info->ai_addr;
		a->sin_len = sizeof( *a );
		a->sin_family = AF_INET;
		a->sin_addr = v4;
		info->ai_addrlen = sizeof( *a );
	}
	*result = info;
	return 0;
}


void ps5q3_freeaddrinfo( struct addrinfo *info )
{
	free( info ); // one entry, with its address behind it
}


// common.c sets a umask before it writes; Sys_FOpen and Sys_Mkdir set the modes here
mode_t ps5q3_umask( mode_t mask )
{
	return 0;
}


void Sys_SetClipboardBitmap( const byte *bitmap, int length )
{
}


// code/unix/linux_signals.c is not built: the console writes its own crash record
void InitSig( void )
{
}


/*
Version 1.4.0 moved the title from PPSA99303, the ID two PS3 emulators (PS5SX3
and KB-RPCS3) use too, to PPSA99319. The first launch without game data copies
the player's pak files (baseq3/*.pk3, but the menus' own zz-ps5-ui.pk3) and
settings (baseq3/q3config.cfg) from the old folder. The old folder is only read,
never changed, and only when it holds Quake's pak0.pk3. A file goes in under a
.part name and takes its own when whole, so a copy cut short starts again.
*/
#define PS5_OLD_DATA "/data/homebrew/PPSA99303/baseq3"
#define PS5_DATA "/app0/baseq3"

static qboolean PS5_CopyFile( const char *name )
{
	static char buffer[ 4 << 20 ];
	char from[ MAX_OSPATH ], to[ MAX_OSPATH ], part[ MAX_OSPATH ];
	FILE *in, *out;
	size_t got;
	qboolean ok = qtrue;

	Com_sprintf( from, sizeof( from ), "%s/%s", PS5_OLD_DATA, name );
	Com_sprintf( to, sizeof( to ), "%s/%s", PS5_DATA, name );
	Com_sprintf( part, sizeof( part ), "%s.part", to );
	if ( access( to, F_OK ) == 0 )
		return qtrue;
	in = fopen( from, "rb" );
	if ( !in ) {
		say( "data: cannot read %s (%s)", from, strerror( errno ) );
		return qfalse;
	}
	out = fopen( part, "wb" );
	if ( !out ) {
		say( "data: cannot write %s (%s)", part, strerror( errno ) );
		fclose( in );
		return qfalse;
	}
	while ( ok && ( got = fread( buffer, 1, sizeof( buffer ), in ) ) > 0 )
		ok = fwrite( buffer, 1, got, out ) == got;
	ok = ok && !ferror( in );
	fchmod( fileno( out ), 0666 );
	ok = fclose( out ) == 0 && ok;
	fclose( in );
	if ( !ok || rename( part, to ) != 0 ) {
		say( "data: copying %s failed (%s)", name, strerror( errno ) );
		unlink( part );
		return qfalse;
	}
	return qtrue;
}

static void PS5_MoveIn( void )
{
	struct dirent *entry;
	const char *name;
	size_t length;
	int copied = 0, failed = 0;
	DIR *dir;

	if ( access( PS5_DATA "/pak0.pk3", F_OK ) == 0 )
		return;
	if ( access( PS5_OLD_DATA "/pak0.pk3", R_OK ) != 0 ) {
		say( "data: no pak0.pk3 here, nor in %s (%s)", PS5_OLD_DATA, strerror( errno ) );
		return;
	}
	dir = opendir( PS5_OLD_DATA );
	if ( !dir ) {
		say( "data: cannot list %s (%s)", PS5_OLD_DATA, strerror( errno ) );
		return;
	}
	say( "data: copying the game data and settings from %s", PS5_OLD_DATA );
	while ( ( entry = readdir( dir ) ) != NULL ) {
		name = entry->d_name;
		length = strlen( name );
		if ( Q_stricmp( name, "q3config.cfg" ) && ( length < 4 || Q_stricmp( name + length - 4, ".pk3" ) || !Q_stricmp( name, "zz-ps5-ui.pk3" ) ) )
			continue;
		if ( PS5_CopyFile( name ) )
			copied++;
		else
			failed++;
	}
	closedir( dir );
	say( "data: %d files copied, %d failed", copied, failed );
}


int main( int argc, const char *argv[] )
{
	char con_title[ MAX_CVAR_VALUE_STRING ];
	int xpos, ypos;
	// The game modules run in the bytecode interpreter (no JIT yet), sound is the
	// 48 kHz the console's output takes, and the menus start at the display's size.
	static char cmdline[1024] =
		"+set vm_game 1 +set vm_cgame 1 +set vm_ui 1 "
		"+set s_khz 48 +set r_fullscreen 1 +set com_introplayed 1";
	FILE *extra;

	platform_init( PS5_TITLE_NAME );
	// IEEE floating point, as the engine's other platforms start with (a title
	// starts with flush-to-zero and denormals-are-zero)
	ps5_fp_ieee();

	PS5_MoveIn();

	// More start-up commands, one line, from the title folder (put there over FTP):
	// /data/homebrew/<TITLE_ID>/cmdline.txt, for example "+map q3dm1"
	extra = fopen( "/app0/cmdline.txt", "r" );
	if ( extra ) {
		const size_t used = strlen( cmdline );
		cmdline[used] = ' ';
		if ( !fgets( cmdline + used + 1, (int)( sizeof( cmdline ) - used - 1 ), extra ) )
			cmdline[used] = '\0';
		cmdline[strcspn( cmdline, "\r\n" )] = '\0';
		fclose( extra );
	}
	say( "command line: %s", cmdline );

	Com_EarlyParseCmdLine( cmdline, con_title, sizeof( con_title ), &xpos, &ypos );
	Com_Init( cmdline );

	while ( 1 ) {
		IN_Frame();
		Com_Frame( CL_NoDelay() );
	}
	return 0;
}
