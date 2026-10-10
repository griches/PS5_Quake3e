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
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>


#include <libgen.h>

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

//=============================================================================

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int: 
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure whether it is actually used as an unsigned int
     (which would affect the wrap period) */
int Sys_Milliseconds( void )
{
	struct timeval tp;
	int curtime;

	gettimeofday( &tp, NULL );
	
	if ( !sys_timeBase )
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	curtime = (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;
	
	return curtime;
}


char *strlwr( char *s ) {
  if ( s==NULL ) { // bk001204 - paranoia
    assert(0);
    return s;
  }
  while (*s) {
    *s = tolower(*s);
    s++;
  }
  return s; // bk001204 - duh
}


/*
==================
Sys_RandomBytes
==================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	FILE *fp;

	fp = fopen( "/dev/urandom", "r" );
	if( !fp )
		return qfalse;

	setvbuf( fp, NULL, _IONBF, 0 ); // don't buffer reads from /dev/urandom

	if ( fread( string, sizeof( byte ), len, fp ) != len ) {
		fclose( fp );
		return qfalse;
	}

	fclose( fp );
	return qtrue;
}


//============================================


static int Sys_ListExtFiles( const char *directory, const char *subdir, const char *extension, const char *filter, char **list, int maxfiles, int subdirs )
{
	char		search[MAX_OSPATH * 2 + MAX_QPATH + 1];
	char		filename[MAX_OSPATH * 2];
	int		nfiles;
	struct dirent	*d;
	DIR		*fdir;
	int		extLen;
	struct stat st;
	qboolean	hasPatterns;
	const char	*x;
	qboolean	dironly;

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = qtrue;
	} else {
		dironly = qfalse;
	}

	extLen = (int)strlen( extension );
	hasPatterns = Com_HasPatterns( extension ); // contains either '?' or '*'
	if ( hasPatterns && extension[0] == '.' && extension[1] != '\0' ) {
		extension++;
	}

	nfiles = 0;

	if ( *subdir != '\0' ) {
		Com_sprintf( search, sizeof( search ), "%s/%s", directory, subdir );
	} else {
		Com_sprintf( search, sizeof( search ), "%s", directory );
	}

	if ((fdir = opendir(search)) == NULL) {
		return nfiles;
	}

	// search
	while ((d = readdir(fdir)) != NULL) {
		if ( search[0] != '\0' ) {
			Com_sprintf( filename, sizeof( filename ), "%s/%s", search, d->d_name );
		} else {
			Q_strncpyz( filename, d->d_name, sizeof( filename ) );
		}
		if (stat(filename, &st) == -1) {
			continue;
		}
		if (st.st_mode & S_IFDIR) {
			// handle recursion
			if ( subdirs > 0 ) {
				if ( !Q_streq( d->d_name, "." ) && !Q_streq( d->d_name, ".." ) ) {
					char subdir2[MAX_OSPATH * 2 + MAX_QPATH + 1];
					if ( *subdir != '\0' ) {
						Com_sprintf( subdir2, sizeof( subdir2 ), "%s/%s", subdir, d->d_name );
					} else {
						Q_strncpyz( subdir2, d->d_name, sizeof( subdir2 ) );
					}
					if ( nfiles >= maxfiles ) {
						break;
					}
					nfiles += Sys_ListExtFiles( directory, subdir2, extension, filter, list + nfiles, maxfiles - nfiles, subdirs - 1);
				}
			}
			if ( !dironly ) {
				continue;
			}
		} else {
			if ( dironly ) {
				continue;
			}
		}
		if ( *subdir != '\0' ) {
			Com_sprintf( filename, sizeof( filename ), "%s/%s", subdir, d->d_name );
		} else {
			Q_strncpyz( filename, d->d_name, sizeof( filename ) );
		}
		if ( filter != NULL && *filter != '\0' ) {
			if ( !Com_FilterPath( filter, filename ) ) {
				continue;
			}
		} else if ( *extension != '\0' ) {
			if ( hasPatterns ) {
				x = strrchr( d->d_name, '.' );
				if ( x == NULL || !Com_FilterExt( extension, x + 1 ) ) {
					continue;
				}
			} else {
				// check for exact extension
				const int length = strlen( d->d_name );
				if ( length < extLen || Q_stricmp( d->d_name + length - extLen, extension ) ) {
					continue;
				}
			}
		}
		if ( nfiles >= maxfiles ) {
			break;
		}
		list[ nfiles++ ] = FS_CopyString( filename );
	}

	closedir( fdir );

	return nfiles;
}


char** Sys_ListFiles( const char *directory, const char *extension, const char *filter, int *numfiles, int subdirs )
{
	char**	listCopy;
	char*	list[MAX_FOUND_FILES];
	int	i, nfiles;

	if ( extension == NULL ) {
		extension = "";
	}

	nfiles = Sys_ListExtFiles( directory, "", extension, filter, list, ARRAY_LEN( list ), subdirs );

	// copy list from stack, reserve extra space for NULL
	listCopy = Z_Malloc( (nfiles + 1) * sizeof( listCopy[0] ) );
	for ( i = 0; i < nfiles; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;


	if ( nfiles > 1 ) {
		Com_SortList( listCopy, nfiles - 1 );
		if ( nfiles > 2 ) {
			if ( Q_streq( listCopy[0], "." ) && Q_streq( listCopy[1], ".." ) ) {
				// emulate old strgtr() function sort behavior for special entries
				char* dot1 = listCopy[0];
				char* dot2 = listCopy[1];
				for ( i = 0; i < nfiles - 2; i++ ) {
					listCopy[i] = listCopy[i + 2];
				}
				listCopy[nfiles - 2] = dot1;
				listCopy[nfiles - 1] = dot2;
			}
		}
	}

	*numfiles = nfiles;
	return listCopy;
}


/*
=================
Sys_FreeFileList
=================
*/
void Sys_FreeFileList( char **list ) {
	int		i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}


/*
=============
Sys_GetFileStats
=============
*/
qboolean Sys_GetFileStats( const char *filename, fileOffset_t *size, fileTime_t *mtime, fileTime_t *ctime ) {
	struct stat s;

	if ( stat( filename, &s ) == 0 ) {
		*size = (fileOffset_t)s.st_size;
		*mtime = (fileTime_t)s.st_mtime;
		*ctime = (fileTime_t)s.st_ctime;
		return qtrue;
	} else {
		*size = 0;
		*mtime = *ctime = 0;
		return qfalse;
	}
}


/*
=================
Sys_Mkdir

PS5: everything the title creates stays reachable over FTP (0777)
=================
*/
qboolean Sys_Mkdir( const char *path )
{
	if ( mkdir( path, 0777 ) == 0 ) {
		chmod( path, 0777 );
		return qtrue;
	}
	return errno == EEXIST ? qtrue : qfalse;
}


/*
=================
Sys_FOpen

PS5: files the title writes are 0666, for the same reason
=================
*/
FILE *Sys_FOpen( const char *ospath, const char *mode )
{
	struct stat buf;
	FILE *f;

	// check if path exists and it is not a directory
	if ( stat( ospath, &buf ) == 0 && S_ISDIR( buf.st_mode ) )
		return NULL;

	f = fopen( ospath, mode );
	if ( f && ( strchr( mode, 'w' ) || strchr( mode, 'a' ) || strchr( mode, '+' ) ) )
		fchmod( fileno( f ), 0666 );
	return f;
}


qboolean Sys_ResetReadOnlyAttribute( const char *ospath )
{
	return qfalse;
}


// PS5: the title's folder; there is no working directory to ask for
const char *Sys_Pwd( void )
{
	return "/app0";
}


// PS5: no home; an empty home path makes the engine write under fs_basepath
const char *Sys_DefaultHomePath( void )
{
	return "";
}


const char *Sys_SteamPath( void )
{
	return "";
}


void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
}


// PS5: no dynamic loading; the game modules run as QVMs
void *Sys_LoadLibrary( const char *name )
{
	return NULL;
}


void Sys_UnloadLibrary( void *handle )
{
}


void *Sys_LoadFunction( void *handle, const char *name )
{
	return NULL;
}


int Sys_LoadFunctionErrors( void )
{
	return 1;
}
