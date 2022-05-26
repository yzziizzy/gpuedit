#!/bin/bash

here="$( cd "$( dirname "$0" )" &> /dev/null && pwd )"


opt_all=0
opt_dest="$HOME/.gpuedit/highlighters/."
opt_parser=0\
opt_sudo=""


while getopts ":ad:ps" opt; do
	case $opt in
		a)
			opt_all=1
			;;
		d)
			opt_dest="$OPTARG""/."
			;;
		p)
			opt_parser=1
			;;
		s)
			opt_sudo=sudo
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


# check if src/sti/parser/parser_gen exists
# and is newer than the source file
cd src/sti/parser
if [ $opt_parser -eq 1 ] || ! [ -f parser_gen ] || [ parser_gen -ot parser_gen.c ]; then
	echo "building parser_gen"
	./build.sh
fi


cd "$here/src/highlighters"
paths=( $(find * -type d) )


for i in "${paths[@]}"; do
	cd "$here/src/highlighters/$i"
	if [ $opt_all -eq 1 ] || [ "../$i.so" -ot lexer.c ] || [ "../$i.so" -ot lexer.h ] || [ "../$i.so" -ot tokens.txt ]; then
		echo "building $i highlighter..."
		./build.sh
		cd "$here"
		$opt_sudo cp "$here/src/highlighters/$i.so" "$opt_dest"
	else
		echo "$i highlighter already up to date"
	fi
done









