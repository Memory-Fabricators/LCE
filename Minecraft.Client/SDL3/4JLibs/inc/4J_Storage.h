#pragma once

#include <vector>

class C4JStringTable;

#define MAX_DISPLAYNAME_LENGTH 128
#define MAX_DETAILS_LENGTH 128
#define MAX_SAVEFILENAME_LENGTH 32

typedef struct
{
    time_t modifiedTime;
    unsigned int dataSize;
    unsigned int thumbnailSize;
} CONTAINER_METADATA;

typedef struct
{
    char UTF8SaveFilename[MAX_SAVEFILENAME_LENGTH];
    char UTF8SaveTitle[MAX_DISPLAYNAME_LENGTH];
    CONTAINER_METADATA metaData;
    PBYTE thumbnailData;
} SAVE_INFO, *PSAVE_INFO;

typedef struct
{
    int iSaveC;
    PSAVE_INFO SaveInfoA;
} SAVE_DETAILS, *PSAVE_DETAILS;

typedef std::vector<PXMARKETPLACE_CONTENTOFFER_INFO> OfferDataArray;
typedef std::vector<PXCONTENT_DATA> XContentDataArray;

#define CURRENT_DLC_VERSION_NUM 3

class C4JStorage
{
  public:
    typedef struct
    {
        unsigned int uiFileSize;
        DWORD dwType;
        DWORD dwWchCount;
        WCHAR wchFile[1];
    } DLC_FILE_DETAILS, *PDLC_FILE_DETAILS;

    typedef struct
    {
        DWORD dwType;
        DWORD dwWchCount;
        WCHAR wchData[1];
    } DLC_FILE_PARAM, *PDLC_FILE_PARAM;

    typedef struct
    {
        WCHAR wchDisplayName[XCONTENT_MAX_DISPLAYNAME_LENGTH];
        CHAR szFileName[XCONTENT_MAX_FILENAME_LENGTH];
        DWORD dwImageOffset;
        DWORD dwImageBytes;
    } CACHEINFOSTRUCT;

    typedef struct
    {
        DWORD dwVersion;
        DWORD dwNewOffers;
        DWORD dwTotalOffers;
        DWORD dwInstalledTotalOffers;
        BYTE bPadding[1024 - sizeof(DWORD) * 4];
    } DLC_TMS_DETAILS;

    enum eGTS_FileTypes
    {
        eGTS_Type_Skin = 0,
        eGTS_Type_Cape,
        eGTS_Type_MAX
    };

    enum eGlobalStorage
    {
        eGlobalStorage_Title = 0,
        eGlobalStorage_TitleUser,
        eGlobalStorage_Max
    };

    enum EMessageResult
    {
        EMessage_Undefined = 0,
        EMessage_Busy,
        EMessage_Pending,
        EMessage_Cancelled,
        EMessage_ResultAccept,
        EMessage_ResultDecline,
        EMessage_ResultThirdOption,
        EMessage_ResultFourthOption
    };

    enum ESaveGameControlState
    {
        ESaveGameControl_Idle = 0,
        ESaveGameControl_Save,
        ESaveGameControl_InternalRequestingDevice,
        ESaveGameControl_InternalGetSaveName,
        ESaveGameControl_InternalSaving,
        ESaveGameControl_CopySave,
        ESaveGameControl_CopyingSave,
    };

    enum ESaveGameState
    {
        ESaveGame_Idle = 0,
        ESaveGame_Save,
        ESaveGame_InternalRequestingDevice,
        ESaveGame_InternalGetSaveName,
        ESaveGame_InternalSaving,
        ESaveGame_CopySave,
        ESaveGame_CopyingSave,
        ESaveGame_Load,
        ESaveGame_GetSavesInfo,
        ESaveGame_Rename,
        ESaveGame_Delete,
        ESaveGame_GetSaveThumbnail
    };

    enum ELoadGameStatus
    {
        ELoadGame_Idle = 0,
        ELoadGame_InProgress,
        ELoadGame_NoSaves,
        ELoadGame_ChangedDevice,
        ELoadGame_DeviceRemoved
    };

    enum EDeleteGameStatus
    {
        EDeleteGame_Idle = 0,
        EDeleteGame_InProgress,
    };

    enum ESGIStatus
    {
        ESGIStatus_Error = 0,
        ESGIStatus_Idle,
        ESGIStatus_ReadInProgress,
        ESGIStatus_NoSaves,
    };

