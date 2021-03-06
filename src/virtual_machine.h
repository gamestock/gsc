#pragma once
#include "stdheader.h"

#include "variable.h"
#include "include/gsc.h"
#include "cvector.h"
#include "dynarray.h"
#include "dynstack.h"

#define VM_STACK_SIZE (65000)
//#define VM_STACK_SIZE (1024 * 1024 * 5)

typedef enum {
	REG_A,
	REG_B,
	REG_C,
	REG_D,
	REG_E,
	REG_F,
	REG_IP,
	REG_SP,
	REG_BP,
	REG_COND,
	e_vm_reg_len
} e_vm_registers;

static const char *e_vm_reg_names[] = {
	"A","B","C","D","E","F","IP","SP","BP",0
};

typedef struct {
	char name[256];
	int position;
	int numargs;
	int flags;
	unsigned int program;
} vm_function_info_t;

typedef struct
{
	unsigned int string;
	unsigned int stackoffset;
	int type;
	varval_t *object;
	bool inuse;
	int numargs;
} vm_event_string_t;

#define VM_MAX_EVENTS (32)
struct vm_thread_s {
	//char ffi_libname[256];
	size_t stacksize;
	intptr_t *stack, registers[e_vm_reg_len];
	int numargs;
	int wait;
	//int flags; //unused for now
	bool active;
	varval_t *self;
	//char string[512]; //used for getting string value types n stuff
	vector strings;
	vm_event_string_t eventstrings[VM_MAX_EVENTS]; //non ptr allocate once, don't wanna alloc/free too much mhm
	unsigned int eventstring;
	unsigned int numeventstrings;
	unsigned char *instr;
};
#if 0
typedef enum
{
	VM_THREAD_FLAG_NONE = 0,
	VM_THREAD_FLAG_METHOD_CALL = BIT(0)
} e_vm_thread_flags;
#endif //maybe later, since for now just push to stack

typedef struct
{
	size_t numargs;
	varval_t *arguments[16];//should be enough right
	varval_t *object;
	//int type;
	int name;
} vm_event_t;

typedef struct
{
	void*(*cfunc)(void);
	vm_function_t callback;
	vm_t *vm; //if we're using multiple vms
	int numargs; //unused for now
	bool inuse;
	int flags;
} vm_ffi_callback_t;

#define VM_MAX_FFI_CALLBACKS (64)

typedef struct
{
	unsigned char *data;
	size_t size;
	char tag[128];
} vm_program_t;

struct vm_s {
	//char *program;
	//int program_size;

#if 1
	void* (*__memory_allocator)(size_t);
	void(*__memory_deallocator)(void*);

	size_t __mem_size, __mem_inuse;
	intptr_t *__mem_block;
	vector __mem_allocations;
#endif

	varval_t *level;
	//varval_t *self;

	vector ffi_callbacks;

	//vector vars;

	intptr_t stack[100]; //small stack for pushing stuff to the threads which pops it later again
	intptr_t registers[e_vm_reg_len];

	/* //are now thread specific
	int registers[e_vm_reg_len];
	int stack[VM_STACK_SIZE]; //when adding pseudo threads (co-routines) each should have own seperate stack imo
	int numargs; //mainly for builtin funcs
	*/
#define MAX_SCRIPT_THREADS 1024
	vm_thread_t *threadrunners;
	int numthreadrunners;
	vm_thread_t *thrunner;
#define MAX_CACHED_VARIABLES (1<<12)
	//varval_t varcache[MAX_CACHED_VARS];
	//unsigned int varcachesize;
	//dynarray varcachearray;
	//vector varcachearray;
	dynstack varcacheavail;
	size_t varcacheindex;
	size_t varcachemax;

	bool is_running;
	bool close_requested;

	int cast_stack[100];
	int cast_stack_ptr;
	vector functioninfo;

	intptr_t tmpstack[512];

	vt_istring_t *istringlist;
	int istringlistsize;

	stockfunction_t *stockfunctionsets[16];
	stockmethod_t *stockmethodsets[16];
	int stockmethodobjecttypes[16];
	int numstockfunctionsets;
	int numstockmethodsets;

	void *m_userpointer;
	int (*m_printf_hook)(const char *, ...);
	dynarray structs;
	dynarray libs;
	dynarray events;
	dynarray programs;
	unsigned char *instr;
};

