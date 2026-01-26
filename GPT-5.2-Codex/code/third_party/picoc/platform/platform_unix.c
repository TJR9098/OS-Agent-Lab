#include "../picoc.h"
#include "../interpreter.h"

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* mark where to end the program for platforms which require this */
jmp_buf PicocExitBuf;

#define MAX_INCLUDE_PATHS 8

static const char *g_include_paths[MAX_INCLUDE_PATHS];
static int g_include_path_count;

void PlatformAddIncludePath(const char *path)
{
    if (path == NULL || path[0] == '\0')
        return;
    if (g_include_path_count < MAX_INCLUDE_PATHS)
        g_include_paths[g_include_path_count++] = path;
}

#ifndef NO_DEBUGGER
#include <signal.h>

Picoc *break_pc = NULL;

static void BreakHandler(int Signal)
{
    break_pc->DebugManualBreak = TRUE;
}

void PlatformInit(Picoc *pc)
{
    /* capture the break signal and pass it to the debugger */
    break_pc = pc;
    signal(SIGINT, BreakHandler);
}
#else
void PlatformInit(Picoc *pc)
{
}
#endif

void PlatformCleanup(Picoc *pc)
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
    
        Buf[MaxLen-1] = '\0';
        strncpy(Buf, InLine, MaxLen-2);
        strncat(Buf, "\n", MaxLen-2);
        
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
char *PlatformReadFile(Picoc *pc, const char *FileName)
{
    struct stat FileInfo;
    char *ReadText;
    FILE *InFile;
    int BytesRead;
    char *p;
    const char *OpenName = FileName;
    char PathBuf[256];
    
    if (stat(FileName, &FileInfo))
    {
        OpenName = NULL;
        if (FileName[0] != '/')
        {
            int i;
            for (i = 0; i < g_include_path_count; i++)
            {
                int n = snprintf(PathBuf, sizeof(PathBuf), "%s/%s", g_include_paths[i], FileName);
                if (n > 0 && n < (int)sizeof(PathBuf) && stat(PathBuf, &FileInfo) == 0)
                {
                    OpenName = PathBuf;
                    break;
                }
            }
        }
        if (OpenName == NULL)
            ProgramFailNoParser(pc, "can't read file %s\n", FileName);
    }
    
    ReadText = malloc(FileInfo.st_size + 1);
    if (ReadText == NULL)
        ProgramFailNoParser(pc, "out of memory\n");
        
    InFile = fopen(OpenName, "r");
    if (InFile == NULL)
        ProgramFailNoParser(pc, "can't read file %s\n", OpenName);
    
    BytesRead = fread(ReadText, 1, FileInfo.st_size, InFile);
    if (BytesRead == 0)
        ProgramFailNoParser(pc, "can't read file %s\n", FileName);

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
void PicocPlatformScanFile(Picoc *pc, const char *FileName)
{
    char *SourceStr = PlatformReadFile(pc, FileName);

    /* ignore "#!/path/to/picoc" .. by replacing the "#!" with "//" */
    if (SourceStr != NULL && SourceStr[0] == '#' && SourceStr[1] == '!') 
    { 
        SourceStr[0] = '/'; 
        SourceStr[1] = '/'; 
    }

    PicocParse(pc, FileName, SourceStr, strlen(SourceStr), TRUE, FALSE, TRUE, TRUE);
}

/* exit the program */
void PlatformExit(Picoc *pc, int RetVal)
{
    pc->PicocExitValue = RetVal;
    longjmp(pc->PicocExitBuf, 1);
}
