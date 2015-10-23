#ifndef SWELL_PROVIDED_BY_APP

//#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include "swell.h"
#include "swell-internal.h"

#include "../mutex.h"

HWND g_swell_only_timerhwnd;

@implementation SWELL_TimerFuncTarget

-(id) initWithId:(UINT_PTR)tid hwnd:(HWND)h callback:(TIMERPROC)cb
{
  if ((self = [super init]))
  {
    m_hwnd=h;
    m_cb=cb;
    m_timerid = tid;
  }
  return self;
}
-(void)SWELL_Timer:(id)sender
{
  if (g_swell_only_timerhwnd && m_hwnd != g_swell_only_timerhwnd) return;
  
  m_cb(m_hwnd,WM_TIMER,m_timerid,GetTickCount());
}
@end

@implementation SWELL_DataHold
-(id) initWithVal:(void *)val
{
  if ((self = [super init]))
  {
    m_data=val;
  }
  return self;
}
-(void *) getValue
{
  return m_data;
}
@end

void SWELL_CFStringToCString(const void *str, char *buf, int buflen)
{
  NSString *s = (NSString *)str;
  if (buflen>0) *buf=0;
  if (buflen <= 1 || !s || [s getCString:buf maxLength:buflen encoding:NSUTF8StringEncoding]) return; // should always work, I'd hope (ambiguous documentation ugh)
  
  NSData *data = [s dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:YES];
  if (!data)
  {
    [s getCString:buf maxLength:buflen encoding:NSASCIIStringEncoding];
    return;
  }
  int len = [data length];
  if (len > buflen-1) len=buflen-1;
  [data getBytes:buf length:len];
  buf[len]=0;
}

void *SWELL_CStringToCFString(const char *str)
{
  if (!str) str="";
  void *ret;
  
  ret=(void *)CFStringCreateWithCString(NULL,str,kCFStringEncodingUTF8);
  if (ret) return ret;
  ret=(void*)CFStringCreateWithCString(NULL,str,kCFStringEncodingASCII);
  return ret;
}

void SWELL_MakeProcessFront(HANDLE h)
{
  SWELL_InternalObjectHeader_NSTask *buf = (SWELL_InternalObjectHeader_NSTask*)h;
  if (buf && buf->hdr.type == INTERNAL_OBJECT_NSTASK && buf->task)
  {
    ProcessSerialNumber psn;
    
    int pid=[(id)buf->task processIdentifier];
    // try for 1sec to get the PSN, if it fails
    int n = 20;
    while (GetProcessForPID(pid,&psn) != noErr && n-- > 0)
    {
      Sleep(50);
    }
    if (n>0) SetFrontProcess(&psn);
  }
}

void SWELL_ReleaseNSTask(void *p)
{
  NSTask *a =(NSTask*)p;
  [a release];
}
DWORD SWELL_WaitForNSTask(void *p, DWORD msTO)
{
  NSTask *a =(NSTask*)p;
  DWORD t = msTO ? GetTickCount()+msTO : 0;
  do 
  {
    if (![a isRunning]) return WAIT_OBJECT_0;
    if (t) Sleep(1);
  }
  while (GetTickCount()<t);

  return [a isRunning] ? WAIT_TIMEOUT : WAIT_OBJECT_0;
}

HANDLE SWELL_CreateProcess(const char *exe, int nparams, const char **params)
{
  NSString *ex = (NSString *)SWELL_CStringToCFString(exe);
  NSMutableArray *ar = [[NSMutableArray alloc] initWithCapacity:nparams];

  int x;
  for (x=0;x <nparams;x++)
  {
    NSString *s = (NSString *)SWELL_CStringToCFString(params[x]?params[x]:"");
    [ar addObject:s];
    [s release];
  }

  NSTask *tsk = [[NSTask alloc] init];

  if (tsk)
  {
    @try {
      [tsk setArguments:ar];
      [tsk setLaunchPath:ex];
      [tsk launch];
    }
    @catch (NSException *exception) { 
      [tsk release];
      tsk=0;
    }
    @catch (id ex) {
    }
  }

  [ex release];
  [ar release];
  if (!tsk) return NULL;
  
  SWELL_InternalObjectHeader_NSTask *buf = (SWELL_InternalObjectHeader_NSTask*)malloc(sizeof(SWELL_InternalObjectHeader_NSTask));
  buf->hdr.type = INTERNAL_OBJECT_NSTASK;
  buf->hdr.count=1;
  buf->task = tsk;
  return buf;
}


