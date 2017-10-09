#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include <string.h>
#include <errno.h>
#include "EFIGlue.h"

#include "EFIGlue.h"
#include "BUMState.h"

static int updateComplete(  char        *statusdir_name,
                            uint64_t    attemptcount,
                            char        *updateconfig)
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
    /*  Perform the complete-update logic. */
    stat = BUMStateNext_CompleteUpdate( BUM_state_p,
                                        attemptcount,
                                        updateconfig);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMStateNext_CompleteUpdate failed\n");
        ret = -1;
        goto exit1;
    }
    /*  Save the BUM state */
    stat = BUMState_Put(statusdir, BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Put failed\n");
        ret = -1;
        /*goto exit1;*/
    }
    /*  Free the state variable */
exit1:
    stat = BUMState_Free(BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Free failed\n");
        ret = -1;
        /*goto exit0;*/
    }
exit0:
    return ret;
}

static int strtou64_strict( char        *str,
                            uint64_t    *value_p)
{
    size_t len;
    int ret;
    char *endptr;
    unsigned long long value;
    len = strlen(str);
    if(len == 0){
        fprintf(stderr, "    strtou64_strict: non-empty string expected\n");
        ret = -1;
    }else{
        errno = 0;
        value = strtoull(str, &endptr, 0);
        if(0 != errno){
            perror("    strtou64_strict: strtoull failed");
            ret = -1;
        }else{
            if('\0' != *endptr){
                fprintf(stderr, "    strtou64_strict: "\
                                "string contains non-digit characters\n");
                ret = -1;
            }else{
                *value_p = (uint64_t)value;
                ret = 0;
            }
        }
    }
    return ret;
}

static const char *usage =  "<BUM status directory> <default attempt count> "\
                            "[<new configuration>]";

int main(int argc, char** argv)
{
    int ret;
    uint64_t attemptcount;
    char *updateconfig;
    if((argc < 3) || (argc > 4)){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        ret = -1;
    }else{
        /*  Check if the (optional) new-configuration name is present */
        if(4 == argc)
            updateconfig = argv[3];
        else
            updateconfig = NULL;
        /*  Parse the attempt count */
        ret = strtou64_strict( argv[2],
                               &attemptcount);
        if(0 != ret){
            fprintf(stderr, "    strtou64_strict failed\n");
        }else{
            ret = updateComplete(   argv[1],
                                    attemptcount,
                                    updateconfig);
            if(0 != ret)
                fprintf(stderr, "    updateComplete failed\n");
        }
    }
    return ret;
}

