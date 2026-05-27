#!/bin/bash -x

#be sure to make it executable at least by You, like chmod 755 ./script_name.sh

#set -x

echo "Node-red autostart script by PJ, for RPi Raspbian"

if [ $(id -u) != "0" ]; then
	echo "You must be a root to do this!"
	exit 1
fi

systemctl status nodered.service | grep -q running
if [ $? -eq 0 ]; then
	echo "You already have it and it is running!!!"
	exit 0
fi

systemctl enable nodered.service

node-red-stop

sudo apt-get install npm
sudo npm install -g npm@2.x
hash -r
cd ~/.node-red
npm install node-red-node-sqlite

npm i node-red-dashboard

npm i node-red-node-ui-list

npm install node-red-contrib-influxdb

#echo "You may safely Ctrl+C to kill this window and node-red service will run in background."
#echo "Or don't, if You like to see logs"
#echo "Or do, and type node-red-log to view logs"

#node-red-start

sudo systemctl start nodered.service