@implementation SWELL_ThreadTmp
-(void)bla:(id)obj
{
  if (a) 
  {
    DWORD (*func)(void *);
    *(void **)(&func) = a;
    func(b);
  }
  [NSThread exit];
}
@end

void SWELL_EnsureMultithreadedCocoa()
{
  static int a;
  if (!a)
  {
    a++;
    if (![NSThread isMultiThreaded]) // force cocoa into multithreaded mode
    {
      SWELL_ThreadTmp *t=[[SWELL_ThreadTmp alloc] init]; 
      t->a=0;
      t->b=0;
      [NSThread detachNewThreadSelector:@selector(bla:) toTarget:t withObject:t];
      ///      [t release];
    }
  }
}

void CreateThreadNS(void *TA, DWORD stackSize, DWORD (*ThreadProc)(LPVOID), LPVOID parm, DWORD cf, DWORD *tidOut)
{
  SWELL_ThreadTmp *t=[[SWELL_ThreadTmp alloc] init]; 
  t->a=(void*)ThreadProc;
  t->b=parm;
  return [NSThread detachNewThreadSelector:@selector(bla:) toTarget:t withObject:t];
}


// used by swell.cpp (lazy these should go elsewhere)
void *SWELL_InitAutoRelease()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  return (void *)pool;
}
void SWELL_QuitAutoRelease(void *p)
{
  if (p)
    [(NSAutoreleasePool*)p release];
}

void SWELL_RunEvents()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  int x=100;
  while (x-- > 0)
  {
    NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate dateWithTimeIntervalSinceNow:0.001] inMode:NSDefaultRunLoopMode dequeue:YES];
    if (!event) break;
    [NSApp sendEvent:event];
  }
  [pool release];
}

// timer stuff
typedef struct TimerInfoRec
{
  UINT_PTR timerid;
  HWND hwnd;
  NSTimer *timer;
  struct TimerInfoRec *_next;
} TimerInfoRec;
static TimerInfoRec *m_timer_list;
static WDL_Mutex m_timermutex;
static pthread_t m_pmq_mainthread;
static void SWELL_pmq_settimer(HWND h, UINT_PTR timerid, UINT rate, TIMERPROC tProc);

