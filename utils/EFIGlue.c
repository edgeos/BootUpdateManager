
#include <stdint.h>
#include <stdlib.h>
#include <uchar.h>
#include "EFIGlue.h"

#include <string.h>

UINTN EFIAPI AsciiStrnLenS( IN CONST CHAR8  *String,
                            IN UINTN        MaxSize)
{
    UINTN i;
    for(i=0; (i<MaxSize) && (String[i]!='\0'); i++);
    return i;
}

CHAR8* EFIAPI AsciiStrCpy(  OUT CHAR8       *Destination,
                            IN  CONST CHAR8 *Source)
{
    return strcpy(Destination, Source);
}

VOID* EFIAPI ZeroMem(   OUT VOID    *Buffer,
                        IN  UINTN   Length)
{
    bzero(Buffer, (size_t)Length);
    return Buffer;
}

INTN EFIAPI CompareMem( IN CONST VOID  *DestinationBuffer,
                        IN CONST VOID  *SourceBuffer,
                        IN UINTN       Length)
{
    return (INTN)memcmp(DestinationBuffer, SourceBuffer, (size_t)Length);
}

