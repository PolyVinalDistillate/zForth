#ifndef zforth_h
#define zforth_h

#include "zfconf.h"

//Sample core definitions for higher-level functionality (added by Nick(PolyVinalDistillate)) - contained in .c file
extern const char ZF_CORE_STR[];

/* Abort reasons */   
typedef enum {
	ZF_OK,
	ZF_ABORT_INTERNAL_ERROR,
	ZF_ABORT_OUTSIDE_MEM,
	ZF_ABORT_DSTACK_UNDERRUN,
	ZF_ABORT_DSTACK_OVERRUN,
	ZF_ABORT_RSTACK_UNDERRUN,
	ZF_ABORT_RSTACK_OVERRUN,
	ZF_ABORT_NOT_A_WORD,
	ZF_ABORT_COMPILE_ONLY_WORD,
	ZF_ABORT_INVALID_SIZE,
    ZF_NICKS_RERUN_FLAG			//Extra flag used to indicate when inner interpreter needs re-invoked
} zf_result;

typedef enum {
	ZF_MEM_SIZE_VAR,
	ZF_MEM_SIZE_CELL,
	ZF_MEM_SIZE_U8,
	ZF_MEM_SIZE_U16,
	ZF_MEM_SIZE_U32,
	ZF_MEM_SIZE_S8,
	ZF_MEM_SIZE_S16,
	ZF_MEM_SIZE_S32
} zf_mem_size;

typedef enum {
	ZF_INPUT_INTERPRET,
	ZF_INPUT_PASS_CHAR,
	ZF_INPUT_PASS_WORD
} zf_input_state;

typedef enum {
	ZF_SYSCALL_EMIT,
	ZF_SYSCALL_PRINT,
	ZF_SYSCALL_TELL,
	ZF_SYSCALL_USER = 128
} zf_syscall_id;


/* ZForth API functions */

//Main update function added by Nick(PolyVinalDistillate). Wraps zf_ReRun and zf_eval for implementing a one-word-at-a-time
//execution system. Send data in through pBuf and specify length in nLen. nLen will be modified to indicate
//the number of bytes actually read from pBuf such that the next entry can supply pBuf+nLen as the buffer
//pointer.
zf_result zf_Main_Update_Fxn(unsigned char* pBuf, unsigned short* nLen);


//Modified by Nick(PolyVinalDistillate). If bEnableThrottle = 0, then zf_eval will not exit until entire input processed. NOTE: This includes 
//forth loops! If bEnableThrottle = 1, then function will exit once first word is processed. subsequently, zf_ReRun() should be called repeatedly 
//until it returns ZF_OK, indicating end of processing of current words. This is handled in the "zf_Main_Update_Fxn()" if run as shown in 
//the PSoC folder's main.c, however zf_eval() can still be used with bEnableThrottle = 0 and will then behave as it did before.
zf_result zf_eval(const char *buf, int bEnableThrottle);

void zf_init(int trace);
void zf_bootstrap(void);
void *zf_dump(size_t *len);
void zf_abort(zf_result reason);

void zf_push(zf_cell v);
zf_cell zf_pop(void);
zf_cell zf_pick(zf_addr n);

/* Host provides these functions */

zf_input_state zf_host_sys(zf_syscall_id id, const char *last_word);
//void zf_host_trace(const char *fmt, va_list va);
zf_cell zf_host_parse_num(const char *buf);

#endif
