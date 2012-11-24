#include "pass2.h"
static int op0[] = { -1 };
static int op1[] = { -1 };
static int op2[] = { 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, -1 };
static int op3[] = { -1 };
static int op4[] = { 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, -1 };
static int op5[] = { 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, -1 };
static int op6[] = { 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, -1 };
static int op7[] = { 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, -1 };
static int op8[] = { 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, -1 };
static int op9[] = { -1 };
static int op10[] = { 83, 84, 85, 86, 87, 88, 89, 100, 101, 102, -1 };
static int op11[] = { 90, 91, 92, 93, 94, 95, 100, 101, 102, -1 };
static int op12[] = { 75, 76, 77, 78, 79, -1 };
static int op13[] = { 80, 81, 82, -1 };
static int op14[] = { 70, 71, 72, 73, 74, -1 };
static int op15[] = { 100, 101, 102, -1 };
static int op16[] = { 100, 101, 102, -1 };
static int op17[] = { 100, 101, 102, -1 };
static int op18[] = { 105, 108, 110, 112, -1 };
static int op19[] = { 103, 104, 106, 107, 109, 111, -1 };
static int op20[] = { 113, 114, -1 };
static int op21[] = { -1 };
static int op22[] = { -1 };
static int op23[] = { 178, 179, 180, 181, 182, 183, 184, -1 };
static int op24[] = { 96, 97, 98, 99, -1 };
static int op25[] = { 129, 131, 132, 133, 134, 135, -1 };
static int op26[] = { 130, 131, 132, 133, 134, 135, -1 };
static int op27[] = { 131, 132, 133, 134, 135, -1 };
static int op28[] = { 131, 132, 133, 134, 135, -1 };
static int op29[] = { 131, 132, 133, 134, 135, -1 };
static int op30[] = { 131, 132, 133, 134, 135, -1 };
static int op31[] = { 131, 132, 133, 134, 135, -1 };
static int op32[] = { 131, 132, 133, 134, 135, -1 };
static int op33[] = { 131, 132, 133, 134, 135, -1 };
static int op34[] = { 131, 132, 133, 134, 135, -1 };
static int op35[] = { -1 };
static int op36[] = { 185, -1 };
static int op37[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, -1 };
static int op38[] = { 1, -1 };
static int op39[] = { -1 };
static int op40[] = { -1 };
static int op41[] = { 153, 155, 157, 159, 161, 163, 165, 167, -1 };
static int op42[] = { 154, 156, 158, 160, 162, 164, 166, 168, -1 };
static int op43[] = { -1 };
static int op44[] = { -1 };
static int op45[] = { 173, 174, 175, 176, -1 };
static int op46[] = { 169, 170, 171, 172, -1 };
static int op47[] = { -1 };
static int op48[] = { -1 };
static int op49[] = { 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, -1 };
static int op50[] = { 128, -1 };
static int op51[] = { 177, -1 };
static int op52[] = { -1 };
static int op53[] = { -1 };
static int op54[] = { 152, -1 };
static int op55[] = { -1 };
static int op56[] = { -1 };
static int op57[] = { -1 };
static int op58[] = { -1 };

