#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>

#include "EFIGlue.h"
#include "BUMState.h"

static const char *usage = "<BUM status directory> <starting config name>";

int main(int argc, char** argv)
{
    int ret;
    EFI_FILE_PROTOCOL *statusdir;
    char *configname;
    EFI_STATUS stat;

    if(argc != 3){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        ret = -1;
    }else{
        statusdir = GetDirFileProtocol(argv[1]);
        configname = argv[2];
        stat = BUMState_Init(statusdir, configname);
        if(EFI_ERROR(stat)){
            fprintf(stderr, "   BUMState_Init failed\n");
            ret = -1;
        }else
            ret = 0;
    }
    return ret;
}

