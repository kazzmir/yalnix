#include <stdlib.h>

int main(){
	while ( 1 ){
		if ( ! malloc( 0x100 ) ){
			break;
		}
	}
}