int *qtable[] = { 
	op0,
	op1,
	op2,
	op3,
	op4,
	op5,
	op6,
	op7,
	op8,
	op9,
	op10,
	op11,
	op12,
	op13,
	op14,
	op15,
	op16,
	op17,
	op18,
	op19,
	op20,
	op21,
	op22,
	op23,
	op24,
	op25,
	op26,
	op27,
	op28,
	op29,
	op30,
	op31,
	op32,
	op33,
	op34,
	op35,
	op36,
	op37,
	op38,
	op39,
	op40,
	op41,
	op42,
	op43,
	op44,
	op45,
	op46,
	op47,
	op48,
	op49,
	op50,
	op51,
	op52,
	op53,
	op54,
	op55,
	op56,
	op57,
	op58,
};
int tempregs[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, -1 };
int permregs[] = { 16, 17, 18, 19, 20, 21, 22, 23, -1 };
bittype validregs[] = {
	0x03fffffc,
	0xffffffff,
};
static int amap[MAXREGS][NUMCLASS] = {
	/* 0 */{ 0x0,0x0,0x0 },
	/* 1 */{ 0x0,0x0,0x0 },
	/* 2 */{ 0x1,0x1,0x0 },
	/* 3 */{ 0x2,0x1,0x0 },
	/* 4 */{ 0x4,0x2,0x0 },
	/* 5 */{ 0x8,0x6,0x0 },
	/* 6 */{ 0x10,0xc,0x0 },
	/* 7 */{ 0x20,0x18,0x0 },
	/* 8 */{ 0x40,0x30,0x0 },
	/* 9 */{ 0x80,0x60,0x0 },
	/* 10 */{ 0x100,0xc0,0x0 },
	/* 11 */{ 0x200,0x180,0x0 },
	/* 12 */{ 0x400,0x300,0x0 },
	/* 13 */{ 0x800,0x600,0x0 },
	/* 14 */{ 0x1000,0x1800,0x0 },
	/* 15 */{ 0x2000,0x3000,0x0 },
	/* 16 */{ 0x4000,0x4000,0x0 },
	/* 17 */{ 0x8000,0xc000,0x0 },
	/* 18 */{ 0x10000,0x18000,0x0 },
	/* 19 */{ 0x20000,0x30000,0x0 },
	/* 20 */{ 0x40000,0x60000,0x0 },
	/* 21 */{ 0x80000,0xc0000,0x0 },
	/* 22 */{ 0x100000,0x180000,0x0 },
	/* 23 */{ 0x200000,0x100000,0x0 },
	/* 24 */{ 0x400000,0x3000,0x0 },
	/* 25 */{ 0x800000,0x2000,0x0 },
	/* 26 */{ 0x0,0x0,0x0 },
	/* 27 */{ 0x0,0x0,0x0 },
	/* 28 */{ 0x0,0x0,0x0 },
	/* 29 */{ 0x0,0x0,0x0 },
	/* 30 */{ 0x0,0x0,0x0 },
	/* 31 */{ 0x0,0x0,0x0 },
	/* 32 */{ 0x3,0x1,0x0 },
	/* 33 */{ 0xc,0x6,0x0 },
	/* 34 */{ 0x18,0xe,0x0 },
	/* 35 */{ 0x30,0x1c,0x0 },
	/* 36 */{ 0x60,0x38,0x0 },
	/* 37 */{ 0xc0,0x70,0x0 },
	/* 38 */{ 0x180,0xe0,0x0 },
	/* 39 */{ 0x300,0x1c0,0x0 },
	/* 40 */{ 0x600,0x380,0x0 },
	/* 41 */{ 0xc00,0x700,0x0 },
	/* 42 */{ 0x1800,0xe00,0x0 },
	/* 43 */{ 0x3000,0x1c00,0x0 },
	/* 44 */{ 0x402000,0x3800,0x0 },
	/* 45 */{ 0xc00000,0x3000,0x0 },
	/* 46 */{ 0xc000,0xc000,0x0 },
	/* 47 */{ 0x18000,0x1c000,0x0 },
	/* 48 */{ 0x30000,0x38000,0x0 },
	/* 49 */{ 0x60000,0x70000,0x0 },
	/* 50 */{ 0xc0000,0xe0000,0x0 },
	/* 51 */{ 0x180000,0x1c0000,0x0 },
	/* 52 */{ 0x300000,0x180000,0x0 },
	/* 53 */{ 0x0,0x0,0x1 },
	/* 54 */{ 0x0,0x0,0x2 },
	/* 55 */{ 0x0,0x0,0x4 },
	/* 56 */{ 0x0,0x0,0x8 },
	/* 57 */{ 0x0,0x0,0x10 },
	/* 58 */{ 0x0,0x0,0x20 },
	/* 59 */{ 0x0,0x0,0x40 },
	/* 60 */{ 0x0,0x0,0x80 },
	/* 61 */{ 0x0,0x0,0x100 },
	/* 62 */{ 0x0,0x0,0x200 },
	/* 63 */{ 0x0,0x0,0x400 },
};
int
aliasmap(int class, int regnum)
{
	return amap[regnum][class-1];
}
static int rmap[NUMCLASS][24] = {
	{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, },
	{ 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, },
};

