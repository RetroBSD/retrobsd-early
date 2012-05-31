echo 'erase, kill ^U, intr ^C'
stty crt

PATH=/bin:/sbin:/games
export PATH

HOME=/
export HOME

TERM=`/bin/setty`
export TERM

MAC=ba:0b:ab:5e:ed:ed
IP=10.1.1.7
export MAC IP
