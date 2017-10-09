/* BUMState.h - BUM-State data structure definitions and headers for helper
 *              functions.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 */

#ifndef __BUM_STATE__
#define __BUM_STATE__

#define BUMSTATE_VERSION (0)
#define BUMSTATE_CONFIG_MAXLEN (128)

typedef struct {
    UINT64  StateUpdateCounter;
    UINT64  StateSize;
    UINT64  Version;
    union {
        struct {
            UINT64  CurrConfig      : 1;
            #define BUMSTATE_CONFIG_DFLT (0)
            #define BUMSTATE_CONFIG_ALTR (1)
            UINT64  UpdateAttempt   : 1;
            UINT64  Reserved        : 62;
        };
        UINT64  raw;
    } Flags;
    UINT64  DfltAttemptCount;
    UINT64  DfltAttemptsRemaining;
    CHAR8   DfltConfig[BUMSTATE_CONFIG_MAXLEN];
    CHAR8   AltrConfig[BUMSTATE_CONFIG_MAXLEN];
    UINT64  Checksum;
} BUM_state_t;

EFI_STATUS EFIAPI BUMState_Init(IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                IN  CHAR8               *Config);

EFI_STATUS EFIAPI BUMState_Get( IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                OUT BUM_state_t         **BUM_state_pp);

EFI_STATUS EFIAPI BUMState_Put( IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                IN  BUM_state_t         *BUM_state_p);

EFI_STATUS EFIAPI BUMState_Free(IN  BUM_state_t *BUM_state_p);

VOID EFIAPI BUMStateNext_StartUpdate(IN  BUM_state_t *BUM_state_p);

EFI_STATUS EFIAPI BUMStateNext_CompleteUpdate(  IN  BUM_state_t *BUM_state_p,
                                                IN  UINT64      AttemptCount,
                                                IN  CHAR8       *UpdateConfig);

EFI_STATUS EFIAPI BUMStateNext_BootTime(IN  BUM_state_t *BUM_state_p);

EFI_STATUS EFIAPI BUMStateNext_RunTimeInit( IN  BUM_state_t *BUM_state_p);

EFI_STATUS EFIAPI BUMState_getCurrConfig(   IN  BUM_state_t *BUM_state_p,
                                            OUT CHAR8       **CurrConfig_pp);

EFI_STATUS EFIAPI BUMState_getNonCurrConfig(IN  BUM_state_t *BUM_state_p,
                                            OUT CHAR8       **CurrConfig_pp);

#endif