int
color2reg(int color, int class)
{
	return rmap[class-1][color];
}
int regK[] = { 0, 24, 21, 11, 0, 0, 0, 0 };
int
classmask(int class)
{
	if(class == CLASSA) return 0xffffff;
	if(class == CLASSB) return 0x1fffff;
	if(class == CLASSC) return 0x7ff;
	if(class == CLASSD) return 0x0;
	if(class == CLASSE) return 0x0;
	if(class == CLASSF) return 0x0;
	return 0x0;
}
static bittype ovlarr[MAXREGS][2] = {
{ 0x1, 0x0,  },
{ 0x2, 0x0,  },
{ 0x4, 0x1,  },
{ 0x8, 0x1,  },
{ 0x10, 0x2,  },
{ 0x20, 0x6,  },
{ 0x40, 0xc,  },
{ 0x80, 0x18,  },
{ 0x100, 0x30,  },
{ 0x200, 0x60,  },
{ 0x400, 0xc0,  },
{ 0x800, 0x180,  },
{ 0x1000, 0x300,  },
{ 0x2000, 0x600,  },
{ 0x4000, 0x1800,  },
{ 0x8000, 0x3000,  },
{ 0x10000, 0x4000,  },
{ 0x20000, 0xc000,  },
{ 0x40000, 0x18000,  },
{ 0x80000, 0x30000,  },
{ 0x100000, 0x60000,  },
{ 0x200000, 0xc0000,  },
{ 0x400000, 0x180000,  },
{ 0x800000, 0x100000,  },
{ 0x1000000, 0x3000,  },
{ 0x2000000, 0x2000,  },
{ 0x4000000, 0x0,  },
{ 0x8000000, 0x0,  },
{ 0x10000000, 0x0,  },
{ 0x20000000, 0x0,  },
{ 0x40000000, 0x0,  },
{ 0x80000000, 0x0,  },
{ 0xc, 0x1,  },
{ 0x30, 0x6,  },
{ 0x60, 0xe,  },
{ 0xc0, 0x1c,  },
{ 0x180, 0x38,  },
{ 0x300, 0x70,  },
{ 0x600, 0xe0,  },
{ 0xc00, 0x1c0,  },
{ 0x1800, 0x380,  },
{ 0x3000, 0x700,  },
{ 0x6000, 0xe00,  },
{ 0xc000, 0x1c00,  },
{ 0x1008000, 0x3800,  },
{ 0x3000000, 0x3000,  },
{ 0x30000, 0xc000,  },
{ 0x60000, 0x1c000,  },
{ 0xc0000, 0x38000,  },
{ 0x180000, 0x70000,  },
{ 0x300000, 0xe0000,  },
{ 0x600000, 0x1c0000,  },
{ 0xc00000, 0x180000,  },
{ 0x0, 0x200000,  },
{ 0x0, 0x400000,  },
{ 0x0, 0x800000,  },
{ 0x0, 0x1000000,  },
{ 0x0, 0x2000000,  },
{ 0x0, 0x4000000,  },
{ 0x0, 0x8000000,  },
{ 0x0, 0x10000000,  },
{ 0x0, 0x20000000,  },
{ 0x0, 0x40000000,  },
{ 0x0, 0x80000000,  },
};
int
interferes(int reg1, int reg2)
{
return (TESTBIT(ovlarr[reg1], reg2)) != 0;
}
