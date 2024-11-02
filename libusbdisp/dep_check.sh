#!/bin/bash

ARCH="$(uname -m)"

if [ "$ARCH" == "x86_64" ]; then
        ARCH_NAME=amd64
elif [ "$ARCH" == "aarch64" ]; then
        ARCH_NAME=arm64
fi

separator(){
sep="\n-------------------------------------------------------------------"
echo -e $sep
}
# Dependencies
dep_check() {
echo -e "\nChecking dependencies\n"

deps=(unzip libev-dev libdrm-dev dkms libpng-dev libghc-x11-dev libusb-1.0-0-dev gcc g++ libev-dev cmake)

for dep in ${deps[@]}
do
	if ! dpkg -s $dep | grep "Status: install ok installed" > /dev/null 2>&1
	then
		default=y
		read -p "$dep not found! Install? [Y/n] " response
		response=${response:-$default}
		if [[ $response =~  ^(yes|y|Y)$ ]]
		then
			if ! apt-get install $dep
			then
				echo "$dep installation failed.  Aborting."
				exit 1
			fi
		else
			separator
			echo -e "\nCannot continue without $dep.  Aborting."
			separator
		exit 1
		fi
	else
		echo "$dep is installed"
	fi
done
}
root_check(){
# root check
if (( $EUID != 0 ));
then
        echo -e "\nMust be run as root (i.e: 'sudo $0')."
        exit 1
fi
}

stage(){
	cp -r ./../build/stage .
	cp deb_install/resource/$ARCH_NAME/libev.so.4.0.0 ./stage/lib
	cd ./stage/lib/ && ln -s libev.so.4.0.0 libev.so.4
}

ACTION=dep

while [ -n "$1" ]; do
  case "$1" in
    dep)
      ACTION="dep"
      ;;

    cp_stage)
      ACTION="cp_stage"
      ;;
    *)
      ;;
  esac
  shift
done

if [ "$ACTION" == "dep" ]; then
	root_check
	dep_check
elif [ "$ACTION" == "cp_stage" ]; then
	rm -rf ./stage
	stage
fi
