/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 */
#include "defs.h"

/*
 * storage allocator
 * (circular first fit strategy)
 */

#define BUSY 01
#define busy(x)	(Rcheat((x)->word) & BUSY)

POS		brkincr = BRKINCR;
BLKPTR		blokp;			/*current search pointer*/
BLKPTR		bloktop = BLK(end) - 1;	/*top of arena (last blok)*/

ADDRESS	alloc(nbytes)
	POS		nbytes;
{
	REG POS		rbytes = round(nbytes + BYTESPERWORD, BYTESPERWORD);
	LOOP	INT		c = 0;
		REG BLKPTR	p = blokp;
		REG BLKPTR	q;
		REP
                        IF ! busy(p)
			THEN	LOOP
			                q = p->word;
                                        IF busy(q) THEN break FI
                                        p->word = q->word;
                                POOL
				IF ADR(q)-ADR(p) >= rbytes
				THEN	blokp = BLK(ADR(p)+rbytes);
					IF q > blokp
					THEN	blokp->word = p->word;
					FI
					p->word = BLK(Rcheat(blokp) | BUSY);
					return(ADR(p+1));
				FI
			FI
			q = p;
                        p = BLK(Rcheat(p->word) & ~BUSY);
		PER p > q ORF (c++) == 0 DONE
		addblok(rbytes);
	POOL
}

VOID	addblok(reqd)
	POS		reqd;
{
	IF stakbas != staktop
	THEN	REG STKPTR	rndstak;
		REG BLKPTR	blokstak;

		pushstak(0);
		rndstak = (STKPTR) round(staktop, BYTESPERWORD) - sizeof(int*);
		blokstak = BLK(stakbas) - 1;
		blokstak->word = stakbsy;
                stakbsy = blokstak;
		bloktop->word = BLK(Rcheat(rndstak) | BUSY);
		bloktop = BLK(rndstak);
	FI
	reqd += brkincr;
        reqd &= ~(brkincr - 1);
	blokp = bloktop;
	blokp->word = BLK(Rcheat(bloktop) + reqd);
	bloktop = blokp->word;
	bloktop->word = BLK(ADR(end) + 1);
	BEGIN
                REG STKPTR stakadr = STK(bloktop + 2);
                staktop = movstr(stakbot, stakadr);
                stakbas = stakbot = stakadr;
	END
}

VOID	free(ap)
	BLKPTR		ap;
{
	REG BLKPTR	p;

	p = ap;
	IF (p != 0) ANDF (p < bloktop)
	THEN
                Lcheat((--p)->word) &= ~BUSY;
	FI
}

prx(x)
	INT		x;
{
        static const char hex[16] = "0123456789abcdef";
	prc(hex[(x >> 28) & 15]);
	prc(hex[(x >> 24) & 15]);
	prc(hex[(x >> 20) & 15]);
	prc(hex[(x >> 16) & 15]);
	prc(hex[(x >> 12) & 15]);
	prc(hex[(x >> 8) & 15]);
	prc(hex[(x >> 4) & 15]);
	prc(hex[x & 15]);
}


#ifdef DEBUG
chkbptr(ptr)
	BLKPTR	ptr;
{
	INT		exf = 0;
	REG BLKPTR	p = end;
	REG BLKPTR	q;
	INT		us = 0, un = 0;

	LOOP
	   q = Rcheat(p->word)&~BUSY;
	   IF p == ptr THEN exf++ FI
	   IF q < end ORF q > bloktop THEN abort(3) FI
	   IF p == bloktop THEN break FI
	   IF busy(p)
	   THEN us += q - p;
	   ELSE un += q - p;
	   FI
	   IF p >= q THEN abort(4) FI
	   p = q;
	POOL
	IF exf == 0 THEN abort(1) FI
	prn(un);
        prc(SP);
        prn(us);
        prc(NL);
}
#endif