UINT_PTR SetTimer(HWND hwnd, UINT_PTR timerid, UINT rate, TIMERPROC tProc)
{
  if (!hwnd && !tProc) return 0; // must have either callback or hwnd
  
  if (hwnd && !timerid) return 0;
  
  if (timerid != ~(UINT_PTR)0 && m_pmq_mainthread && pthread_self()!=m_pmq_mainthread)
  {   
    SWELL_pmq_settimer(hwnd,timerid,(rate==(UINT)-1)?((UINT)-2):rate,tProc);
    return timerid;
  }
  
  
  if (hwnd && ![(id)hwnd respondsToSelector:@selector(SWELL_Timer:)])
  {
    if (![(id)hwnd isKindOfClass:[NSWindow class]]) return 0;
    hwnd=(HWND)[(id)hwnd contentView];
    if (![(id)hwnd respondsToSelector:@selector(SWELL_Timer:)]) return 0;
  }
  
  WDL_MutexLock lock(&m_timermutex);
  TimerInfoRec *rec=NULL;
  if (hwnd||timerid)
  {
    rec = m_timer_list;
    while (rec)
    {
      if (rec->timerid == timerid && rec->hwnd == hwnd) // works for both kinds
        break;
      rec=rec->_next;
    }
  }
  
  bool recAdd=false;
  if (!rec) 
  {
    rec=(TimerInfoRec*)malloc(sizeof(TimerInfoRec));
    recAdd=true;
  }
  else 
  {
    [rec->timer invalidate];
    rec->timer=0;
  }
  
  rec->timerid=timerid;
  rec->hwnd=hwnd;
  
  if (!hwnd || tProc)
  {
    // set timer to this unique ptr
    if (!hwnd) timerid = rec->timerid = (UINT_PTR)rec;
    
    SWELL_TimerFuncTarget *t = [[SWELL_TimerFuncTarget alloc] initWithId:timerid hwnd:hwnd callback:tProc];
    rec->timer = [NSTimer scheduledTimerWithTimeInterval:(wdl_max(rate,1)*0.001) target:t selector:@selector(SWELL_Timer:)
                                                userInfo:t repeats:YES];
    [t release];
    
  }
  else
  {
    SWELL_DataHold *t=[[SWELL_DataHold alloc] initWithVal:(void *)timerid];
    rec->timer = [NSTimer scheduledTimerWithTimeInterval:(wdl_max(rate,1)*0.001) target:(id)hwnd selector:@selector(SWELL_Timer:)
                                                userInfo:t repeats:YES];
    
    [t release];
  }
  [[NSRunLoop currentRunLoop] addTimer:rec->timer forMode:(NSString*)kCFRunLoopCommonModes];
  
  if (recAdd)
  {
    rec->_next=m_timer_list;
    m_timer_list=rec;
  }
  
  return timerid;
}
void SWELL_RunRunLoop(int ms)
{
  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:ms*0.001]];
}
void SWELL_RunRunLoopEx(int ms, HWND hwndOnlyTimer)
{
  HWND h=g_swell_only_timerhwnd;
  g_swell_only_timerhwnd = hwndOnlyTimer;
  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:ms*0.001]];
  if (g_swell_only_timerhwnd == hwndOnlyTimer) g_swell_only_timerhwnd = h;
}

BOOL KillTimer(HWND hwnd, UINT_PTR timerid)
{
  if (!hwnd && !timerid) return FALSE;
  
  WDL_MutexLock lock(&m_timermutex);
  if (timerid != ~(UINT_PTR)0 && m_pmq_mainthread && pthread_self()!=m_pmq_mainthread)
  {
    SWELL_pmq_settimer(hwnd,timerid,~(UINT)0,NULL);
    return TRUE;
  }
  BOOL rv=FALSE;
  
  // don't allow removing all global timers
  if (timerid!=~(UINT_PTR)0 || hwnd) 
  {
    TimerInfoRec *rec = m_timer_list, *lrec=NULL;
    while (rec)
    {
      
      if (rec->hwnd == hwnd && (timerid==~(UINT_PTR)0 || rec->timerid == timerid))
      {
        TimerInfoRec *nrec = rec->_next;
        
        // remove self from list
        if (lrec) lrec->_next = nrec;
        else m_timer_list = nrec;
        
        [rec->timer invalidate];
        free(rec);
        
        rv=TRUE;
        if (timerid!=~(UINT_PTR)0) break;
        
        rec=nrec;
      }
      else 
      {
        lrec=rec;
        rec=rec->_next;
      }
    }
  }
  return rv;
}



///////// PostMessage emulation

BOOL PostMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  id del=[NSApp delegate];
  if (del && [del respondsToSelector:@selector(swellPostMessage:msg:wp:lp:)])
    return !![(SWELL_DelegateExtensions*)del swellPostMessage:hwnd msg:message wp:wParam lp:lParam];
  return FALSE;
}

void SWELL_MessageQueue_Clear(HWND h)
{
  id del=[NSApp delegate];
  if (del && [del respondsToSelector:@selector(swellPostMessageClearQ:)])
    [(SWELL_DelegateExtensions*)del swellPostMessageClearQ:h];
}



// implementation of postmessage stuff



typedef struct PMQ_rec
{
  HWND hwnd;
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;
  
  struct PMQ_rec *next;
  bool is_special_timer; // if set, then msg=interval(-1 for kill),wParam=timer id, lParam = timerproc
} PMQ_rec;

