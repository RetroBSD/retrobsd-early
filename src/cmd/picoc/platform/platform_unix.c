#include "../picoc.h"
#include "../interpreter.h"

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* mark where to end the program for platforms which require this */
jmp_buf PicocExitBuf;

#ifndef NO_DEBUGGER
#include <signal.h>

static void BreakHandler(int Signal)
{
    PlatformPrintf("break\n");
    DebugManualBreak = TRUE;
}

void PlatformInit()
{
    /* capture the break signal and pass it to the debugger */
    signal(SIGINT, BreakHandler);
}
#else
void PlatformInit()
{
}
#endif

void PlatformCleanup()
{
}

/* get a line of interactive input */
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)
{
#ifdef USE_READLINE
    if (Prompt != NULL)
    {
        /* use GNU readline to read the line */
        char *InLine = readline(Prompt);
        if (InLine == NULL)
            return NULL;

        Buf[MaxLen] = '\0';
        strncpy(Buf, InLine, MaxLen-1);
        strncat(Buf, "\n", MaxLen-1);

        if (InLine[0] != '\0')
            add_history(InLine);

        free(InLine);
        return Buf;
    }
#endif

    if (Prompt != NULL)
        printf("%s", Prompt);

    fflush(stdout);
    return fgets(Buf, MaxLen, stdin);
}

/* get a character of interactive input */
int PlatformGetCharacter()
{
    fflush(stdout);
    return getchar();
}

/* write a character to the console */
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *Stream)
{
    putchar(OutCh);
}

/* read a file into memory */
char *PlatformReadFile(const char *FileName)
{
    struct stat FileInfo;
    char *ReadText;
    FILE *InFile;
    int BytesRead;
    char *p;

    if (stat(FileName, &FileInfo))
        ProgramFail(NULL, "can't read file %s\n", FileName);

    ReadText = malloc(FileInfo.st_size + 1);
    if (ReadText == NULL)
        ProgramFail(NULL, "out of memory\n");

    InFile = fopen(FileName, "r");
    if (InFile == NULL)
        ProgramFail(NULL, "can't read file %s\n", FileName);

    BytesRead = fread(ReadText, 1, FileInfo.st_size, InFile);
    if (BytesRead == 0)
        ProgramFail(NULL, "can't read file %s\n", FileName);

    ReadText[BytesRead] = '\0';
    fclose(InFile);

    if ((ReadText[0] == '#') && (ReadText[1] == '!'))
    {
        for (p = ReadText; (*p != '\r') && (*p != '\n'); ++p)
        {
            *p = ' ';
        }
    }

    return ReadText;
}

/* read and scan a file for definitions */
void PicocPlatformScanFile(const char *FileName)
{
    char *SourceStr = PlatformReadFile(FileName);

    /* ignore "#!/path/to/picoc" .. by replacing the "#!" with "//" */
    if (SourceStr != NULL && SourceStr[0] == '#' && SourceStr[1] == '!')
    {
        SourceStr[0] = '/';
        SourceStr[1] = '/';
    }

    PicocParse(FileName, SourceStr, strlen(SourceStr), TRUE, FALSE, TRUE, TRUE);
}

/* exit the program */
void PlatformExit(int RetVal)
{
    PicocExitValue = RetVal;
    longjmp(PicocExitBuf, 1);
}
