#!/bin/bash
echo "Start compiling shaders"
shopt -s nullglob
for dir in */;
do
	echo "Precompile shaders in - " $dir
	for filename in $dir*.glsl; 
	do
		echo "process $filename"
		new_file="${filename/.glsl/.spv}"
		glslc.exe "$filename" "-o" "$new_file"
	done

	for filename in $dir*.vert; 
	do
		echo "process $filename"
		new_file="${filename/.vert/_vert.spv}"
		glslc.exe "$filename" "-o" "$new_file"
	done

	for filename in $dir*.frag; 
	do
		echo "process $filename"
		new_file="${filename/.frag/_frag.spv}"
		glslc.exe "$filename" "-o" "$new_file"
	done

	for filename in $dir*.geom; 
	do
		echo "process $filename"
		new_file="${filename/.geom/_geom.spv}"
		glslc.exe "$filename" "-o" "$new_file"
	done
done

read -p "Press any key to resume ..."
