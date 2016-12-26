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

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
/*
 * Vector	Description					Type 		Error code
 * 0		Divide error					Fault		No
 * 1		Debug exception							No
 * 2		Mon-maskable interrupt		Interrupt	No
 * 3		Breakpoint					Trap			No
 * 4		Overflow						Trap			No
 * 5		Bounds check					Fault		No
 * 6		Illegal opcode				Fault		No
 * 7		Device not available			Fault		No
 * 8		Double fault					Abort		Zero
 * 9		Reserved
 * 10		Invalid task switch segment	Fault		Yes
 * 11		Segment not present			Fault		Yes
 * 12		Stack exception				Fault		Yes
 * 13		General protection fault		Fault		Yes
 * 14		Page fault					Fault		Yes
 * 15		Reserved
 * 16		Floating point error			Fault		No
 * 17		Aligment check				Fault		Zero
 * 18		Machine check				Abort		No
 * 19		SIMD floating point error	Fault		No
 * 20-31	Reserved						
 * 32-255	User defined interrupts		Interrupt	No
 * 48       System call
 */
 
/* TRAPHANDLER_NOEC trap handler with no error code */
TRAPHANDLER_NOEC(entry0, T_DIVIDE);
TRAPHANDLER_NOEC(entry1, T_DEBUG);
TRAPHANDLER_NOEC(entry2, T_NMI);
TRAPHANDLER_NOEC(entry3, T_BRKPT);
TRAPHANDLER_NOEC(entry4, T_OFLOW);
TRAPHANDLER_NOEC(entry5, T_BOUND);
TRAPHANDLER_NOEC(entry6, T_ILLOP);
TRAPHANDLER_NOEC(entry7, T_DEVICE);
TRAPHANDLER(entry8, T_DBLFLT);
/* TRAPHANDLER(entry9, T_COPROC ); */
TRAPHANDLER(entry10, T_TSS);
TRAPHANDLER(entry11, T_SEGNP);
TRAPHANDLER(entry12, T_STACK);
TRAPHANDLER(entry13, T_GPFLT);
TRAPHANDLER(entry14, T_PGFLT);
/* TRAPHANDLER(entry15, T_RES);*/
TRAPHANDLER_NOEC(entry16, T_FPERR);
TRAPHANDLER(entry17, T_ALIGN);
TRAPHANDLER_NOEC(entry18, T_MCHK);
TRAPHANDLER_NOEC(entry19, T_SIMDERR );

TRAPHANDLER_NOEC(entry48, T_SYSCALL);

// The IDT entries 32-47 correspond to the IRQs 0-15. 
/*
// Hardware IRQ numbers. We receive these as (IRQ_OFFSET+IRQ_WHATEVER)
#define IRQ_TIMER        0
#define IRQ_KBD          1
#define IRQ_SERIAL       4
#define IRQ_SPURIOUS     7
#define IRQ_IDE         14
#define IRQ_ERROR       19
*/
TRAPHANDLER_NOEC(irq0, IRQ_OFFSET+IRQ_TIMER);
TRAPHANDLER_NOEC(irq1, IRQ_OFFSET+IRQ_KBD);
TRAPHANDLER_NOEC(irq2, IRQ_OFFSET+2);
TRAPHANDLER_NOEC(irq3, IRQ_OFFSET+3);
TRAPHANDLER_NOEC(irq4, IRQ_OFFSET+IRQ_SERIAL);
TRAPHANDLER_NOEC(irq5, IRQ_OFFSET+5);
TRAPHANDLER_NOEC(irq6, IRQ_OFFSET+6);
TRAPHANDLER_NOEC(irq7, IRQ_OFFSET+IRQ_SPURIOUS);
TRAPHANDLER_NOEC(irq8, IRQ_OFFSET+8);
TRAPHANDLER_NOEC(irq9, IRQ_OFFSET+9);
TRAPHANDLER_NOEC(irq10, IRQ_OFFSET+10);
TRAPHANDLER_NOEC(irq11, IRQ_OFFSET+11);
TRAPHANDLER_NOEC(irq12, IRQ_OFFSET+12);
TRAPHANDLER_NOEC(irq13, IRQ_OFFSET+13);
TRAPHANDLER_NOEC(irq14, IRQ_OFFSET+IRQ_IDE);
TRAPHANDLER_NOEC(irq15, IRQ_OFFSET+15);
TRAPHANDLER_NOEC(irq19, IRQ_OFFSET+IRQ_ERROR);


.globl sysenter_handler;
.type sysenter_handler, @function;
.align 2;
sysenter_handler:
/*
 * Lab 3: Your code here for system call handling
 */
 	
	pushl $GD_UD|3
	pushl %ebp
	pushfl
	pushl $GD_UT|3
	pushl %esi
	pushl $0
	pushl $0
	pushl %ds
	pushl %es
	pushal
	
	// kernel segment 
	movw $GD_KD, %ax
	movw %ax, %ds
	movw %ax, %es
	
	// syscall function, push trapframe as a pointer 
	pushl %esp
	call my_syscall
	
	// set trapframe back to register
	popl %esp
	popal
	popl %es
	popl %ds
	movl %ebp, %ecx
	movl %esi, %edx
	//sysenter will automatically disable interrupt, but sysexit won't recover it
	sti
	
	sysexit

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	/* push values to make the stack look like a struct Trapframe */
	pushw $0    #uint16_t tf_padding2
	pushw %ds	#uint16_t tf_ds
	pushw $0    #uint16_t tf_padding1
	pushw %es	#uint16_t tf_es
	pushal		#struct PushRegs tf_regs

	/* load GD_KD into %ds and %es */
    movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es
	
	/* pushl %esp to pass a pointer to the Trapframe as an argument to trap() */
	pushl %esp
	
	/* call trap */
	call trap
