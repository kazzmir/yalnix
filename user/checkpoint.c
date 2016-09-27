// CS5460 Checkpoint Test Program

#include "hardware.h"
#include "yalnix.h"
#include "stdio.h"

char buf[80];

int main(int argc, char *argv[]){
	int i;  
	for(i=0; i<argc; i++){
		sprintf(buf, "Testing %d\n", i);
		write(1, buf, strlen(buf));
	}
	return 0;
}
