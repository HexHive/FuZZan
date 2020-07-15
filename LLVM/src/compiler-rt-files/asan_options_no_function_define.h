// Mode 1: naitve (asan)
// Mode 2: fuzzan asan (with logging + init)
// Mode 3: fuzzan full-rbtree
// Mode 4: fuzzan full-min (1G)
// Mode 5: fuzzan full-min (4G)
// Mode 6: fuzzan full-min (8G)
// Mode 7: fuzzan full-min (16G)
// Mode 8: fuzzan sampling mode
// Mode 9: fuzzan native msan + disable storage lock/unlock
// Mode 10: fuzzan full-min msan (16G) + disable log

#if defined (FMODE1)

#elif defined(FMODE2)
#define DISABLELOG

#elif defined(FMODE3)
#define ENABLEHEXASAN
#define ENABLERBTREE
#define DISABLELOG

#elif defined(FMODE4)
#define ENABLEHEXASAN
#define ENABLEMINSHADOW
#define DISABLELOG
#define MIN_1G

#elif defined(FMODE5)
#define ENABLEHEXASAN
#define ENABLEMINSHADOW
#define DISABLELOG
#define MIN_4G

#elif defined(FMODE6)
#define ENABLEHEXASAN
#define ENABLEMINSHADOW
#define DISABLELOG
#define MIN_8G

#elif defined(FMODE7)
#define ENABLEHEXASAN
#define ENABLEMINSHADOW
#define DISABLELOG
#define MIN_16G

#elif defined(FMODE8)
#define ENABLEHEXASAN
#define ENABLEMINSHADOW
#define MIN_1G
#define DISABLELOG
#define ENABLESAMPLEING
#define Dynamic_metadata_MODE1

#elif defined(FMODE9)
#define ENABLEFUZZANMSAN

#elif defined(FMODE10)
#define ENABLEFUZZANMSAN
#define DISABLEMLOG
#define MMIN_16G
#endif

//Variables
#define MAXSAVEENTRY 5
#define MAXCACHESIZE 65535
#define BT_BUF_SIZE 1000
#define QUARANTINEQSIZE 10000
#define SAMPLESIZE 10000
#define MINSHADOWSIZE 0x20000000

#ifndef NDEBUG
#define NDEBUG  // to disable assert();
#endif

//Options (Report Bug)
//#define PRINT_CHECK_RESULT
//#define FILE_CHECK_RESULT
#define TERMINATE_PROGRAM
#define DO_REPORT_BADCAST_FATAL_NOCOREDUMP
#ifdef DO_REPORT_BADCAST_FATAL_NOCOREDUMP
#define TERMINATE _exit(-1);
#else
#define TERMINATE Abort();
#endif
