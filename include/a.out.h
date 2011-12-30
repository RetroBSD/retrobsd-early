/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef	_AOUT_H_
#define	_AOUT_H_

#include <sys/exec.h>

/* Valid magic number check. */
#define	N_BADMAG(x) 		(((x).a_magic) != OMAGIC && \
				 ((x).a_magic) != XMAGIC && \
				 ((x).a_magic) != NMAGIC)

/* Text segment offset. */
#define	N_TXTOFF(x) 		sizeof(struct exec)

/* Data segment offset. */
#define	N_DATOFF(x) 		(N_TXTOFF(x) + (x).a_text)

/* Text relocation table offset. */
#define	N_TRELOFF(x) 		(N_DATOFF(x) + (x).a_data)

/* Data relocation table offset. */
#define	N_DRELOFF(x) 		(N_TRELOFF(x) + (x).a_reltext)

/* Symbol table offset. */
#define	N_SYMOFF(x) 		((x).a_magic == OMAGIC ? \
                                    N_DRELOFF(x) + (x).a_reldata : \
                                    N_DATOFF(x) + (x).a_data)

#define	_AOUT_INCLUDE_
#include <nlist.h>

/* Relocations */
#define RSMASK  0x70            /* bitmask for segments */
#define RABS    0
#define RTEXT   0x20
#define RDATA   0x30
#define RBSS    0x40
#define RSTRNG  0x60            /* for assembler */
#define REXT    0x70            /* externals and bitmask */

#define RFMASK  0x07            /* bitmask for format */
#define RHIGH16 0x02            /* upper part of byte address: bits 31:16 */
#define RWORD16 0x03            /* word address: bits 17:2 */
#define RWORD26 0x04            /* word address: bits 27:2 */

#define RINDEX(h)       ((h) >> 8)
#define RSETINDEX(h)    ((h) << 8)

#endif	/* !_AOUT_H_ */
