#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H

typedef struct _PF_SCENARIO_ID
{
    WCHAR ScenName[30];
    ULONG HashId;
} PF_SCENARIO_ID, *PPF_SCENARIO_ID;

typedef struct _PF_LOG_ENTRY
{
    ULONG FileOffset:30;
    ULONG Type:2;
    union
    {
        ULONG FileKey;
        ULONG FileSequenceNumber;
    };
} PF_LOG_ENTRY, *PPF_LOG_ENTRY;

typedef struct _PFSN_LOG_ENTRIES
{
    LIST_ENTRY TraceBuffersLink;
    LONG NumEntries;
    LONG MaxEntries;
    PF_LOG_ENTRY Entries[ANYSIZE_ARRAY];
} PFSN_LOG_ENTRIES, *PPFSN_LOG_ENTRIES;

typedef struct _PF_SECTION_INFO
{
    ULONG FileKey;
    ULONG FileSequenceNumber;
    ULONG FileIdLow;
    ULONG FileIdHigh;
} PF_SECTION_INFO, *PPF_SECTION_INFO;

typedef struct _PF_TRACE_HEADER
{
    ULONG Version;
    ULONG MagicNumber;
    ULONG Size;
    PF_SCENARIO_ID ScenarioId;
    ULONG ScenarioType; // PF_SCENARIO_TYPE
    ULONG EventEntryIdxs[8];
    ULONG NumEventEntryIdxs;
    ULONG TraceBufferOffset;
    ULONG NumEntries;
    ULONG SectionInfoOffset;
    ULONG NumSections;
    ULONG FaultsPerPeriod[10];
    LARGE_INTEGER LaunchTime;
    ULONGLONG Reserved[5];
} PF_TRACE_HEADER, *PPF_TRACE_HEADER;

typedef struct _PFSN_TRACE_DUMP
{
    LIST_ENTRY CompletedTracesLink;
    PF_TRACE_HEADER Trace;
} PFSN_TRACE_DUMP, *PPFSN_TRACE_DUMP;

typedef struct _PFSN_TRACE_HEADER
{
    ULONG Magic;
    LIST_ENTRY ActiveTracesLink;
    PF_SCENARIO_ID ScenarioId;
    ULONG ScenarioType; // PF_SCENARIO_TYPE
    ULONG EventEntryIdxs[8];
    ULONG NumEventEntryIdxs;
    PPFSN_LOG_ENTRIES CurrentTraceBuffer;
    LIST_ENTRY TraceBuffersList;
    ULONG NumTraceBuffers;
    KSPIN_LOCK TraceBufferSpinLock;
    KTIMER TraceTimer;
    LARGE_INTEGER TraceTimerPeriod;
    KDPC TraceTimerDpc;
    KSPIN_LOCK TraceTimerSpinLock;
    ULONG FaultsPerPeriod[10];
    LONG LastNumFaults;
    LONG CurPeriod;
    LONG NumFaults;
    LONG MaxFaults;
    PEPROCESS Process;
    EX_RUNDOWN_REF RefCount;
    WORK_QUEUE_ITEM EndTraceWorkItem;
    LONG EndTraceCalled;
    PPFSN_TRACE_DUMP TraceDump;
    NTSTATUS TraceDumpStatus;
    LARGE_INTEGER LaunchTime;
    PPF_SECTION_INFO SectionInfo;
    ULONG SectionInfoCount;
} PFSN_TRACE_HEADER, *PPFSN_TRACE_HEADER;

typedef struct _PFSN_PREFETCHER_GLOBALS
{
    LIST_ENTRY ActiveTraces;
    KSPIN_LOCK ActiveTracesLock;
    PPFSN_TRACE_HEADER SystemWideTrace;
    LIST_ENTRY CompletedTraces;
    FAST_MUTEX CompletedTracesLock;
    LONG NumCompletedTraces;
    PKEVENT CompletedTracesEvent;
    LONG ActivePrefetches;
} PFSN_PREFETCHER_GLOBALS, *PPFSN_PREFETCHER_GLOBALS;