static WDL_Mutex *m_pmq_mutex;
static PMQ_rec *m_pmq, *m_pmq_empty, *m_pmq_tail;
static int m_pmq_size;
static id m_pmq_timer;
#define MAX_POSTMESSAGE_SIZE 1024

void SWELL_Internal_PostMessage_Init()
{
  if (m_pmq_mutex) return;
  id del = [NSApp delegate];
  if (!del || ![del respondsToSelector:@selector(swellPostMessageTick:)]) return;
  
  m_pmq_mainthread=pthread_self();
  m_pmq_mutex = new WDL_Mutex;
  
  m_pmq_timer = [NSTimer scheduledTimerWithTimeInterval:0.05 target:(id)del selector:@selector(swellPostMessageTick:) userInfo:nil repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer:m_pmq_timer forMode:(NSString*)kCFRunLoopCommonModes];
  //  [ release];
  // set a timer to the delegate
}

#ifndef SWELL_JUST_FOR_THREADING
void SWELL_MessageQueue_Flush()
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq, *startofchain=m_pmq;
  m_pmq=m_pmq_tail=0;
  m_pmq_mutex->Leave();
  
  int cnt=0;
  // process out queue
  while (p)
  {
    if (p->is_special_timer)
    {
      if (p->msg == ~(UINT)0) KillTimer(p->hwnd,p->wParam);
      else SetTimer(p->hwnd,p->wParam,p->msg,(TIMERPROC)p->lParam);
    }
    else
      SendMessage(p->hwnd,p->msg,p->wParam,p->lParam); 
    
    cnt ++;
    if (!p->next) // add the chain back to empties
    {
      m_pmq_mutex->Enter();
      m_pmq_size-=cnt;
      p->next=m_pmq_empty;
      m_pmq_empty=startofchain;
      m_pmq_mutex->Leave();
      break;
    }
    p=p->next;
  }
}
#endif

void SWELL_Internal_PMQ_ClearAllMessages(HWND hwnd)
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq;
  PMQ_rec *lastrec=NULL;
  while (p)
  {
    if (hwnd && p->hwnd != hwnd) { lastrec=p; p=p->next; }
    else
    {
      PMQ_rec *next=p->next; 
      
      p->next=m_pmq_empty; // add p to empty list
      m_pmq_empty=p;
      m_pmq_size--;
      
      
      if (p==m_pmq_tail) m_pmq_tail=lastrec; // update tail
      
      if (lastrec)  p = lastrec->next = next;
      else p = m_pmq = next;
    }
  }
  m_pmq_mutex->Leave();
}

static void SWELL_pmq_settimer(HWND h, UINT_PTR timerid, UINT rate, TIMERPROC tProc)
{
  if (!h||!m_pmq_mutex) return;
  WDL_MutexLock lock(m_pmq_mutex);
  
  PMQ_rec *rec=m_pmq;
  while (rec)
  {
    if (rec->is_special_timer && rec->hwnd == h && rec->wParam == timerid)
    {
      rec->msg = rate; // adjust to new rate
      rec->lParam = (LPARAM)tProc;
      return;
    }
    rec=rec->next;
  }  
  
  rec=m_pmq_empty;
  if (rec) m_pmq_empty=rec->next;
  else rec=(PMQ_rec*)malloc(sizeof(PMQ_rec));
  rec->next=0;
  rec->hwnd=h;
  rec->msg=rate;
  rec->wParam=timerid;
  rec->lParam=(LPARAM)tProc;
  rec->is_special_timer=true;
  
  if (m_pmq_tail) m_pmq_tail->next=rec;
  else 
  {
    PMQ_rec *p=m_pmq;
    while (p && p->next) p=p->next; // shouldnt happen unless m_pmq is NULL As well but why not for safety
    if (p) p->next=rec;
    else m_pmq=rec;
  }
  m_pmq_tail=rec;
  m_pmq_size++;
}

