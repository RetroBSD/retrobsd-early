main()
{
   int a,b,c,d;
   int arr[5];
   int *pi;
   char arrc[5];
   char *pic;
   int s1,s2;
   int z;
   int t;
   int *pip;
   int *picp;
   int e1,e2;

   a = 21;
   b = 31;
   c = 71;
   d = 82;

   arr[0] = 10;
   arr[1] = 20;
   arr[2] = 30;
   arr[3] = 40;
   arr[4] = 50;
   pi = &arr[0];

   arrc[0] = 13;
   arrc[1] = 23;
   arrc[2] = 33;
   arrc[3] = 43;
   arrc[4] = 53;
   pic = &arrc[0];

   w_s("21+31="); w_n(a+b,10); w_s(" (52)\n");
   w_s("21-31="); w_n(a-b,10); w_s(" (-10)\n");
   w_s("21&71="); w_n(a&c,10); w_s(" (5)\n");
   w_s("21|82="); w_n(a|d,10); w_s(" (87)\n");
   w_s("21^82="); w_n(a^d,10); w_s(" (71)\n");
   w_s("21*82="); w_n(a*d,10); w_s(" (1722)\n");
   w_s("82%21="); w_n(d%a,10); w_s(" (19)\n");
   w_s("82/21="); w_n(d/a,10); w_s(" (3)\n");
   w_s("*pi="); w_n(*pi,10); w_s(" (10)\n");
   w_s("*pi+1="); w_n(*pi+1,10); w_s(" (11)\n");
   w_s("*(pi+1)="); w_n(*(pi+1),10); w_s(" (20)\n");
   w_s("&arr[3]-&arr[0]="); w_n(&arr[3]-&arr[0],10); w_s(" (3)\n");

   w_s("*pic="); w_n(*pic,10); w_s(" (13)\n");
   w_s("*pic+1="); w_n(*pic+1,10); w_s(" (14)\n");
   w_s("*(pic+1)="); w_n(*(pic+1),10); w_s(" (23)\n");
   w_s("&arrc[3]-&arrc[0]="); w_n(&arrc[3]-&arrc[0],10); w_s(" (3)\n");

   s1 = 3;
   s2 = -200;
   w_s("82<<3="); w_n(d<<s1,10); w_s(" (656)\n");
   w_s("82>>3="); w_n(d>>s1,10); w_s(" (10)\n");
   w_s("-200>>3="); w_n(s2>>s1,10); w_s(" (-25)\n");
   w_s("-200<<3="); w_n(s2<<s1,10); w_s(" (-1600)\n");

   w_s("-s1="); w_n(-s1,10); w_s(" (-3)\n");
   w_s("-s2="); w_n(-s2,10); w_s(" (200)\n");

   w_s("~82="); w_n(~d,10); w_s(" (-83)\n"); 

   z = 0;

   w_s("!82="); w_n(!d,10); w_s(" (0)\n"); 
   w_s("!0="); w_n(!z,10); w_s(" (1)\n"); 

   t = 0;
   if(t) printt(t,"failure\n"); else printt(t,"success\n"); 
   t = 1;
   if(t) printt(t,"success\n"); else printt(t,"failure\n"); 
   t = 8;
   if(t) printt(t,"success\n"); else printt(t,"failure\n"); 
   t = -2;
   if(t) printt(t,"success\n"); else printt(t,"failure\n"); 

   w_s("0&&0="); w_n(z&&z,10); w_s(" (0)\n"); 
   w_s("0&&21="); w_n(z&&a,10); w_s(" (0)\n"); 
   w_s("3&&21="); w_n(s1&&a,10); w_s(" (1)\n"); 
   w_s("21&&3="); w_n(a&&s1,10); w_s(" (1)\n"); 

   w_s("0||0="); w_n(z||z,10); w_s(" (0)\n"); 
   w_s("0||21="); w_n(z||a,10); w_s(" (1)\n"); 
   w_s("3||21="); w_n(s1||a,10); w_s(" (1)\n"); 
   w_s("21||3="); w_n(a||s1,10); w_s(" (1)\n"); 

   pi = 4;
   w_s("pi++="); w_n(pi++,10); w_s(" (4)\n"); 
   w_s("pi="); w_n(pi,10); w_s(" (8)\n"); 
   w_s("++pi="); w_n(++pi,10); w_s(" (12)\n"); 
   w_s("pi--="); w_n(pi--,10); w_s(" (12)\n"); 
   w_s("pi="); w_n(pi,10); w_s(" (8)\n"); 
   w_s("--pi="); w_n(--pi,10); w_s(" (4)\n"); 

   pic = 4;
   w_s("pic++="); w_n(pic++,10); w_s(" (4)\n"); 
   w_s("pic="); w_n(pic,10); w_s(" (5)\n"); 
   w_s("++pic="); w_n(++pic,10); w_s(" (6)\n"); 
   w_s("pic--="); w_n(pic--,10); w_s(" (6)\n"); 
   w_s("pic="); w_n(pic,10); w_s(" (5)\n"); 
   w_s("--pic="); w_n(--pic,10); w_s(" (4)\n"); 

   t = 4;
   w_s("t++="); w_n(t++,10); w_s(" (4)\n"); 
   w_s("t="); w_n(t,10); w_s(" (5)\n"); 
   w_s("++t="); w_n(++t,10); w_s(" (6)\n"); 
   w_s("t--="); w_n(t--,10); w_s(" (6)\n"); 
   w_s("t="); w_n(t,10); w_s(" (5)\n"); 
   w_s("--t="); w_n(--t,10); w_s(" (4)\n"); 
    
   t = 4;
   w_s("t==4="); w_n(t==4,10); w_s(" (1)\n"); 
   w_s("t==3="); w_n(t==3,10); w_s(" (0)\n"); 
   w_s("t==5="); w_n(t==5,10); w_s(" (0)\n"); 
   t = -4;
   w_s("t==-4="); w_n(t==-4,10); w_s(" (1)\n"); 
   w_s("t==-3="); w_n(t==-3,10); w_s(" (0)\n"); 
   w_s("t==-5="); w_n(t==-5,10); w_s(" (0)\n"); 
   w_s("t==4="); w_n(t==4,10); w_s(" (0)\n"); 
   w_s("t==3="); w_n(t==3,10); w_s(" (0)\n"); 
   w_s("t==5="); w_n(t==5,10); w_s(" (0)\n"); 
 
   t = 4;
   w_s("t!=4="); w_n(t!=4,10); w_s(" (0)\n"); 
   w_s("t!=3="); w_n(t!=3,10); w_s(" (1)\n"); 
   w_s("t!=5="); w_n(t!=5,10); w_s(" (1)\n"); 
   t = -4;
   w_s("t!=-4="); w_n(t!=-4,10); w_s(" (0)\n"); 
   w_s("t!=-3="); w_n(t!=-3,10); w_s(" (1)\n"); 
   w_s("t!=-5="); w_n(t!=-5,10); w_s(" (1)\n"); 
   w_s("t!=4="); w_n(t!=4,10); w_s(" (1)\n"); 
   w_s("t!=3="); w_n(t!=3,10); w_s(" (1)\n"); 
   w_s("t!=5="); w_n(t!=5,10); w_s(" (1)\n"); 

   t = 4;
   w_s("t<4="); w_n(t<4,10); w_s(" (0)\n"); 
   w_s("t<3="); w_n(t<3,10); w_s(" (0)\n"); 
   w_s("t<5="); w_n(t<5,10); w_s(" (1)\n"); 
   w_s("t<-1="); w_n(t<-1,10); w_s(" (0)\n"); 

   w_s("t<=4="); w_n(t<=4,10); w_s(" (1)\n"); 
   w_s("t<=3="); w_n(t<=3,10); w_s(" (0)\n"); 
   w_s("t<=5="); w_n(t<=5,10); w_s(" (1)\n"); 
   w_s("t<=-1="); w_n(t<=-1,10); w_s(" (0)\n"); 

   t = 4;
   w_s("t>4="); w_n(t>4,10); w_s(" (0)\n"); 
   w_s("t>3="); w_n(t>3,10); w_s(" (1)\n"); 
   w_s("t>5="); w_n(t>5,10); w_s(" (0)\n"); 
   w_s("t>-1="); w_n(t>-1,10); w_s(" (1)\n"); 

   w_s("t>=4="); w_n(t>=4,10); w_s(" (1)\n"); 
   w_s("t>=3="); w_n(t>=3,10); w_s(" (1)\n"); 
   w_s("t>=5="); w_n(t>=5,10); w_s(" (0)\n"); 
   w_s("t>=-1="); w_n(t>=-1,10); w_s(" (1)\n"); 
 

   pi = -100;
   w_s("pi<4="); w_n(pi<4,10); w_s(" (0)\n"); 
   w_s("pi<3="); w_n(pi<3,10); w_s(" (0)\n"); 
   w_s("pi<-100="); w_n(pi<-100,10); w_s(" (0)\n"); 
   w_s("pi<-1="); w_n(pi<-1,10); w_s(" (1)\n"); 

   w_s("pi<=4="); w_n(pi<=4,10); w_s(" (0)\n"); 
   w_s("pi<=3="); w_n(pi<=3,10); w_s(" (0)\n"); 
   w_s("pi<=-100="); w_n(pi<=-100,10); w_s(" (1)\n"); 
   w_s("pi<=-1="); w_n(pi<=-1,10); w_s(" (1)\n"); 

   pi = -100;
   w_s("pi>4="); w_n(pi>4,10); w_s(" (1)\n"); 
   w_s("pi>3="); w_n(pi>3,10); w_s(" (1)\n"); 
   w_s("pi>-100="); w_n(pi>-100,10); w_s(" (0)\n"); 
   w_s("pi>-1="); w_n(pi>-1,10); w_s(" (0)\n"); 

   w_s("pi>=4="); w_n(pi>=4,10); w_s(" (1)\n"); 
   w_s("pi>=3="); w_n(pi>=3,10); w_s(" (1)\n"); 
   w_s("pi>=-100="); w_n(pi>=-100,10); w_s(" (1)\n"); 
   w_s("pi>=-1="); w_n(pi>=-1,10); w_s(" (0)\n"); 

   w_s("switch test: ");
   switch(t)
   {
      case 3:
         w_s("failure");
         break;
      case 4:
         w_s("success");
         break;
      case 5:
         w_s("failure");
         break;
   }
   w_s("\n");

   w_s("switch fallthrough test: ");
   switch(t)
   {
      case 3:
         w_s("failure");
         break;
      case 4:
         w_s("OKSOFAR: ");
      case 5:
         w_s("success if oksofar printed before this in caps");
         break;
   }
   w_s("\n");
   
   pi = &arr[0];
   pip = &arr[3];
   w_s("*pip-*pi: "); w_n(*pip-*pi,10); w_s(" 30\n");
   w_s("pip-pi: "); w_n(pip-pi,10); w_s(" 3\n");
   w_s("*pip: "); w_n( *pip, 10 ); w_s(" 40\n");
   w_s("*(pip-3): "); w_n(*(pip-3),10); w_s(" 10\n");
   w_s("*&arr[3]: "); w_n( *&arr[3], 10 ); w_s(" 40\n");
   // The following causes an address error, so it has been removed
   // and needs to be fixed
   w_s("*(&arr[3]-3): "); w_n(0/**(&arr[3]-3)*/,10); w_s(" 10 broken - causes address error\n");
   exit(0);
}

printt(t,str)
int t;
char *str;
{
   w_s("bool t test on value "); 
   w_n(t,10);
   w_s(" ");
   w_s(str);
}
