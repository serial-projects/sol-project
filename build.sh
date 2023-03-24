#!/bin/bash
CC="g++"
CXXFLAGS="-g -O2 -march=native"
SRC="./src/*.cpp"
TGT="./bin/sol"
prepare(){
	[[ -d "./bin" ]] && return || mkdir ./bin
}
compile(){
	COMMAND="$CC $SRC -o $TGT $CXXFLAGS"
	echo "[@]: $COMMAND" ; $COMMAND
	if [[ $? -ne 0 ]]; then
		echo "[!] Failed to compile!"
		exit 1
	fi
} 
prepare
compile
