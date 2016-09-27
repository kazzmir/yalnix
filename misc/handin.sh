#!/bin/sh

createtar(){
	arg=$1
	dir=rafkind-$arg
	rm -rf $dir &>/dev/null
	mkdir $dir
	cp Makefile SConstruct README $dir
	cp -a src $dir
	cp -a include $dir
	find $dir -name .svn | xargs rm -rf
	tar -cf rafkind-$arg.tar $dir
	rm -rf $dir
}

if [ "x$1" = "x" ]; then
	echo "handin.sh <projectN>"
	exit 0
fi

createtar $1
echo "Created rafkind-$1.tar"
echo "handin cs5460 $1 rafkind-$1.tar"