#define vm_stack vm->thrunner->stack
#define vm_registers vm->thrunner->registers
//#define vm_printf vm->m_printf_hook

//#define stack_push(vm, x) (vm->thrunner==NULL?vm->stack[++vm->registers[REG_SP]]:vm->thrunner->stack[++vm_registers[REG_SP]] = (intptr_t)x)
//#define stack_pop(vm) (vm->thrunner==NULL?vm->stack[vm->registers[REG_SP]--]:vm->thrunner->stack[vm_registers[REG_SP]--])

#include <stdio.h> //getchar

static void stack_push(vm_t *vm, intptr_t x) {
	if (vm->thrunner == NULL)
		vm->stack[++vm->registers[REG_SP]] = x;
	else {
		if (vm->thrunner->registers[REG_SP] + 1 >= vm->thrunner->stacksize) {
			vm_printf("REG_SP=%d,STACKSIZE=%d\n", vm->thrunner->registers[REG_SP], vm->thrunner->stacksize);
			getchar();
			return;
		}
		vm->thrunner->stack[++vm->thrunner->registers[REG_SP]] = x;
	}
}

varval_t *vv_cast(vm_t *vm, varval_t *vv, int desired_type);
static void stack_push_vv(vm_t *vm, varval_t *x) {
	if (vm->cast_stack_ptr <= 0)
		stack_push(vm, (intptr_t)x);
	else
	{
		while (vm->cast_stack_ptr > 0)
		{
			int cast_type = vm->cast_stack[--vm->cast_stack_ptr];
			stack_push(vm, vv_cast(vm, x, cast_type));// stack_push_vv(vm, vv_cast(vm, x, cast_type));
			//vm_printf("desired cast type = %s, current = %s\n", e_var_types_strings[cast_type], e_var_types_strings[VV_TYPE(vv)]);
			se_vv_free(vm, x);
		}
	}
}

static intptr_t stack_get(vm_t *vm, int at) {
	if (vm->thrunner == NULL)
		return vm->stack[vm->registers[REG_SP] - at];
	return vm->thrunner->stack[vm->thrunner->registers[REG_SP] - at];
}

#define stack_current(x) (stack_get(vm,0))

static intptr_t stack_pop(vm_t *vm) {
	if (vm->thrunner == NULL)
		return vm->stack[vm->registers[REG_SP]--];
	return vm->thrunner->stack[vm->thrunner->registers[REG_SP]--];
}

static varval_t *stack_pop_vv(vm_t *vm) {
	return (varval_t*)stack_pop(vm);
}

typedef vm_long_t vm_hash_t;

typedef struct
{
	char name[256];
	vm_hash_t hash;
	void *address;
} vm_ffi_lib_func_t;

typedef struct
{
	char name[256];
	dynarray functions;
	vm_hash_t hash;
	void *handle;
} vm_ffi_lib_t;

void vm_use_program(vm_t *vm, unsigned int program);
void vm_thread_reset_events(vm_t *vm, vm_thread_t *thr);
bool vm_thread_is_stalled(vm_t *vm, vm_thread_t *thr);
vm_ffi_lib_func_t *vm_library_function_get(vm_t *vm, vm_ffi_lib_t *lib, const char *n);
vm_ffi_lib_t *vm_library_get(vm_t *vm, const char *n);
vm_ffi_lib_func_t *vm_library_function_get_any(vm_t *vm, const char *n, vm_ffi_lib_t **which_lib);

void* vm_library_handle_open(const char *libname);
void vm_library_handle_close(void *p);

#ifdef MEMORY_DEBUG
#define vm_mem_free(a,b) (vm_mem_free_r(a,b,__FILE__,__LINE__))
#define vm_mem_alloc(a,b) (vm_mem_alloc_r(a,b,__FILE__,__LINE__))
void *vm_mem_alloc_r(vm_t*, size_t, const char *, int);
void vm_mem_free_r(vm_t *vm, void*, const char *, int);
#else
void *vm_mem_alloc_r(vm_t*, size_t);
void vm_mem_free_r(vm_t *vm, void*);
#define vm_mem_alloc vm_mem_alloc_r
#define vm_mem_free vm_mem_free_r
#endif
