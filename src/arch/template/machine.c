/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

/** @file
 * Templated machine layer
 * @ingroup Machine
 *
 * This file explains what the machine layer has to provide (which functions
 * need to be implemented). Depending on the flags set in the files
 * conv-common.h, conv-mach.h and the possible other suboption file
 * conv-mach-suboption.h, some additional functions may be needed to be
 * implemented.

 * Throughout the file, "#if CMK_VARIABLE" means it was set to 1 in the .h
 * files, "#if ! CMK_VARIABLE" means it was set to 0.

/*@{*/

/** FUNCTIONS ALWAYS TO BE IMPLEMENTED

 * This first section of the file reports which methods must always be
 * implemented inside the machine layer.
 */ 

void ConverseInit(int, char**, CmiStartFn, int, int);
void ConverseExit(void);

void CmiAbort(const char *);

void          CmiSyncSendFn(int, int, char *);
CmiCommHandle CmiAsyncSendFn(int, int, char *);
void          CmiFreeSendFn(int, int, char *);

void          CmiSyncBroadcastFn(int, char *);
CmiCommHandle CmiAsyncBroadcastFn(int, char *);
void          CmiFreeBroadcastFn(int, char *);

void          CmiSyncBroadcastAllFn(int, char *);
CmiCommHandle CmiAsyncBroadcastAllFn(int, char *);
void          CmiFreeBroadcastAllFn(int, char *);

int           CmiAsyncMsgSent(CmiCommHandle handle);
void          CmiReleaseCommHandle(CmiCommHandle handle);

void	      CmiMultipleSend(unsigned int, int, int *, char **);
void	      CmiMultipleIsend(unsigned int, int, int *, char **);


/** MULTICAST/VECTOR SENDING FUNCTIONS

 * In relations to some flags, some other delivery functions may be needed.
 */

#if ! CMK_MULTICAST_LIST_USE_COMMON_CODE
void          CmiSyncListSendFn(int, int *, int, char*);
CmiCommHandle CmiAsyncListSendFn(int, int *, int, char*);
void          CmiFreeListSendFn(int, int *, int, char*);
#endif

#if ! CMK_MULTICAST_GROUP_USE_COMMON_CODE
void          CmiSyncMulticastFn(CmiGroup, int, char*);
CmiCommHandle CmiAsyncMulticastFn(CmiGroup, int, char*);
void          CmiFreeMulticastFn(CmiGroup, int, char*);
#endif

#if ! CMK_VECTOR_SEND_USES_COMMON_CODE
void          CmiSyncVectorSend(int, int, int *, char **);
CmiCommHandle CmiAsyncVectorSend(int, int, int *, char **);
void          CmiSyncVectorSendAndFree(int, int, int *, char **);
#endif


/** NODE SENDING FUNCTIONS

 * If there is a node queue, and we consider also nodes as entity (tipically in
 * SMP versions), these functions are needed.
 */

#if CMK_NODE_QUEUE_AVAILABLE

void          CmiSyncNodeSendFn(int, int, char *);
CmiCommHandle CmiAsyncNodeSendFn(int, int, char *);
void          CmiFreeNodeSendFn(int, int, char *);

void          CmiSyncNodeBroadcastFn(int, char *);
CmiCommHandle CmiAsyncNodeBroadcastFn(int, char *);
void          CmiFreeNodeBroadcastFn(int, char *);

void          CmiSyncNodeBroadcastAllFn(int, char *);
CmiCommHandle CmiAsyncNodeBroadcastAllFn(int, char *);
void          CmiFreeNodeBroadcastAllFn(int, char *);

#endif


/** GROUPS DEFINITION

 * For groups of processors (establishing and managing) some more functions are
 * needed, they also con be found in common code (convcore.c) or here.
 */

