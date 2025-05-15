# WSL Installation for Zephyr RTOS
# Based on zephyr install script

sudo apt-get update
sudo apt-get install -y wget

# Installation for WSL2
sudo apt install linux-tools-virtual hwdata
sudo update-alternatives --install /usr/local/bin/usbip usbip `ls /usr/lib/linux-tools/*/usbip | tail -n1` 20
sudo apt-get install -y libxkbcommon-x11-0
sudo apt --fix-broken install -y

#Install prerequisits for Zephyr
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

#download and install zephyr sdk
mkdir $HOME/zephyr_install
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz -P $HOME/zephyr_install
tar xvf $HOME/zephyr_install/zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz -C $HOME/zephyr_install
rm $HOME/zephyr_install/zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz
$HOME/zephyr_install/zephyr-sdk-0.17.0/setup.sh -t arm-zephyr-eabi -h -c

#Allow access to serial ports
sudo apt-get remove modemmanager brltty	#Remove serial terminal interference
sudo adduser $USER dialout		#Add user permissions for dialout

#install zephyr west tool
sudo apt install -y python3-venv    #install python virtual environment
cd $HOME/zephyr_install
python3 -m venv env
source $HOME/zephyr_install/env/bin/activate
cd $HOME
echo source $HOME/zephyr_install/env/bin/activate >> $HOME/.bashrc
pip3 install west
source $HOME/.bashrc

#install nrf command line tools
wget https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-24-0/nrf-command-line-tools_10.24.0_amd64.deb -P $HOME/zephyr_install
cd $HOME/zephyr_install
sudo dpkg -i nrf-command-line-tools_10.24.0_amd64.deb
sudo apt install /opt/nrf-command-line-tools/share/JLink_Linux_V794e_x86_64.deb --fix-broken

#install openocd and screen
sudo apt install -y openocd
sudo apt install -y screen
