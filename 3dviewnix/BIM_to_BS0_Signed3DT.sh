#!/bin/bash

if test $# = 2
then
	sigma=1.85
elif test $# = 3
then
	sigma=$3
else
	echo Usage: BIM_to_BS0_Signed3DT.sh '<input_file> <output_file> [<sigma>]'
	test $# = 3
	exit
fi

siz=( `get_slicenumber "$1" -s` )
ndinterpolate "$1" "$2.INTRP-TMP" 2 ${siz[1]} ${siz[1]} ${siz[1]} 1 1 1 1 || exit

# make 3D distance transform
LTDT3D "$2.INTRP-TMP" "$2.DT-TMP" 2 2 0 resolution=.004 || exit
algebra "$2.INTRP-TMP" "$2.DT-TMP" "$2.ALG-TMP" 32767+y x 32767-y || exit

# Gaussian filter on the distance transformed files; # the input '$3' is 1.85 times pixel size for 13*13*13 kernel, or .92 times pixel size for 7*7*7 kernel
gaussian_3D "$2.ALG-TMP" "$2.FILT-TMP" $sigma || exit

track_all "$2.FILT-TMP" "$2" 1.0 32767.0 65535.0 26 0 0 || exit

rm "$2.FILT-TMP" "$2.DT-TMP" "$2.INTRP-TMP" "$2.ALG-TMP"
