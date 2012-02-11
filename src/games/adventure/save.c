/*
 * save (III)   J. Gillogly
 * save user core image for restarting
 */
#include <unistd.h>
#include <fcntl.h>
#include "hdr.h"

void
save(savfile)                           /* save game state              */
char *savfile;
{
	int fd, i;
        struct travlist *entry;

	fd = creat(savfile, 0644);
	if (fd < 0) {
	        printf("Cannot write to %s\n", savfile);
		exit(0);
	}
        write(fd, &game, sizeof game);  /* write all game data          */

        for (i=0; i<LOCSIZ; i++) {      /* save travel lists            */
                entry = travel[i];
                if (! entry)
                        continue;
                while (entry != 0) {
                        /*printf("entry %d: next=%p conditions=%d tloc=%d tverb=%d\n",
                                i, entry->next, entry->conditions, entry->tloc, entry->tverb);*/
                        write(fd, entry, sizeof(*entry));
                        entry = entry->next;
                }
        }
        printf("Saved %lu bytes to %s\n", lseek(fd, 0, SEEK_CUR), savfile);
	close(fd);
}

int
restore(savfile)                        /* restore game state           */
char *savfile;
{
	int fd, i;
        struct travlist **entryp;

	fd = open(savfile, O_RDONLY);
	if (fd < 0)
	        return 0;
		                        /* read all game data           */
        if (read(fd, &game, sizeof game) != sizeof game) {
failed:         close(fd);
	        return 0;
        }

        for (i=0; i<LOCSIZ; i++) {      /* restore travel lists         */
                if (! travel[i])
                        continue;
                entryp = &travel[i];
                while (*entryp != 0) {
                        *entryp = (struct travlist*) malloc(sizeof(**entryp));
                        if (! *entryp)
                                goto failed;
                        if (read(fd, *entryp, sizeof **entryp) != sizeof **entryp)
                                goto failed;
                        /*printf("entry %d: next=%p conditions=%d tloc=%d tverb=%d\n",
                                i, (*entryp)->next, (*entryp)->conditions,
                                (*entryp)->tloc, (*entryp)->tverb);*/
                        entryp = &(*entryp)->next;
                }
        }
	close(fd);
        return 1;
}
