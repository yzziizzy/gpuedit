#!/bin/sh

here="$( cd "$( dirname "$0" )" &> /dev/null && pwd )"

sudo echo -n

opt_dest=/usr/bin
opt_o=3
opt_q_auto='-q'
opt_q_make='-s'

while getopts ":d:o:v" opt; do
	case $opt in
		d)
			# release build destination
			opt_dest="$OPTARG"
			;;
		o)
			# optimization level
			opt_o="$OPTARG"
			;;
		v)
			# verbose mode
			opt_q_auto=''
			opt_q_make=''
			;;
		\?)
			echo "Invalid option: -$OPTARG"
			exit 1
			;;
		:)
			echo "Option -$OPTARG requires an argument"
			exit 1
			;;
	esac
done

shift $(($OPTIND - 1))

echo "configuring for release..." \
&& ./autogen.sh $opt_q_auto CFLAGS="-O$opt_o" $* \
&& echo "cleaning..." \
&& make clean $opt_q_make \
&& echo "building for release..." \
&& make -j $opt_q_make \
&& echo "installing..." \
&& sudo cp src/gpuedit "$opt_dest" \
&& echo "configuring for development..." \
&& ./autogen.sh  $opt_q_auto CFLAGS='-O0' \
&& echo "cleaning..." \
&& make clean $opt_q_make \
&& echo "building for development..." \
&& make -j $opt_q_make \
&& echo "done"

