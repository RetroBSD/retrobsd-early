
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_NONBLOCK	0x0004		/* no delay */
#define	O_APPEND	0x0008		/* set append mode */
#define	O_SHLOCK	0x0010		/* open with shared file lock */
#define	O_EXLOCK	0x0020		/* open with exclusive file lock */
#define	O_ASYNC		0x0040		/* signal pgrp when data ready */
#define	O_FSYNC		0x0080		/* synchronous writes */
#define	O_CREAT		0x0200		/* create if nonexistant */
#define	O_TRUNC		0x0400		/* truncate to zero length */
#define	O_EXCL		0x0800		/* error if already exists */

int aaa;
int bbb;
int ccc;
char gc;
char gbuffer[3];
int gibuffer[4];

extern errno;

main()
{
   char b;
   int la;
   unsigned int u1, u2;
   int s1, s2;
   unsigned char uc1, uc2;
   char sc1, sc2;
   int fd;
   char buffer[6];
   int ibuffer[7];
   
   w_s( "sizeof(uc1): "); w_n( sizeof(uc1), 10 ); w_s( " 1\n" );
   w_s( "sizeof(sc1): "); w_n( sizeof(sc1), 10 ); w_s( " 1\n" );
   w_s( "sizeof(u1): "); w_n( sizeof(u1), 10 ); w_s( " 4\n" );
   w_s( "sizeof(s1): "); w_n( sizeof(s1), 10 ); w_s( " 4\n" );
   w_s( "sizeof(aaa): "); w_n( sizeof(aaa), 10 ); w_s( " 4\n" );
   w_s( "sizeof(bbb): "); w_n( sizeof(bbb), 10 ); w_s( " 4\n" );
   w_s( "sizeof(gc): "); w_n( sizeof(gc), 10 ); w_s( " 1\n" );
   w_s( "sizeof(buffer): "); w_n( sizeof(buffer), 10 ); w_s( " 6\n" );
   w_s( "sizeof(ibuffer): "); w_n( sizeof(ibuffer), 10 ); w_s( " 28\n" );
   w_s( "sizeof(char): "); w_n( sizeof(char), 10 ); w_s( " 1\n" );
   w_s( "sizeof(int): "); w_n( sizeof(int), 10 ); w_s( " 4\n" );
   w_s( "sizeof(gbuffer): "); w_n( sizeof(gbuffer), 10 ); w_s( " 3\n" );
   w_s( "sizeof(gibuffer): "); w_n( sizeof(gibuffer), 10 ); w_s( " 16\n" );
   // sizeof(ibuffer[0]) is not supported, so the following can be used...
   w_s( "sizeof(ibuffer)/sizeof(int): "); w_n( sizeof(ibuffer)/sizeof(int), 10 ); w_s( " 7\n" );

   aaa = 1;
   bbb = 2;
   la = 4;

   
   w_n(aaa,10);
   w_s(" 1\n");
   w_n(bbb,10);
   w_s(" 2\n");
   w_n(la,10);
   w_s(" 4\n");

   uc1 = 0x80;
   sc1 = 0x80;
   s1 = uc1;
   s2 = sc1;
   w_s( "unsigned char (0x80) -> int: "); w_n( s1, 10 ); w_s( " 128\n" );
   w_s( "signed   char (0x80) -> int: "); w_n( s2, 10 ); w_s( " -128\n" );

   u1 = uc1;
   u2 = sc1;
   w_s( "unsigned char (0x80) -> unsigned: "); w_n( u1, 10 ); w_s( " 128\n" );
   w_s( "signed   char (0x80) -> unsigned: "); w_n( u2, 10 ); w_s( " -128\n" );

   la = errno;
   w_s("errno: "); w_n(la,10); w_s( " 0\n");
   
   write( 1, "abcd ", 5 );
   la = errno;
   w_s("errno after good write call: "); w_n(la,10); w_s( " 0\n");
   
   write( 10, "abcde", 5 );
   la = errno;
   w_s("errno after bad write call: "); w_n(la,10); w_s( " 9\n");
 
   write( 1, "abcd ", 5 );
   la = errno;
   w_s("good write after failed should not overwrite errno: "); w_n(la,10); w_s( " 9\n");
   
   errno = 0;
   write( 1, "abcd ", 5 );
   la = errno;
   w_s("good write after errno set to zero: "); w_n(la,10); w_s( " 0\n");
 
   la = write( 1, "abcd ", 5 );
   w_s( "write() return: " ); w_n(la,10); w_s( " 5\n" );
 
   la = write( 10, "abcd ", 5 );
   w_s( "write(bad fd) return: " ); w_n(la,10); w_s( " -1\n" );

   fd = open( "/a.txt", O_WRONLY|O_CREAT, 0666 );
   if( fd != -1 )
   {
	   w_s("open success\n");
	   la = write( fd, "abcd\n", 5 );
	   if( la == 5 ) w_s( "write success\n" ); else  w_s( "write failed\n" );
	   la = close( fd );
	   if( la != -1 ) w_s( "close success\n" ); else w_s( "close failed\n" );
   }
   else
   {
	   w_s("open failed\n");
   }

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;
	buffer[4] = 0;
	buffer[5] = 0;
	
   fd = open( "/a.txt", O_RDONLY, 0666 );
   if( fd != -1 )
   {
	   w_s("open success\n");
	   la = read( fd, buffer, 5 );
	   w_s( buffer );
	   if( la == 5 ) w_s( "read success\n" ); else  w_s( "read failed\n" );
	   la = close( fd );
	   if( la != -1 ) w_s( "close success\n" ); else w_s( "close failed\n" );
   }
   else
   {
	   w_s("open failed\n");
   }
   
   if( buffer[0] != 'a') w_s("data0 readback from file MISMATCH\n");
   if( buffer[1] != 'b') w_s("data1 readback from file MISMATCH\n");
   if( buffer[2] != 'c') w_s("data2 readback from file MISMATCH\n");
   if( buffer[3] != 'd') w_s("data3 readback from file MISMATCH\n");
   if( buffer[4] != '\n') w_s("data4 readback from file MISMATCH\n");
   
   if( buffer[0] != 'a' || buffer[1] != 'b' || buffer[2] != 'c' || buffer[3] != 'd' || buffer[4] != '\n') w_s("data readback from file MISMATCH\n");
   else w_s("data readback from file OK\n");
   
   exit(0);
}
