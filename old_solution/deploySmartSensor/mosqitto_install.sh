#!/bin/bash -x

#be sure to make it executable at least by You, like chmod 755 ./mosquitto_install.sh

#set -x

echo "Mosqitto install script by PJ, for RPi Raspbian. Loosely based on http://www.steves-internet-guide.com/install-mosquitto-linux/"

if [ $(id -u) != "0" ]; then
	echo "You must be a root to do this!"
	exit 1
fi

netstat -at | grep -q 1880
if [ $? == 0 ]; then
	echo "Mosquitto already running!"
	exit 0
fi

echo -n "What is Your name?>"
read -t 5 userName
echo "Cool $userName, let's try"

#To add to your keyring, do the following as root
wget -O - http://repo.mosquitto.org/debian/mosquitto-repo.gpg.key | apt-key add -

if [ $? -gt 0 ]; then
	echo "Failed adding a key!"
	exit 1
fi

source /etc/os-release
test $VERSION_ID = "10" && wget -O /etc/apt/sources/list.d/mosquitto-buster.list http://repo.mosquitto.org/debian/mosquitto-buster.list
test $VERSION_ID = "9" && wget -O /etc/apt/sources/list.d/mosquitto-stretch.list http://repo.mosquitto.org/debian/mosquitto-stretch.list
test $VERSION_ID = "8" && wget -O /etc/apt/sources/list.d/mosquitto-jessie.list http://repo.mosquitto.org/debian/mosquitto-jessie.list
test $VERSION_ID = "7" && wget -O /etc/apt/sources/list.d/mosquitto-wheezy.list http://repo.mosquitto.org/debian/mosquitto-wheezy.list

apt-get update && apt-get install mosquitto
if [ $? -gt 0 ]; then
	echo "Failed running apt-get!"
	exit 1
fi

#check if this is running
netstat -at | grep -q 1880
if [ $? == 0 ]; then
	echo "Mosquitto already running!"
	exit 0
else
	service mosquitto start || echo "No good. Somehow does not work!!!"
fi

