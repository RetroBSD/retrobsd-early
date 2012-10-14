w_s(s)
char*s;
{
    write( 1, s, strlen(s) );
}

strlen(s)
char*s;
{
    char *e;
    e = s;
    while(*e != 0) ++e;
    return e-s;
}

w_c(c)
char c;
{
    write( 1, &c, 1 );
}

w_n(number, radix)
int number, radix;
{
    int i;
    char *drp;
    drp = "0123456789ABCDEF";

    if (number < 0 & radix == 10)
    {
	w_c('-');
        number = -number;
    }
    if ((i = number / radix) != 0)
        w_n(i, radix);
    w_c(drp[number % radix]);
}