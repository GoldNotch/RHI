#!/bin/bash
install_path="./"
while getopts o: flag
do
    case "${flag}" in
        o) install_path=${OPTARG};;
    esac
done

echo "Start compiling shaders into " $install_path
shopt -s nullglob
for dir in */;
do
	echo "Precompile shaders in - " $dir
	for filename in $dir*.glsl; 
	do
		echo "process $filename"
		new_file="$install_path$(basename ${filename/.glsl/.spv})"
		glslc.exe "$filename" "-o" "$new_file"
		echo "compiled - " $new_file
	done

	for filename in $dir*.vert; 
	do
		echo "process $filename"
		new_file="$install_path$(basename ${filename/.vert/_vert.spv})"
		glslc.exe "$filename" "-o" "$new_file"
		echo "compiled - " $new_file
	done

	for filename in $dir*.frag; 
	do
		echo "process $filename"
		new_file="$install_path$(basename ${filename/.frag/_frag.spv})"
		glslc.exe "$filename" "-o" "$new_file"
		echo "compiled - " $new_file
	done

	for filename in $dir*.geom; 
	do
		echo "process $filename"
		new_file="$install_path$(basename ${filename/.geom/_geom.spv})"
		glslc.exe "$filename" "-o" "$new_file"
		echo "compiled - " $new_file
	done
done

read -p "Press any key to resume ..."
