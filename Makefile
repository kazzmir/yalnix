all:
	@scons -j 3 || ./failsafe.sh

clean:
	@scons -c || rm yalnix

count:
	wc src/*.c user/*.c user/file-server/*.c fs/*.c

list:
	ls -l src/*.c include/*.h

kill:
	killall yalnixtty

yalnix: all

makeuserprog:
	cd userprog ; make
