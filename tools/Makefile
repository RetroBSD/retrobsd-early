SUBDIR		= elf2aout fsutil virtualmips mkrd

all install depend: ${SUBDIR}
		-for i in ${SUBDIR}; do ${MAKE} -C $$i ${MFLAGS} DESTDIR=${DESTDIR} $@; done

clean:
		rm -f *~
		for i in ${SUBDIR}; do ${MAKE} -C $$i ${MFLAGS} clean; done
