#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include "EFIGlue.h"
#include "EFIGlue.h"
#include "BUMState.h"

static int bootTimeTest(char *statusdir_name)
{
    int ret;
    EFI_FILE_PROTOCOL *statusdir;
    BUM_state_t       *BUM_state_p;
    EFI_STATUS stat;
    /*  Assume success */
    ret = 0;
    /*  Acquire the status directory */
    statusdir = GetDirFileProtocol(statusdir_name);
    /*  Acquire the BUM state */
    stat = BUMState_Get(statusdir, &BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Get failed\n");
        ret = -1;
        goto exit0;
    }
    /*  Perform the boot-time logic. */
    BUMStateNext_BootTime(BUM_state_p);
    /*  Save the BUM state */
    stat = BUMState_Put(statusdir, BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Put failed\n");
        ret = -1;
        /*goto exit1;*/
    }
/*exit1:*/
    /*  Free the state variable */
    stat = BUMState_Free(BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Free failed\n");
        ret = -1;
        /*goto exit0;*/
    }
exit0:
    return ret;
}

static const char *usage = "<BUM status directory>";

int main(int argc, char** argv)
{
    int ret;
    if(argc != 2){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        ret = -1;
    }else{
        ret = bootTimeTest(argv[1]);
        if(0 != ret)
            fprintf(stderr, "   bootTimeTest failed\n");
    }
    return ret;
}

