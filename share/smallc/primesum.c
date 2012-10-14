main()
{
    int sum;
    int n;
    sum = 0;
    for(n=2;n<10000;++n){
        if( isprime(n) ){
            sum += n;
        }
    }
    w_s("sum of primes less than 10000: ");
    w_n(sum,10);
    w_c('\n');

    exit(1);
}

isprime(n)
int n;
{
    int j;
    if( n == 2 ) return 1;
    if( n % 2 == 0 ) return 0;
    for(j=3;j*j<=n;j+=2)
        if( n % j == 0 ) return 0;
    return 1;
}
