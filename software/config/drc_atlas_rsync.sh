#!/bin/bash

read -p "WARNING: This will copy all files from ~/drc to ~/drc on atlas0. Okay? [yN]: " answer
case ${answer:0:1} in
	y|Y )
		rsync -av --exclude-from=$HOME/drc/software/config/drc_atlas_rsync_excludes.txt $HOME/drc/ drc@atlas0:/home/drc/drc
	;;
	* )
		echo "Cancelled."
		exit
	;;
esac
