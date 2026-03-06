#!/bin/bash -x

#be sure to make it executable at least by You, like chmod 755 ./xxx_install.sh

echo "SQLite install script by PJ, for RPi Raspbian."

set +x
read -p "Show debug text too? (Y/N): " debugTxt && [[ $debugTxt == [yY] ]] && set -x

if [[ -z $SUDO_USER ]]; then
	echo "Use sudo to run this script"
	exit 1
fi

if [ $(id -u) != "0" ]; then
	echo "You must be a root to do this!"
	exit 1
fi

copyDb=yes
apt list --installed | grep -q sqlite3
if [ $? -eq 0 ]; then
	echo "You already have sqlite3 engine and may use it!!!"
	read -p "Copy database? (Y/N): " copyDb && [[ $copyDb == [yY] || $copyDb == [yY][eE][sS] ]] && copyDb=yes
else
	apt-get --assume-yes install sqlite3
	if [ $? -gt 0 ]; then
		echo "Failed running apt-get install sqlite3!"
		exit 1
	fi
fi

if [ $copyDb == "yes" ]; then
        cp ./dbSmartSensor /home/$SUDO_USER/dbSmartSensor
	chmod 666 /home/$SUDO_USER/dbSmartSensor
	echo "template DB copied to /home/$SUDO_USER/dbSmartSensor file"
fi

echo "SQLite db for Wireless Sensor should be ready."

apt list --installed | grep -q sqlitebrowser
if [ $? -eq 0 ]; then
	exit 0
	echo "Sqlitebrowser already installed. Use it to browse/edit the database"
else
	read -p "Install SQLite Browser? (Y/N): " confirm && [[ $confirm == [yY] || $confirm == [yY][eE][sS] ]] || exit 0
	apt-get --assume-yes install sqlitebrowser
	if [ $? -gt 0 ]; then
		echo "Failed running apt-get install sqlitebrowser!"
		exit 1
	fi
fi

exit 0
