/* a stupid shell.
 * *All* programs executed by this shell have their first two arguments
 * as the terminal to print to and the id of the current working directory.
 */
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include "yalnix.h"
#include "fs.h"

#define MAX_LENGTH 1024  /* Should be long enough to hold any line */

static void execProgram( int terminal, char ** argv, char * buf ){
	int pid;
	if ( pid = Fork() ){
		int res;
		if ( pid == -1 ){
			TtyPrintf( terminal, "Shell: Cannot fork process\n" );
			return;
		}
		if ( (pid = Wait(&res)) != -1 ){
			// fprintf(stderr, "PID %d exit status = %d\n", pid, res);
			// TtyPrintf( terminal, "PID %d exit status = %d\n", pid, res);
		} else {
			fprintf(stderr, "PID %d aborted by kernel.\n", pid);
			TtyPrintf( terminal, "PID %d aborted by kernel.\n", pid);
		}
	} else {
		Exec( argv[0], argv );
		TtyPrintf( terminal, "Could not exec `%s'.\n", buf );
		Exit( -2 );
	}
}

void cleanPath( char * buffer ){
	char tmp[ MAX_LENGTH ];
	int i;
	int position = 0;
	int last_char = 0;
	tmp[ position ] = 0;
	for ( i = 0; i < strlen( buffer ) + 1; i++ ){
		// TtyPrintf( 0, "tmp %s position %d last %d current %d\n", tmp, position, last_char, buffer[ i ] );
		switch ( buffer[ i ] ){
			case '/' : {
				/* '//' becomes '/' */
				if ( last_char == '/' ){
				/* './' becomes '' */
				} else if ( last_char == '.' ){
					/*
					while ( tmp[ position ] != '/' ){
						position -= 1;
					}
					*/
					position -= 1;
					if ( position == 0 ){
						position += 1;
					}
					last_char = '/';
				} else {
					tmp[ position ] = buffer[ i ];
					last_char = '/';
					position += 1;
				}
				break;
			}
			case '.' : {
				if ( last_char == '.' ){
					while ( position > 1 && tmp[ position ] != '/' ){
						position -= 1;
					}
					if ( position != 1 ){
						position -= 1;
						while ( position > 1 && tmp[ position ] != '/' ){
							position -= 1;
						}
					}
					last_char = '/';
				} else {
					last_char = '.';
					tmp[ position ] = buffer[ i ];
					position += 1;
				}
				break;
			}
			case 0 : {
				if ( last_char == '.' ){
					position -= 1;
				}
				// tmp[ position ] = 0;
				break;
			}
			default : {
				tmp[ position ] = buffer[ i ];	
				position += 1;
				last_char = buffer[ i ];
				break;
			}
		}
		tmp[ position ] = 0;
	}
	if ( strlen( tmp ) != 1 && tmp[ position - 1 ] == '/' ){
		tmp[ position - 1 ] = 0;
	}
	strcpy( buffer, tmp );
}

int main( int argc, char *argv[] ){
	char buf[MAX_LENGTH];
	char * cmd_argv[MAX_LENGTH];  /* An overkill for expediency */
	// char * prompt = "yalnix $ ";
	char prompt[ MAX_LENGTH ];
	char separators[4];
	int termno;
	int err;

	separators[0] = ' ';
	separators[1] = '\t';
	separators[2] = '\n';
	separators[3] = '\0';

	if (argc < 2) {
		Exit(-2);
	}

	termno = atoi(argv[1]);

	/* In a real OS NUM_TERMINALS could be obtained from a lib call
	 * or a syscall
	 */
	if (termno < 0 || termno > 3) {  
		Exit(-3);
	}

	TtyPrintf( termno, "Starting bash....\n" );
	
	// FH_t current_directory = Lookup( "/", 0 );
	char current_path[ MAX_LENGTH ];
	strcpy( current_path, "/" );

	while ( 1 ){
		// char current_path[ MAX_LENGTH - 32 ];
		int n, i;
		int j = 0;
		bzero(buf, MAX_LENGTH);

		/*
		if ( GetPath( current_path, MAX_LENGTH - 32, current_directory ) == ERROR ){
			sprintf( prompt, "yalnix ? $ " );
		} else {
			sprintf( prompt, "yalnix %s $ ", current_path );
		}
		*/
		sprintf( prompt, "yalnix-bash %s $ ", current_path );

		TtyPrintf( termno, prompt );

		n = TtyRead( termno, buf, MAX_LENGTH );

		if ( n == 0 || n >= MAX_LENGTH ){ /* line too big */
			continue;
		}

		buf[n] = '\0';

		if ( !(cmd_argv[j++] = strtok(buf, separators)) )
			continue;
		if ( strcmp(cmd_argv[0], "exit") == 0 ) {
			TtyPrintf(termno, "Exitting shell....\n");
			Exit(0);
		} else if ( strcmp( cmd_argv[ 0 ], "cd" ) == 0 ){
			if ( !(cmd_argv[j++] = strtok( NULL, separators)) )
				continue;
			char tmp[ MAX_LENGTH ];
			if ( *cmd_argv[ j - 1 ] == '/' ){
				strcpy( tmp, cmd_argv[ j - 1 ] );
			} else {
				strcpy( tmp, current_path );
				strcat( tmp, "/" );
				strcat( tmp, cmd_argv[ j - 1 ] );
			}
			// TtyPrintf( termno, "Current '%s' next '%s'\n", current_path, tmp );
			// FH_t new_dir = Lookup( cmd_argv[ j - 1 ], 0 );
			FH_t new_dir = Lookup( tmp, 0 );
			if ( new_dir != ERROR && IsDirectory( new_dir ) ){
				// current_directory = new_dir;
				strcpy( current_path, tmp );
				cleanPath( current_path );
			}
			continue;
		}

		char b1[ 4 ];
		sprintf( b1, "%d", termno );
		char b2[ 16 ];
		sprintf( b2, "%d", Lookup( current_path, 0 ) );
		cmd_argv[j++] = b1;
		cmd_argv[j++] = b2;

		while ( cmd_argv[j++] = strtok(NULL, separators) ){
			/**/
		}

		execProgram( termno, cmd_argv, buf );
	}

	return 0;
}
