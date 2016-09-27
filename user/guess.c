/* guess the number game. either the human or the computer can be the guesser
 */

#include "yalnix.h"

static int tty = 0;

static void initRandom(){
	srand( time( 0 ) );
}

static int menu(){
	char buf[ 4 ];
	TtyPrintf( tty, "What kind of game do you want to play?\n" );
	TtyPrintf( tty, "1. You( human ) pick a number and I will guess it\n" );
	TtyPrintf( tty, "2. I( computer ) pick a number and you guess it\n" );
	int read = TtyRead( tty, buf, 4 );
	buf[ read ] = 0;
	return atoi( buf );
}

/* computer uses 'number' as a starting guess */
static void computerGuess( int number ){
	int correct = 0;
	int low = 1;
	int high = 100;
	int tries = 0;
	TtyPrintf( tty, "Ok. Think of a number between 1 and 100 and I will guess it\n" );
	while ( ! correct ){
		char buf[ 4 ];
		tries += 1;
		TtyPrintf( tty, "Is it %d?\n", number );
		int response = -1;
		while ( response == -1 ){
			TtyPrintf( tty, "1. Too low\n" );
			TtyPrintf( tty, "2. Too high\n" );
			TtyPrintf( tty, "3. Thats right\n" );
			int read = TtyRead( tty, buf, 4 );
			buf[ read ] = 0;
			response = atoi( buf );
			switch ( response ){
				case 1 : {
					low = number;
					number = (low + high) / 2;
					break;
				}
				case 2 : {
					high = number;
					number = (low + high) / 2;
					break;
				}
				case 3 : {
					correct = 1;
					break;
				}
				default : {
					response = -1;
				}
			}
		}
	}

	TtyPrintf( tty, "I won in %d tries!\n", tries );
}

static void humanGuess( int number ){
	int tries = 0;
	int guess = -1;
	while ( guess != number ){
		char buf[ 5 ];
		tries += 1;
		TtyPrintf( tty, "Guess a number between 1 and 100\n" );
		int read = TtyRead( tty, buf, 5 );
		buf[ read ] = 0;
		guess = atoi( buf );
		if ( guess < number ){
			TtyPrintf( tty, "Too low\n" );
		} else if ( guess > number ){
			TtyPrintf( tty, "Too high\n" );
		}
	}
	TtyPrintf( tty, "You won in %d tries!\n", tries );
}

static void playGame( int number ){
	int game = menu();
	switch( game ){
		case 1 : {
			computerGuess( number );
			break;
		}
		case 2 : {
			humanGuess( number );
			break;
		}
		default : {
			playGame( number );
		}
	}
}

static int ask(){
	char buf[ 10 ];
	TtyPrintf( tty, "Do you want to play again? y/n\n" );
	TtyRead( tty, buf, 10 );
	return buf[ 0 ] == 'y' || buf[ 0 ] == 'Y';
}

int main( int argc, char ** argv ){

	if ( argc > 1 ){
		tty = atoi( argv[ 1 ] );
	}

	initRandom();
	do{
		playGame( rand() % 100 );
	} while ( ask() );
	TtyPrintf( tty, "Bye!\n" );
}
