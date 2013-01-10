#!/bin/bash
positions=( "+3+21" "+21+21" "+39+21" "+57+21" "+21+3" "+21+39" )
if [ "$#" -ne 6 ]
then
	echo "usage: ./make-cube id0 id1 id2 id3 id4 id5 id6"
	exit 1
fi

args=( "$1" "$2" "$3" "$4" "$5" "$6" )
command='convert -size 72x54 xc:#00000000
	-size 72x18 canvas:white -geometry +0+18 -composite
	-size 18x54 canvas:white -geometry +18+0 -composite '
marker_base="std-border/SimpleStd_"
output_file='ar-cube'

for i in {0..5}
do
	n=`printf %03d ${args["$i"]}`
	output_file="$output_file-$n"
	command="$command $marker_base$n.png -geometry ${positions["$i"]} -composite "
done
command="$command $output_file.png"
${command}