#if ! CMK_MULTICAST_DEF_USE_COMMON_CODE
void     CmiGroupInit();
CmiGroup CmiEstablishGroup(int npes, int *pes);
void     CmiLookupGroup(CmiGroup grp, int *npes, int **pes);
#endif


/** MESSAGE DELIVERY FUNCTIONS

 * In order to deliver the messages to objects (either converse register
 * handlers, or charm objects), a scheduler is needed. The one implemented in
 * convcore.c can be used, or a new one can be implemented here. At present, all
 * machines use the default one, exept sim-linux.

 * If the one in convcore.c is used, still one function is needed.
 */

#if CMK_CMIDELIVERS_USE_COMMON_CODE /* use the default one */

CpvDeclare(void*, CmiLocalQueue);
void *CmiGetNonLocal();

#elif /* reimplement the scheduler and delivery */

void CsdSchedulerState_new(CsdSchedulerState_t *state);
void *CsdNextMessage(CsdSchedulerState_t *state);
int  CsdScheduler(int maxmsgs);

void CmiDeliversInit();
int  CmiDeliverMsgs(int maxmsgs);
void CmiDeliverSpecificMsg(int handler);

#endif


/** SHARED VARIABLES DEFINITIONS

 * In relation to which CMK_SHARED_VARS_ flag is set, different
 * functions/variables need to be defined and initialized correctly.
 */

#if CMK_SHARED_VARS_UNAVAILABLE /* Non-SMP version of shared vars. */

int _Cmi_mype;
int _Cmi_numpes;
int _Cmi_myrank; /* Normally zero; only 1 during SIGIO handling */

void CmiMemLock();
void CmiMemUnlock();

#endif

#if CMK_SHARED_VARS_POSIX_THREADS_SMP /*Used by the net-*-smp versions*/

int _Cmi_numpes;
int _Cmi_mynodesize;
int _Cmi_mynode;
int _Cmi_numnodes;

int CmiMyPe();
int CmiMyRank();
int CmiNodeFirst(int node);
int CmiNodeSize(int node);
int CmiNodeOf(int pe);
int CmiRankOf(int pe);

/* optional, these functions are implemented in "machine-smp.c", so including
   this file avoid the necessity to reimplement them.
 */
void CmiNodeBarrier(void);
void CmiNodeAllBarrier(void);
CmiNodeLock CmiCreateLock();
void CmiDestroyLock(CmiNodeLock lock);

#endif

/* NOT VERY USEFUL */
#if CMK_SHARED_VARS_EXEMPLAR /* Used only by HP Exemplar version */

int _Cmi_numpes;
int _Cmi_mynodesize;

void CmiMemLock();
void CmiMemUnlock();
void *CmiSvAlloc(int);

/* optional, these functions are implemented in "machine-smp.c", so including
   this file avoid the necessity to reimplement them.
 */
void CmiNodeBarrier(void);
CmiNodeLock CmiCreateLock(void);

#endif

/* NOT VERY USEFUL */
#if CMK_SHARED_VARS_UNIPROCESSOR /*Used only by uth- and sim- versions*/

int _Cmi_mype;
int _Cmi_numpes;

void         CmiLock(CmiNodeLock lock);
void         CmiUnlock(CmiNodeLock lock);
int          CmiTryLock(CmiNodeLock lock);

/* optional, these functions are implemented in "machine-smp.c", so including
   this file avoid the necessity to reimplement them.
 */
void CmiNodeBarrier();
void CmiNodeAllBarrier();
CmiNodeLock  CmiCreateLock(void);
void         CmiDestroyLock(CmiNodeLock lock);

#endif

/* NOT VERY USEFUL */
#if CMK_SHARED_VARS_PTHREADS /*Used only by origin-pthreads*/

int CmiMyPe();
int _Cmi_numpes;

void CmiMemLock();
void CmiMemUnlock();

void         CmiLock(CmiNodeLock lock);
void         CmiUnlock(CmiNodeLock lock);
int          CmiTryLock(CmiNodeLock lock);

