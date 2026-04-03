/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

# trap entry points
# err code or no err code determined by intel 64 isa-32
# given in ordefr from trap.h

# no err code ones
TRAPHANDLER_NOEC(t_divide,  T_DIVIDE)
TRAPHANDLER_NOEC(t_debug,   T_DEBUG)
TRAPHANDLER_NOEC(t_nmi,     T_NMI)
TRAPHANDLER_NOEC(t_brkpt,   T_BRKPT)
TRAPHANDLER_NOEC(t_oflow,   T_OFLOW)
TRAPHANDLER_NOEC(t_bound,   T_BOUND)
TRAPHANDLER_NOEC(t_illop,   T_ILLOP)
TRAPHANDLER_NOEC(t_device,  T_DEVICE)

# err coded ones
TRAPHANDLER(t_dblflt,  T_DBLFLT)
TRAPHANDLER(t_tss,     T_TSS)
TRAPHANDLER(t_segnp,   T_SEGNP)
TRAPHANDLER(t_stack,   T_STACK)
TRAPHANDLER(t_gpflt,   T_GPFLT)
TRAPHANDLER(t_pgflt,   T_PGFLT)

# fperr has no err code
TRAPHANDLER_NOEC(t_fperr,   T_FPERR)

# has error code
TRAPHANDLER(t_align,   T_ALIGN)

# no err code
TRAPHANDLER_NOEC(t_mchk,    T_MCHK)
TRAPHANDLER_NOEC(t_simderr, T_SIMDERR)

# syscall
TRAPHANDLER_NOEC(t_syscall, T_SYSCALL)


# _alltraps: build a Trapframe and call trap(tf)

.globl _alltraps
.type _alltraps, @function
.align 2
_alltraps:
    # build struct Trapframe on the stack

    # by now hardware alr pushed eip, cs, eflags, esp/ss if user mode, and maybe err code
    # now need to push rest of Trapframe

    pushl %ds   # push data seg reg
    pushl %es   # push extra seg reg
    pushal      # push all general purpose regs on stack

    # switch to kernel data segments
    movw $GD_KD, %ax
    movw %ax, %ds
    movw %ax, %es

    # push curr stack pointer to Trapframe to trap()
    # C equiv: trap((struct Trapframe *)esp);
    pushl %esp
    call trap

    # trap() should not return (env_run -> iret), but just in case:
    addl $4, %esp
1:
    jmp 1b

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

// HINT 1 : TRAPHANDLER_NOEC(t_divide, T_DIVIDE);
//          Do something like this if there is no error code for the trap
// HINT 2 : TRAPHANDLER(t_dblflt, T_DBLFLT);
//          Do something like this if the trap includes an error code..
// HINT 3 : READ Intel's manual to check if the trap includes an error code
//          or not...

/*
 * Lab 3: Your code here for _alltraps
 */


