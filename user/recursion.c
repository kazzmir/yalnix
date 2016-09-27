/* grow the stack until the stack explodes. it should die at some point */

void f( unsigned int x ){
	// printf( "Called f %u\n", x );
	f( x + 1 );
}

int main(){
	f( 1 );
}
