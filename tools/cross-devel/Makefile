SUBDIR		= ar as ld nm ranlib size strip

all install depend: ${SUBDIR}
		-for i in ${SUBDIR}; do ${MAKE} -C $$i ${MFLAGS} DESTDIR=${DESTDIR} $@; done

clean:
		rm -f *~
		for i in ${SUBDIR}; do ${MAKE} -C $$i ${MFLAGS} clean; done
