#!/bin/bash -x

#be sure to make it executable at least by You, like chmod 755 ./xxx_install.sh

#set -x

echo "Influxdb install script by PJ, for RPi Raspbian."

if [ $(id -u) != "0" ]; then
	echo "You must be a root to do this!"
	exit 1
fi

systemctl status influxdb | grep -q running
if [ $? -eq 0 ]; then
	echo "You already have it and it is running!!!"
	exit 0
fi

wget -qO- https://repos.influxdata.com/influxdb.key | sudo apt-key add -
if [ $? -gt 0 ]; then
	echo "Failed adding a key!"
	exit 1
fi
source /etc/os-release
test $VERSION_ID = "7" && echo "deb https://repos.influxdata.com/debian wheezy stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
test $VERSION_ID = "8" && echo "deb https://repos.influxdata.com/debian jessie stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
test $VERSION_ID = "9" && echo "deb https://repos.influxdata.com/debian stretch stable" | sudo tee /etc/apt/sources.list.d/influxdb.list

#apt-get --assume-yes update && 
apt-get --assume-yes install influxdb
if [ $? -gt 0 ]; then
	echo "Failed running apt-get install influxdb!"
	exit 1
fi

#apt-get --assume-yes install crudini
#if [ $? -gt 0 ]; then
#	echo "Failed running apt-get install crudini!"
#	exit 1
#fi

# these must be set in /etc/influxdb/influxdb.conf
#[http]
#enabled = true
#bind-address = ":8086"
# but the file is ugly and crudini cannot parse it, shit

cp ./influxdb.conf /etc/influxdb/influxdb.conf
chown root /etc/influxdb/influxdb.conf
chmod 644 /etc/influxdb/influxdb.conf

#sudo  tar cvzf ./influxdbData /var/lib/influxdb/data/
tar -xvf ./influxdbData

systemctl unmask influxdb.service
systemctl start influxdb
systemctl enable influxdb.service


#check if this is running
systemctl status influxdb | grep -q running
if [ $? -gt 0 ]; then
	echo "It failed somehow!!!"
	exit 1
fi

#on some versions the client is not available, so install it manually here
sudo apt install influxdb-client

echo "https://docs.influxdata.com/influxdb/v1.8/query_language/"
echo "Examples of influxQL: just run from bash influx and go on"
echo "influx"
echo "show databases"
echo "use dbSmartSensor"
echo "show measurements"
echo "select * from test"
echo "delete from test where time < '2020-05-26'"

exit 0