#if 0
typedef struct _BCB
{
    LIST_ENTRY BcbSegmentListHead;
    LIST_ENTRY BcbRemoveListEntry;
    BOOLEAN RemoveOnClose;
    ULONG TimeStamp;
    PFILE_OBJECT FileObject;
    ULONG CacheSegmentSize;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER FileSize;
    PCACHE_MANAGER_CALLBACKS Callbacks;
    PVOID LazyWriteContext;
    KSPIN_LOCK BcbLock;
    ULONG RefCount;
#if defined(DBG) || defined(KDBG)
	BOOLEAN Trace; /* enable extra trace output for this BCB and it's cache segments */
#endif
} BCB, *PBCB;
#endif

typedef struct _NOCC_BCB
{
    /* Public part */
    PUBLIC_BCB Bcb;

    /* So we know the initial request that was made */
    PFILE_OBJECT FileObject;
	PSECTION_OBJECT SectionObject;
	LARGE_INTEGER FileOffset;
	ULONG Length;
	PVOID BaseAddress;
	BOOLEAN Dirty;
	PVOID OwnerPointer;

	/* Reference counts */
	ULONG RefCount;

	LIST_ENTRY ThisFileList;

	KEVENT ExclusiveWait;
	BOOLEAN Exclusive;
	ULONG ExclusiveWaiter;
} NOCC_BCB, *PNOCC_BCB;

typedef struct _NOCC_CACHE_MAP
{
	LIST_ENTRY AssociatedBcb;
	PFILE_OBJECT FileObject;
	ULONG NumberOfMaps;
} NOCC_CACHE_MAP, *PNOCC_CACHE_MAP;

VOID
NTAPI
CcPfInitializePrefetcher(
    VOID
);

VOID
NTAPI
CcMdlReadComplete2(
    IN PMDL MemoryDescriptorList,
    IN PFILE_OBJECT FileObject
);

VOID
NTAPI
CcMdlWriteComplete2(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain
);

VOID
NTAPI
CcInitView(VOID);

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

NTSTATUS
NTAPI
CcRosFlushDirtyPages(
    ULONG Target,
    PULONG Count
);

VOID
NTAPI
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosReferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTSTATUS
NTAPI
CcTryToInitializeFileCache(PFILE_OBJECT FileObject);

/*
 * Macro for generic cache manage bugchecking. Note that this macro assumes
 * that the file name including extension is always longer than 4 characters.
 */
#define KEBUGCHECKCC \
    KEBUGCHECKEX(CACHE_MANAGER, \
    (*(ULONG*)(__FILE__ + sizeof(__FILE__) - 4) << 16) | \
    (__LINE__ & 0xFFFF), 0, 0, 0)

/* Private data */

#define CACHE_SINGLE_FILE_MAX (16)
#define CACHE_OVERALL_SIZE (32 * 1024 * 1024)
#define CACHE_STRIPE VACB_MAPPING_GRANULARITY
#define CACHE_SHIFT 18
#define CACHE_NUM_SECTIONS (CACHE_OVERALL_SIZE / CACHE_STRIPE)
#define CACHE_ROUND_UP(x) (((x) + (CACHE_STRIPE-1)) & ~(CACHE_STRIPE-1))
#define CACHE_ROUND_DOWN(x) ((x) & ~(CACHE_STRIPE-1))
#define CACHE_NEED_SECTIONS(OFFSET,LENGTH) \
	((CACHE_ROUND_UP((OFFSET)->QuadPart + (LENGTH)) -		\
	  CACHE_ROUND_DOWN((OFFSET)->QuadPart)) >> CACHE_SHIFT)
#define INVALID_CACHE ((ULONG)~0)

extern NOCC_BCB CcCacheSections[CACHE_NUM_SECTIONS];
extern PRTL_BITMAP CcCacheBitmap;
extern KGUARDED_MUTEX CcMutex;
extern KEVENT CcDeleteEvent;
extern ULONG CcCacheClockHand;
extern LIST_ENTRY CcPendingUnmap;

extern VOID CcpLock();
extern VOID CcpUnlock();
extern VOID CcpDereferenceCache(ULONG Sector);

#endif
