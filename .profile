echo 'erase, kill ^U, intr ^C'
stty crt
PATH=/bin:/sbin
export PATH
HOME=/
export HOME
TERMCAP='linux:co#80:li#25:bs:am:cm=\E[%i%d;%dH:cl=\E[H\E[2J:ho=\E[H:up=\E[A:do=\E[B:nd=\E[C:le=^H:cu=\E[7m \E[m:kh=\E[1~:ku=\E[A:kd=\E[B:kr=\E[C:kl=\E[D:kP=\E[5~:kN=\E[6~:kI=\E[2~:kD=\E[3~:kh=\E[1~:kH=\E[4~:k.=\E[Z:k1=\E[[A:k2=\E[[B:k3=\E[[C:k4=\E[[D:k5=\E[[E:k6=\E[17~:k7=\E[18~:k8=\E[19~:k9=\E[20~:k0=\E[21~:'
export TERMCAP