/* optional, these functions are implemented in "machine-smp.c", so including
   this file avoid the necessity to reimplement them.
 */
void CmiNodeBarrier();
void CmiNodeAllBarrier();
CmiNodeLock  CmiCreateLock(void);
void         CmiDestroyLock(CmiNodeLock lock);

#endif

/* NOT VERY USEFUL */
#if CMK_SHARED_VARS_NT_THREADS /*Used only by win32 versions*/

int _Cmi_numpes;
int _Cmi_mynodesize;
int _Cmi_mynode;
int _Cmi_numnodes;

int CmiMyPe();
int CmiMyRank();
int CmiNodeFirst(int node);
int CmiNodeSize(int node);
int CmiNodeOf(int pe);
int CmiRankOf(int pe);

/* optional, these functions are implemented in "machine-smp.c", so including
   this file avoid the necessity to reimplement them.
 */
void CmiNodeBarrier(void);
void CmiNodeAllBarrier(void);
CmiNodeLock CmiCreateLock(void);
void CmiDestroyLock(CmiNodeLock lock);

#endif


/** TIMERS DEFINITIONS

 * In relation to what CMK_TIMER_USE_ is selected, some * functions may need to
 * be implemented.
 */

/* If all the CMK_TIMER_USE_ are set to 0, the following timer functions are
   needed. */

void   CmiTimerInit();
double CmiTimer();
double CmiWallTimer();
double CmiCpuTimer();
int    CmiTimerIsSynchronized();

/* If one of the following is set to 1, barriers are needed:
   CMK_TIMER_USE_GETRUSAGE
   CMK_TIMER_USE_RDTSC
   CMK_TIMER_USE_BLUEGENEL
*/

void CmiBarrier();
void CmiBarrierZero();


/** PRINTF FUNCTIONS

 * Default code is provided in convcore.c but for particular architectures they
 * can be reimplemented. At present only net- versions reimplement them.

 */

#if CMK_CMIPRINTF_IS_A_BUILTIN

void CmiPrintf(const char *, ...);
void CmiError(const char *, ...);
int  CmiScanf(const char *, ...);

#endif


/** SPANNING TREE

 * During some working operations (such as quiescence detection), spanning trees
 * are used. Default code in convcore.c can be used, or a new definition can be
 * implemented here.
 */

#if ! CMK_SPANTREE_USE_COMMON_CODE

int      CmiNumSpanTreeChildren(int) ;
int      CmiSpanTreeParent(int) ;
void     CmiSpanTreeChildren(int node, int *children);

int      CmiNumNodeSpanTreeChildren(int);
int      CmiNodeSpanTreeParent(int) ;
void     CmiNodeSpanTreeChildren(int node, int *children) ;

#endif


/** CCS

 * If CCS is available the following function is needed, used in debug-conv.c
 */

#if CMK_CCS_AVAILABLE
void CmiNotifyIdle();
#endif


/** IMMEDIATE MESSAGES

 * If immediate messages are supported, the following function is needed. There
 * is an exeption if the machine progress is also defined (see later for this).

 * Moreover, the file "immediate.c" should be included, otherwise all its
 * functions and variables have to be redefined.
*/

#if CMK_CCS_AVAILABLE

#include "immediate.c"

#if ! CMK_MACHINE_PROGRESS_DEFINED /* Hack for some machines */
void CmiProbeImmediateMsg();
#endif

#endif


/** MACHINE PROGRESS DEFINED

 * Some machines (like BlueGene/L) do not have coprocessors, and messages need
 * to be pulled out of the network manually. For this reason the following
 * functions are needed. Notice that the function "CmiProbeImmediateMsg" must
 * not be defined anymore.
 */

#if CMK_MACHINE_PROGRESS_DEFINED

CpvDeclare(int, networkProgressCount);
int  networkProgressPeriod;

void CmiMachineProgressImpl();

#endif
