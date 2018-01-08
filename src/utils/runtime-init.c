#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include "EFIGlue.h"
#include "EFIGlue.h"
#include "BUMState.h"

/*  seems redundant, but defining as macros to ensure
    error-reporting consistency */
#define BOOTSUCCESS "BOOTSUCCESS"
#define UPDTSUCCESS "UPDATESUCCESS"
#define BOOTFAILURE "BOOTFAILURE"
#define UPDTFAILURE "UPDATEFAILURE"
#define FATALERROR  "FATALERROR"

static char* getBootStatusString(BUM_state_t *BUM_state_p)
{
    char *StatusString;
    /*  Check for failure: booting alternate */
    if(BUMSTATE_CONFIG_ALTR == BUM_state_p->Flags.CurrConfig){
        /*  Check if this was a failed update */
        if(1 == BUM_state_p->Flags.UpdateAttempt)
            StatusString = UPDTFAILURE;
        else
            StatusString = BOOTFAILURE;
    }else{ /*Success: booted default */
        /*  Check if this was a successful update */
        if(1 == BUM_state_p->Flags.UpdateAttempt)
            StatusString = UPDTSUCCESS;
        else
            StatusString = BOOTSUCCESS;
    }
    return StatusString;
}

static int runTimeInit( char *statedir_name,
                        char **StatusString_p)
{
    int ret;
    BUM_state_t       *BUM_state_p;
    char *StatusString;
    EFI_STATUS stat;
    /*  Assume success */
    ret = 0;
    /*  Acquire the BUM state */
    stat = BUMState_Get(statedir_name, &BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Get failed\n");
        StatusString = FATALERROR;
        ret = -1;
        goto exit0;
    }
    /*  Get boot status */
    StatusString = getBootStatusString(BUM_state_p);
    /*  Perform the run-time logic. */
    BUMStateNext_RunTimeInit(BUM_state_p);
    /*  Save the BUM state */
    stat = BUMState_Put(statedir_name, BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Put failed\n");
        StatusString = FATALERROR;
        ret = -1;
        /*goto exit1;*/
    }
/*exit1:*/
    /*  Free the state variable */
    stat = BUMState_Free(BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Free failed\n");
        StatusString = FATALERROR;
        ret = -1;
        /*goto exit0;*/
    }
exit0:
    *StatusString_p = StatusString;
    return ret;
}

static const char *usage = "<BUM state directory>";

int main(int argc, char** argv)
{
    int ret;
    char *StatusString;
    if(argc != 2){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        StatusString = FATALERROR;
        ret = -1;
    }else{
        ret = runTimeInit(argv[1], &StatusString);
    }
    printf("%s", StatusString);
    return ret;
}

