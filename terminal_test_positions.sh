function goto() {
	echo -e -n "\e[1;20Hgoto column $2, then overwrite"
	echo -e -n "\e[$1;$2H"
}

function clrscr() {
	echo -e -n "\e[2J"
}

function put_at() {
	goto 2 1
	echo 123한567890
	goto 2 $1
	sleep 2
	echo -n /
	sleep 2
}

clrscr

goto 1 1
echo 1234567890

put_at 3
put_at 4
put_at 5
put_at 6
sleep 2
echo

