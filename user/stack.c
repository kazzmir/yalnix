/* test stack creation. when stack5 is called the program should be killed
 */

#include "yalnix.h"

void stack5(){
	int q;
	int * f = &q - 0x5000;
	TtyPrintf( 0, "In stack 5. I will die when I access %p\n", f );
	*f = 23;
	TtyPrintf( 0, "I didn't die. There is a bug in the kernel!\n" );
}

void stack4( int * q ){
	int v[ 0x10000 ];
	TtyPrintf( 0, "In stack 4 v %p\n", &v );
	v[ 12 ] = q[ 434 ];
	stack5();
}

void stack3( char x ){
	int q[ 0x1000 ];
	TtyPrintf( 0, "In stack 3 q %p\n", &q );
	q[ 434 ] = x + 9;
	stack4( q );
}

void stack2(){
	char x[ 100 ];
	TtyPrintf( 0, "In stack 2 x %p\n", &x );
	x[ 43 ] = 9 * 2;
	stack3( x[ 43 ] );
}

void stack1(){
	TtyPrintf( 0, "In stack 1\n" );
	stack2();
}

int main(){

	stack1();

	return 0;

}
