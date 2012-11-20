/* qmclilib.h
 * QM client C API library functions and tokens.
 * Copyright (c) 2004 Ladybridge Systems, All Rights Reserved
 */

/* Function definitions */

#ifdef __BORLANDC__
   /* Borland C compiler for Windows users */
   #define DLLEntry __declspec(dllimport)
#endif

#ifdef _MSC_VER
   /* Microsoft C compiler for Windows users */
   #define DLLEntry __declspec(dllimport)

   #define QMCall _QMCall
   #define QMChange _QMChange
   #define QMClearSelect _QMClearSelect
   #define QMClose _QMClose
   #define QMConnect _QMConnect
   #define QMConnected _QMConnected
   #define QMConnectLocal _QMConnectLocal
   #define QMDisconnect _QMDisconnect
   #define QMDisconnectAll _QMDisconnectAll
   #define QMDcount _QMDcount
   #define QMDel _QMDel
   #define QMDelete _QMDelete
   #define QMDeleteu _QMDeleteu
   #define QMEndCommand _QMEndCommand
   #define QMError _QMError
   #define QMExecute _QMExecute
   #define QMExtract _QMExtract
   #define QMField _QMField
   #define QMFree _QMFree
   #define QMGetSession _QMGetSession
   #define QMIns _QMIns
   #define QMLocate _QMLocate
   #define QMMarkMapping _QMMarkMapping
   #define QMMatch _QMMatch
   #define QMMatchfield _QMMatchfield
   #define QMOpen _QMOpen
   #define QMRead _QMRead
   #define QMReadl _QMReadl
   #define QMReadList _QMReadList
   #define QMReadNext _QMReadNext
   #define QMReadu _QMReadu
   #define QMRelease _QMRelease
   #define QMReplace _QMReplace
   #define QMRespond _QMRespond
   #define QMSelect _QMSelect
   #define QMSelectIndex _QMSelectIndex
   #define QMSelectLeft _QMSelectLeft
   #define QMSelectRight _QMSelectRight
   #define QMSetLeft _QMSetLeft
   #define QMSetRight _QMSetRight
   #define QMSetSession _QMSetSession
   #define QMStatus _QMStatus
   #define QMWrite _QMWrite
   #define QMWriteu _QMWriteu
#endif

#ifndef DLLEntry
   /* Other platforms */
   #define DLLEntry
#endif

DLLEntry void QMCall(char * subrname, short int argc, ...);
DLLEntry char * QMChange(char * src, char * old_string, char * new_string, int occurrences, int start);
DLLEntry void QMClearSelect(int listno);
DLLEntry void QMClose(int fno);
DLLEntry int QMConnect(char * host, int port, char * username, char * password, char * account);
DLLEntry int QMConnected(void);
DLLEntry int QMConnectLocal(char * account);
DLLEntry int QMDcount(char * src, char * delim_str);
DLLEntry char * QMDel(char * src, int fno, int vno, int svno);
DLLEntry void QMDelete(int fno, char * id);
DLLEntry void QMDeleteu(int fno, char * id);
DLLEntry void QMDisconnect(void);
DLLEntry void QMDisconnectAll(void);
DLLEntry void QMEndCommand(void);
DLLEntry char * QMError(void);
DLLEntry char * QMExecute(char * cmnd, int * err);
DLLEntry char * QMExtract(char * src, int fno, int vno, int svno);
DLLEntry char * QMField(char * src, char * delim, int first, int occurrences);
DLLEntry void QMFree(void * p);
DLLEntry int QMGetSession(void);
DLLEntry char * QMIns(char * src, int fno, int vno, int svno, char * new_string);
DLLEntry int QMLocate(char * item, char * src, int fno, int vno, int svno, int * pos, char * order);
DLLEntry void QMMarkMapping(int fno, int state);
DLLEntry int QMMatch(char * str, char * pattern);
DLLEntry char * QMMatchfield(char * str, char * pattern, int component);
DLLEntry int QMOpen(char * filename);
DLLEntry char * QMRead(int fno, char * id, int * err);
DLLEntry char * QMReadl(int fno, char * id, int wait, int * err);
DLLEntry char * QMReadList(int listno);
DLLEntry char * QMReadNext(short int listno);
DLLEntry char * QMReadu(int fno, char * id, int wait, int * err);
DLLEntry void QMRelease(int fno, char * id);
DLLEntry char * QMReplace(char * src, int fno, int vno, int svno, char * new_string);
DLLEntry char * QMRespond(char * response, int * err);
DLLEntry void QMSelect(int fno, int listno);
DLLEntry void QMSelectIndex(int fno, char * indexname, char * indexvalue, int listno);
DLLEntry char * QMSelectLeft(int fno, char * indexname, int listno);
DLLEntry char * QMSelectRight(int fno, char * indexname, int listno);
DLLEntry void QMSetLeft(int fno, char * indexname);
DLLEntry void QMSetRight(int fno, char * indexname);
DLLEntry int QMSetSession(int session);
DLLEntry int QMStatus(void);
DLLEntry void QMWrite(int fno, char * id, char * data);
DLLEntry void QMWriteu(int fno, char * id, char * data);


/* Server error status values */

#define SV_OK             0    /* Action successful                       */
#define SV_ON_ERROR       1    /* Action took ON ERROR clause             */
#define SV_ELSE           2    /* Action took ELSE clause                 */
#define SV_ERROR          3    /* Action failed. Error text available     */
#define SV_LOCKED         4    /* Action took LOCKED clause               */
#define SV_PROMPT         5    /* Server requesting input                 */

/* END-CODE */
