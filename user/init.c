#include "yalnix.h"
#ifdef LINUX
#define NULL 0
#endif

#include <stdio.h>

#define CONSOLE 0
#define MAX_TERMINAL 2

#define MAX_ARGC 32

#define NOFORK (-1)
#define NOEXEC (-2)

int ForkChild(char * cmd_argv[])
{
  int pid;

  switch (pid = Fork()) {
  case 0:
    Exec(cmd_argv[0], cmd_argv);
    return NOEXEC;
  case ERROR:
    return NOFORK;
  default:
    return pid;
    break;
  }
}

char * getProgram( int id ){
	switch ( id ){
		case 0 : return "console";
		case 1 : return "bash";
		case 2 : return "shell";
		default : return "bash";
	}
}

void startFileServer(){
	TtyPrintf( 0, "Starting file server\n" );
	if ( Fork() == 0 ){
		char * argv[ 2 ];
		argv[ 0 ] = "fileserver";
		argv[ 1 ] = 0;
		Exec( "fileserver", argv );
		TtyPrintf( 0, "Could not start file server\n" );
		// Exit( -1 );
	}
}

int main(int argc, char *argv[]){
  int pids[MAX_TERMINAL + 1];
  char * cmd_argv[MAX_ARGC];
  char numbuf[5];
  int i;
  int child_cnt;
  int status;

  startFileServer();

  cmd_argv[1] = numbuf;
  cmd_argv[2] = NULL;
  child_cnt = 0;
  for ( i = 0; i <= MAX_TERMINAL; i++) {
    sprintf(numbuf, "%d", i);

    cmd_argv[ 0 ] = getProgram( i );
    /*
    if (i == CONSOLE) {
      cmd_argv[0] = "console";
    }
    else {
      cmd_argv[0] = "bash";
    }      
    */

    switch (pids[i] = ForkChild(cmd_argv)) {
    case NOEXEC:
      TtyPrintf(CONSOLE," forking to terminal %d, %s %s\n", i, 
		cmd_argv[0], cmd_argv[1]);
      TtyPrintf(CONSOLE,
	"Control prog for terminal %d could not be started.\n", i);
      break;
    case NOFORK:
      TtyPrintf(CONSOLE," forking to terminal %d, %s %s\n", i, 
		cmd_argv[0], cmd_argv[1]);
      TtyPrintf(CONSOLE,
		"Error in Fork() for terminal %d.\n", i);  
      break;
    default:
      TtyPrintf(CONSOLE," forking to terminal %d, %s %s\n", i, 
		cmd_argv[0], cmd_argv[1]);
      TtyPrintf(CONSOLE, 
		"Starting pid %d in terminal %d\n", pids[i], i);
      child_cnt++;
    }
  }


  if (pids[CONSOLE] == -1) {
    TtyPrintf(CONSOLE, "Cannot start Console monitor, halting...\n");
    Exit(-1);
  }
  while (1) {
    int pid;
    if (!child_cnt) {
      TtyPrintf(CONSOLE, 
		"No terminals could be serviced, halting...\n");
      Exit(-2);
    }
    TtyPrintf(CONSOLE," waiting....\n");
    pid = Wait(&status);
    TtyPrintf(CONSOLE," returned from wait, pid %d\n", pid);

    if (pid == pids[CONSOLE]) {
      TtyPrintf(CONSOLE, "Halting Yalnix\n");
      Shutdown();
      Exit(0);
    }
    for (i = 0; i <= MAX_TERMINAL; i++) {
      if (pid == pids[i]) break;
    }
    if ( i > MAX_TERMINAL ){
    	continue;
    }
    TtyPrintf(CONSOLE, "Pid %d died at terminal %d.\n", pids[i], i);
    // cmd_argv[0] = "bash";
    cmd_argv[ 0 ] = getProgram( i );
    sprintf(numbuf, "%d", i);
    pids[i] = ForkChild(cmd_argv);
    switch (pids[i]) {
    case NOEXEC:
      TtyPrintf(CONSOLE,
		"Shell for terminal %d could not be started.\n", i);
      child_cnt--;
      break;
    case NOFORK:
      TtyPrintf(CONSOLE,
		"Error in Fork() for terminal shell %d.\n", i);  
      child_cnt--;
      break;
    default:
      TtyPrintf(CONSOLE,
		"Started shell with pid %d in terminal %d.\n",
		pids[i], i);
      break;
    }
  }

  return 0;
}
