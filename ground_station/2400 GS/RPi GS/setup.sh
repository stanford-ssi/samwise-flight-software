# Creating needed directories
mkdir /home/pi/images
mkdir /home/pi/videos
mkdir /home/pi/logs

# Install packages
echo -e "\e[1;32mInstalling system-wide packages... \e[0m"
apt update -y
apt install python3-pip
apt install i2c-tools
apt install cmake
apt install git
