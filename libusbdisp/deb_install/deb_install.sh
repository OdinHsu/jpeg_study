#!/bin/bash

COREDIR=/opt/apps/com.racertech.usbdisp/
PRODUCT="USB-DISP"
user_name=$(pwd | cut -d "/" -f3)
VERSION=1.0.0
ACTION=install
tal=118

lsb="$(lsb_release -is)"
ARCH="$(uname -m)"

if [ "$ARCH" == "x86_64" ]; then
	ARCH_NAME=amd64
elif [ "$ARCH" == "aarch64" ]; then
	ARCH_NAME=arm64
fi

export user_name
# Dependencies
dep_check() {
    deps=(libusb-1.0-0-dev libdrm-dev dkms)
    for dep in ${deps[@]}
    do
        if ! dpkg -s $dep | grep "Status: install ok installed" > /dev/null 2>&1
        then
	    if ! apt-get update
            then
                echo "$dep installation failed.  Aborting."
                exit 1
            fi
            if ! apt-get install -y $dep
            then
                echo "$dep installation failed.  Aborting."
		exit 1
            fi
        else
            echo "$dep is installed"
        fi
    done
}
patch_check(){
	if [ "$lsb" == "Kylin" ] && [ -f "/etc/.kyinfo" ]; then
		#if [ '-2203-' == $(cat /etc/.kyinfo | grep "milestone" | grep -o "\-2203\-") ]; then
			echo -e "\ninstalling xorg patch for fix bug"
			if [ -d "/opt/apps/com.racertech.usbdisp/files/xorg/" ]; then
				echo -e "\nxorg patch exist"
				dpkg -i /opt/apps/com.racertech.usbdisp/files/xorg/$ARCH_NAME/*common*
				dpkg -i /opt/apps/com.racertech.usbdisp/files/xorg/$ARCH_NAME/*xorg-core*
			fi
		#fi
	fi
}
root_check(){
# root check
if (( $EUID != 0 ));
then
        echo -e "\nMust be run as root (i.e: 'sudo $0')."
        exit 1
fi
}

usage()
{
  echo
  echo "Installs $PRODUCT, version $VERSION."
  echo "Usage: $SELF [ install | uninstall ]"
  echo
  echo "The default operation is install."
  echo "If unknown argument is given, a quick compatibility check is performed but nothing is installed."
  exit 1
}

install()
{
  tail -n +$tal "$0" >/tmp/com.racertech.usbdisp_1.0.0-1_$ARCH_NAME.deb
  dpkg -i /tmp/com.racertech.usbdisp_1.0.0-1_$ARCH_NAME.deb > usbdisp_dpkg.log
  cat usbdisp_dpkg.log
}


uninstall()
{
  rm -rf /tmp/com.racertech*
  dpkg --remove com.racertech.usbdisp
  rmmod evdi
}


root_check

while [ -n "$1" ]; do
  case "$1" in
    install)
      ACTION="install"
      ;;

    uninstall)
      ACTION="uninstall"
      ;;
    *)
      usage
      ;;
  esac
  shift
done

if [ "$ACTION" == "install" ]; then
  dep_check > dep_check.log
  cat dep_check.log
  install
  patch_check
elif [ "$ACTION" == "uninstall" ]; then
  uninstall
fi

exit 0
