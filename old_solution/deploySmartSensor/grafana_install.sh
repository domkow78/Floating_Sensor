#!/bin/bash -x

#be sure to make it executable at least by You, like chmod 755 ./xxx_install.sh

#set -x

echo "grafana install script by PJ, for RPi Raspbian."

if [ $(id -u) != "0" ]; then
	echo "You must be a root to do this!"
	exit 1
fi

systemctl status grafana-server | grep -q running
if [ $? -eq 0 ]; then
	echo "You already have it and it is running!!!"
#	exit 0
fi

wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
if [ $? -gt 0 ]; then
	echo "Failed adding a key!"
	exit 1
fi

echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list
if [ $? -gt 0 ]; then
	echo "Failed adding a deb source to the list!"
	exit 1
fi

apt-get update
echo "Fetched package list from new repository"

apt install -y grafana
if [ $? -gt 0 ]; then
	echo "Failed adding installing deb!"
	exit 1
fi

#sudo  tar cvzf ./grafanaData /var/lib/grafana
tar -xvf ./grafanaData


 systemctl unmask grafana-server.service
 systemctl start grafana-server
 systemctl enable grafana-server.service

systemctl status grafana-server | grep -q running
if [ $? -gt 0 ]; then
	echo "Somehow it failed!!!"
	exit 1
fi

echo "You should have some grafana working under :8086."
exit 0
