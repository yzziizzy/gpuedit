#!/bin/bash


sys_path=/etc/gpuedit
sudo rm -rf $sys_path

sys_config_path=$sys_path/config
echo "Copying configuration files to $sys_config_path..."
sudo mkdir -p $sys_config_path
sudo cp ./config/options.release.json $sys_path/options.json
sudo cp ./config/commands.json $sys_config_path/.


echo "Building highlighters..."
bash mkhighlight.sh
sys_highlighters_path=$sys_path/highlighters
echo "Copying highlighters to $sys_highlighters_path..."
sudo mkdir -p $sys_highlighters_path
sudo cp ./src/highlighters/*.so $sys_highlighters_path/.
sudo cp ./config/*_colors.txt $sys_highlighters_path/.


sys_shaders_path=$sys_path/shaders
echo "Copying shaders to $sys_shaders_path..."
sudo mkdir -p $sys_shaders_path
sudo cp ./src/shaders/* $sys_shaders_path/.


echo "Building executable..."
bash build.sh -d
sudo cp ./gpuedit /usr/bin/gpuedit


home_path=~/.gpuedit
if [ ! -d $home_path ]; then
	echo "Generating user configs..."
	mkdir -p $home_path
	cp ./config/options.user.json $home_path/options.json
	cp ./config/commands.json $home_path/.
	# todo: get system level themes in case of other user
	# todo: generate user configs on startup rather than installation
	home_themes_path=$home_path/themes
	mkdir -p $home_themes_path
	cp ./config/themes/* $home_themes_path/.
else
	echo "Found existing user configs. Skipping."
fi

echo "Done."