BOOL SWELL_Internal_PostMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd||!m_pmq_mutex) return FALSE;
  if (![(id)hwnd respondsToSelector:@selector(swellCanPostMessage)]) return FALSE;
  
  BOOL ret=FALSE;
  m_pmq_mutex->Enter();
  
  if ((m_pmq_empty||m_pmq_size<MAX_POSTMESSAGE_SIZE) && [(id)hwnd swellCanPostMessage])
  {
    PMQ_rec *rec=m_pmq_empty;
    if (rec) m_pmq_empty=rec->next;
    else rec=(PMQ_rec*)malloc(sizeof(PMQ_rec));
    rec->next=0;
    rec->hwnd=hwnd;
    rec->msg=msg;
    rec->wParam=wParam;
    rec->lParam=lParam;
    rec->is_special_timer=false;
    
    if (m_pmq_tail) m_pmq_tail->next=rec;
    else 
    {
      PMQ_rec *p=m_pmq;
      while (p && p->next) p=p->next; // shouldnt happen unless m_pmq is NULL As well but why not for safety
      if (p) p->next=rec;
      else m_pmq=rec;
    }
    m_pmq_tail=rec;
    m_pmq_size++;
    
    ret=TRUE;
  }
  
  m_pmq_mutex->Leave();
  
  return ret;
}


static bool s_rightclickemulate=true;

bool IsRightClickEmulateEnabled()
{
  return s_rightclickemulate;
}

void SWELL_EnableRightClickEmulate(BOOL enable)
{
  s_rightclickemulate=enable;
}

int g_swell_terminating;
void SWELL_PostQuitMessage(void *sender)
{
  g_swell_terminating=true;

  [NSApp terminate:(id)sender];
}


#ifndef MAC_OS_X_VERSION_10_9
typedef uint64_t NSActivityOptions;
enum
{
  NSActivityIdleDisplaySleepDisabled = (1ULL << 40),
  NSActivityIdleSystemSleepDisabled = (1ULL << 20),
  NSActivitySuddenTerminationDisabled = (1ULL << 14),
  NSActivityAutomaticTerminationDisabled = (1ULL << 15),
  NSActivityUserInitiated = (0x00FFFFFFULL | NSActivityIdleSystemSleepDisabled),
  NSActivityUserInitiatedAllowingIdleSystemSleep = (NSActivityUserInitiated & ~NSActivityIdleSystemSleepDisabled),
  NSActivityBackground = 0x000000FFULL,
  NSActivityLatencyCritical = 0xFF00000000ULL,
};


@interface NSProcessInfo (reaperhostadditions)
- (id<NSObject>)beginActivityWithOptions:(NSActivityOptions)options reason:(NSString *)reason;
- (void)endActivity:(id<NSObject>)activity;

@end

#endif

void SWELL_DisableAppNap(int disable)
{
  if (!g_swell_terminating && floor(NSFoundationVersionNumber) > 945.00) // 10.9+
  {
    static int cnt;
    static id obj;

    cnt += disable;

    @try
    {
      if (cnt > 0)
      {
        if (!obj)
        {
          const NSActivityOptions  v = NSActivityLatencyCritical |  NSActivityIdleSystemSleepDisabled;

          // beginActivityWithOptions returns an autoreleased object
          obj = [[NSProcessInfo processInfo] beginActivityWithOptions:v reason:@"SWELL_DisableAppNap"];
          if (obj) [obj retain];
        }
      }
      else
      {
        id a = obj;
        if (a)
        {
          // in case we crash somehow, dont' want obj sticking around pointing to a stale object
          obj = NULL;
          [[NSProcessInfo processInfo] endActivity:a];
          [a release]; // apparently releasing this is enough, without the endActivity call, but the docs are quite vague
        }
      }
    }
    @catch (NSException *exception) {
    }
    @catch (id ex) {
    }
  }
}


int SWELL_GetOSXVersion()
{
  static SInt32 v;
  if (!v)
  {
    if (NSAppKitVersionNumber >= 1266.0) 
    {
      v=0x10a0; // 10.10+ Gestalt(gsv) return 0x109x, so we bump this to 0x10a0
    }
    else 
    {
      SInt32 a = 0x1040;
      Gestalt(gestaltSystemVersion,&a);
      v=a;
    }
  }
  return v;
}

#endif
