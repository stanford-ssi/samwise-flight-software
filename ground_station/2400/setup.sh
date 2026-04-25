# Creating needed directories
mkdir -p /home/pi/images
mkdir -p /home/pi/videos
mkdir -p /home/pi/logs

# Install packages
echo -e "\e[1;32mInstalling system-wide packages... \e[0m"
apt update -y
apt install -y python3-pip
apt install -y i2c-tools
apt install -y cmake
apt install -y git