    enum EDLCStatus
    {
        EDLC_Error = 0,
        EDLC_Idle,
        EDLC_NoOffers,
        EDLC_AlreadyEnumeratedAllOffers,
        EDLC_NoInstalledDLC,
        EDLC_Pending,
        EDLC_LoadInProgress,
        EDLC_Loaded,
        EDLC_ChangedDevice
    };

    enum ESavingMessage
    {
        ESavingMessage_None = 0,
        ESavingMessage_Short,
        ESavingMessage_Long
    };

    enum ETMSStatus
    {
        ETMSStatus_Idle = 0,
        ETMSStatus_Fail,
        ETMSStatus_Fail_ReadInProgress,
        ETMSStatus_Fail_WriteInProgress,
        ETMSStatus_Pending,
    };

    enum eTMS_FileType
    {
        eTMS_FileType_Normal = 0,
        eTMS_FileType_Graphic,
    };

    enum eTMS_FILETYPEVAL
    {
        TMS_FILETYPE_BINARY,
        TMS_FILETYPE_CONFIG,
        TMS_FILETYPE_JSON,
        TMS_FILETYPE_MAX
    };

    enum eTMS_UGCTYPE
    {
        TMS_UGCTYPE_NONE,
        TMS_UGCTYPE_IMAGE,
        TMS_UGCTYPE_MAX
    };

    typedef struct
    {
        CHAR szFilename[256];
        int iFileSize;
        eTMS_FILETYPEVAL eFileTypeVal;
    } TMSPP_FILE_DETAILS, *PTMSPP_FILE_DETAILS;

    typedef struct
    {
        int iCount;
        PTMSPP_FILE_DETAILS FileDetailsA;
    } TMSPP_FILE_LIST, *PTMSPP_FILE_LIST;

    typedef struct
    {
        DWORD dwSize;
        PBYTE pbData;
    } TMSPP_FILEDATA, *PTMSPP_FILEDATA;

    // Core save game
    void Init(const wchar_t *, const char *, DWORD, void (*)(LPVOID, bool), LPVOID);
    void ResetSaveData();
    void SetSaveTitle(const wchar_t *);
    void StoreTMSPathName();
    void SetDLCPackageRoot(const char *);
    void RegisterMarketplaceCountsCallback(LPVOID, LPVOID);
    void Tick();
    bool LoadContent(int, XCONTENT_DATA &);
    bool SaveContent(int, XCONTENT_DATA &, void (*)(LPVOID, bool), LPVOID);
    bool EraseContent(int, XCONTENT_DATA &);
    void GetSaveDetails(int, SAVE_DETAILS *);
    void GetDeviceDetails(int, DWORD *, void *, int *);
    int GetStorageDevices(int, bool);
    bool GetSaveUniqueNumber(INT *piVal);
    unsigned int GetSaveSize();
    void GetSaveData(void *pvData, unsigned int *puiBytes);
    PVOID AllocateSaveData(unsigned int uiBytes);
    bool GetSaveDisabled();
    C4JStorage::ESaveGameState GetSaveState();

    void SetSaveImages(PBYTE pbThumbnail, DWORD dwThumbnailBytes, PBYTE pbImage, DWORD dwImageBytes, PBYTE pbTextData, DWORD dwTextDataBytes);
    C4JStorage::ESaveGameState SaveSaveData(int (*Func)(LPVOID, const bool), LPVOID lpParam);

    // Subfile-based split save
    int AddSubfile(unsigned int subfileId);
    unsigned int GetSubfileCount();
    void GetSubfileDetails(int idx, unsigned int *subfileId, unsigned char **data, unsigned int *sizeOut);
    void ResetSubfiles();
    void UpdateSubfile(int idx, unsigned char *data, unsigned int size);
    C4JStorage::ESaveGameState SaveSubfiles(int (*Func)(LPVOID, const bool), LPVOID lpParam);

    DWORD MountInstalledDLC(int iPad, DWORD dwDLC, int (*Func)(LPVOID, int, DWORD, DWORD), LPVOID lpParam, LPCSTR szMountDrive = NULL);
    DWORD UnmountInstalledDLC(LPCSTR szMountDrive = NULL);
    bool GetSaveUniqueFilename(char *pszName);
    C4JStorage::ESaveGameState DoesSaveExist(bool *pbExists);
};

extern C4JStorage StorageManager;
