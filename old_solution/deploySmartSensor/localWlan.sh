#!/bin/bash -x
#be sure to make it executable at least by You, like chmod 755 
#./xxx_install.sh set -x
echo "WiFi install script by PJ, to enable wlan, connect to local wifi, get IP, update routes..." 
if [ $(id -u) != "0" ]; then
	echo "You must be a root to do this!" 
	exit 1 
fi 

if [ $# -ne 2 ]; then
	echo "USAGE: $0 SSID PASSPHRASE \n"
	exit 1
fi

WLAN=$(ls /sys/class/net | grep w)
SSID=$1
PASS=$2

#route -n 
#iwconfig
ifconfig $WLAN up
ifconfig $WLAN | grep -q "flags=.*<.*UP"
if [ $? -ne 0 ]; then
	echo "Failed upping $WLAN !\n"
	exit 1
fi

iwlist $WLAN scan | grep -q "ESSID:\"$SSID\""

wpa_passphrase $SSID $PASS | tee /etc/wpa_supplicant.conf
wpa_supplicant -B -c /etc/wpa_supplicant.conf -i $WLAN
dhclient $WLAN

route -n | grep -q $WLAN
if [ $? -eq 0 ]; then
	echo "Your routes via $WLAN iface:\n"
	route -n | grep $WLAN
elif
	echo "something went wrong!\n"
	exit 1
fi


echo "Should be ready."
exit 0 
