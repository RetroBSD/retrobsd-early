TERMCAP variable contains a list of terminal capabilities.

    co      Numeric     Number of columns
    li      Numeric     Number of lines
    nb      Flag        No bell
    bs      Flag        Use \b as backspace
    cm      String      Cursor movement
    cl      String      Clear screen
    ho      String      Home: move to top left corner
    up      String      Home: move up
    do      String      Move down
    nd      String      Move right
    le      String      Move left
    cu      String      Draw cursor label

    kh      Key Home
    kH      Key End
    ku      Key Up
    kd      Key Down
    kr      Key Right
    kl      Key Left

    kP      Key Page Up
    kN      Key Page Down
    kI      Key Insert Char
    kD      Key Delete Char
    DL      Key Delete Line
    IL      Key Insert Line
    ER      Key Enter
    k-      Key File
    k.      Key '.'
    k0      Key '0'
    k1      Key '1'
    k2      Key '2'
    k3      Key '3'
    k4      Key '4'
    k5      Key '5'
    k6      Key '6'
    k7      Key '7'
    k8      Key '8'
    k9      Key '9'

Example for Linux terminal:

TERMCAP=linux:co#80:li#25:bs:cm=\E[%i%d;%dH:cl=\E[H\E[2J:ho=\E[H:\
up=\E[A:do=\E[B:nd=\E[C:le=\10:cu=\E[7m \E[m:kh=\E[1~:ku=\E[A:kd=\E[B:\
kr=\E[C:kl=\E[D:kP=\E[5~:kN=\E[6~:kI=\E[2~:kD=\E[3~:kh=\E[1~:kH=\E[4~:\
k.=\E[Z:k1=\E[[A:k2=\E[[B:k3=\E[[C:k4=\E[[D:k5=\E[[E:k6=\E[17~:\
k7=\E[18~:k8=\E[19~:k9=\E[20~:k0=\E[21~:F1=\E[23~:F2=\E[24~:
