# Make directories
echo -e "\e[1;32mCreating Directories...\e[0m"
mkdir /home/pi/images
mkdir /home/pi/videos
mkdir /home/pi/logs

# Enable raspi-config (may change in the future)
echo -e "\e[1;32mEnabling configuration...\e[0m"
raspi-config nonint do_i2c 0
raspi-config nonint do_spi 0
raspi-config nonint do_serial_hw 0
raspi-config nonint do_serial_cons 1


# Install packages
echo -e "\e[1;32mInstalling system-wide packages (may take a while)...\e[0m"
apt update -y
apt install python3-pip
apt install ffmpeg
apt install xz-utils
apt install i2c-tools
apt install cmake
apt install git

# Install python packages
echo -e "\e[1;32mIntsalling python packages...\e[0m"
pip install -r /home/pi/code/requirements.txt --break-system-packages

# Configure services
echo -e "\e[1;32mEnabling Service...\e[0m"
sudo cp /home/pi/code/services/mainCode.service /etc/systemd/system/mainCode.service
systemctl daemon-reload
systemctl enable mainCode
systemctl start mainCode


echo -e "\e[1;32mBuilding ssdv executable...\e[0m"
cd /home/pi/code/ssdv
sudo make clean
sudo make -j4

# Reboot
echo ""
echo -e "\e[1;36m Install Complete! Rebooting...\e[0m"
reboot now
