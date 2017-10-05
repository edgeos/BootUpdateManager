/* LogPrint.c - Implement most BUMState logic.
 *              NOTE:   This code is meant to be compiled as a part of both
 *                      an EFI application and user-space utilities.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 */

/*  Need to include __BUMState.h as <> rather than "" to ensure the correct
    header file is included when compiling for user space instead of EFI space
*/
#include <__BUMState.h>

#define ASTATE_FILENAME L"A.state"
#define BSTATE_FILENAME L"B.state"

static EFI_STATUS CopyConfig(   OUT CHAR8 Dst[static BUMSTATE_CONFIG_MAXLEN],
                                IN  CHAR8 *Src)
{
    EFI_STATUS ret;
    UINTN len = AsciiStrnLenS(Src, BUMSTATE_CONFIG_MAXLEN);
    if(BUMSTATE_CONFIG_MAXLEN == len){
        ret = EFI_INVALID_PARAMETER;
    }else{
        AsciiStrCpy(Dst, Src);
        ret = EFI_SUCCESS;
    }
    return ret;
}

static UINT64 BUMState_GetSum(  IN  BUM_state_t *state_p)
{
    UINT64 *buffer, len, i, sum;
    len = state_p->StateSize / sizeof(UINT64);
    buffer = (UINT64*)state_p;
    for(i=0, sum=0; i<len; i++) sum += buffer[i];
    return sum;
}

static VOID BUMState_SetSum(IN  BUM_state_t *state_p)
{
    UINT64 sum;
    state_p->Checksum=0;
    sum = BUMState_GetSum(state_p);
    state_p->Checksum = -sum;
}

#define BUMState_SumValid(state_p) (0 == BUMState_GetSum(state_p))

EFI_STATUS BUMState_Init(   IN  EFI_FILE_PROTOCOL   *BootStatDir,
                            IN  CHAR8               *Config)
{
    EFI_STATUS ret;
    BUM_state_t state;
    /*  Zero-out state*/
    ZeroMem((VOID*)&state, sizeof(state));
    /*  Write to the B.state file */
    ret = Common_CreateWriteCloseFile(  BootStatDir,
                                        BSTATE_FILENAME,
                                        (VOID*)&state,
                                        0);
    if(EFI_ERROR(ret))
        goto exit0;
    /*  Populate the A.state file */
    state.StateUpdateCounter = 1;
    state.StateSize = sizeof(BUM_state_t);
    state.Version   = BUMSTATE_VERSION;
    /* state.Flags.raw = 0 */;
    /*  Always initializing to an unsafe boot state
            so we're in alternate
            and attempt count is 0
    */
    state.Flags.CurrConfig  = BUMSTATE_CONFIG_ALTR;
    state.DfltAttemptCount  = 0;
    /* state.DfltConfig[128] = {0}; */
    ret = CopyConfig(state.AltrConfig, Config);
    if(EFI_ERROR(ret))
        goto exit0;
    /*  Set state.AtomicityChecksum; */
    BUMState_SetSum(&state);
    /*  Write to the A.state file */
    ret = Common_CreateWriteCloseFile(  BootStatDir,
                                        ASTATE_FILENAME,
                                        (VOID*)&state,
                                        sizeof(state));
exit0:
    return ret;
}

#if 0
EFI_STATUS BUMState_Get(    IN  EFI_FILE_PROTOCOL   *BootStatDir,
                            OUT BUM_state_t         **BUM_state_pp);

EFI_STATUS BUMState_Put(IN  EFI_FILE_PROTOCOL   *BootStatDir,
                        IN  BUM_state_t         *BUM_state_p);

EFI_STATUS BUMStateNext_BootTime(   IN  BUM_state_t *BUM_state_p);

EFI_STATUS BUMStateNext_RunTimeInit(IN  BUM_state_t *BUM_state_p);

EFI_STATUS BUMStateNext_StartUpdate(IN  BUM_state_t *BUM_state_p);

EFI_STATUS BUMStateNext_CompleteUpdate( IN  BUM_state_t *BUM_state_p,
                                        IN  UINTN       AttemptCount,
                                        IN  CHAR8       *UpdateConfig);

EFI_STATUS BUMState_getCurrConfig(  IN  BUM_state_t *BUM_state_p,
                                    OUT CHAR8       **CurrConfig_pp);

EFI_STATUS BUMState_getNonCurrConfig(   IN  BUM_state_t *BUM_state_p,
                                        OUT CHAR8       **CurrConfig_pp);
#endif

