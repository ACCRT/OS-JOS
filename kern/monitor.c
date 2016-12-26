// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
// Used in lab3 to run c,si,x command when breakpoint
#include <kern/env.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "time","Display time the function need", mon_time },
	{ "c", "Continue to run", mon_c},
	{ "si", "Step one", mon_si},
	{ "x", "", mon_x}
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

int
mon_time(int argc, char **argv, struct Trapframe *tf)
{
	char* fn_name = argv[1];
	int start = 0;
	int end = 0;
	if(strcmp(fn_name,"kerninfo") == 0)
	{
		start = read_tsc();
		mon_kerninfo(argc,argv,tf);
		end = read_tsc();
	}
	else if(strcmp(fn_name,"help") == 0)
	{
		start = read_tsc();
		mon_help(argc,argv,tf);
		end = read_tsc();
	}
	cprintf("kerninfo cycles: %d\n",end-start);
	return 0;
}

int
mon_c(int argc, char **argv, struct Trapframe *tf)
{
	if (tf == NULL) 
	{
		cprintf("No trapped environment\n");
		return 1;
	}
	
	// make trap frame's eflag be not traped
	tf->tf_eflags = tf->tf_eflags & (~FL_TF);
	env_run(curenv);
	return 0;
}

int
mon_si(int argc, char **argv, struct Trapframe *tf)
{
	if (tf == NULL) 
	{
		cprintf("No trapped environment\n");
		return 1;
	}
	
	// make trap frame's eflag be traped
	// FL_TF Eflags register trap flag
	tf->tf_eflags = tf->tf_eflags | FL_TF;

	cprintf("tf_eip=0x%x\n", tf->tf_eip);
	env_run(curenv);
	return 0;
}

int
mon_x(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 2) cprintf("please enter x addr");
	
	// read argv[1] as 16-base
	uint32_t addr = strtol(argv[1], NULL, 16);
	// get the value in address addr
	uint32_t val = *(int *)addr;
    
	cprintf("%d\n", val);
	return 0;
}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr() {
    uint32_t pretaddr;
    __asm __volatile("leal 4(%%ebp), %0" : "=r" (pretaddr)); 
    return pretaddr;
}

void
do_overflow(void)
{
    cprintf("Overflow success\n");
}

void
start_overflow(void)
{
	// You should use a techique similar to buffer overflow
	// to invoke the do_overflow function and
	// the procedure must return normally.

    // And you must use the "cprintf" function with %n specifier
    // you augmented in the "Exercise 9" to do this job.

    // hint: You can use the read_pretaddr function to retrieve 
    //       the pointer to the function call return address;

    char str[256] = {};
    int nstr = 0;
    uint32_t *pret_addr;
	// Your code here.
    
	uint32_t* ebp_addr = (uint32_t*)read_ebp();
	uint32_t* esp_addr = (uint32_t*)read_esp();
	uint32_t ebp = *ebp_addr;
	uint32_t number = ((uint32_t)ebp_addr-(uint32_t)str);
	int i;
	for(i=0;i<number;i++)
		str[i] = 1;
	
	//str[number+3] = (ebp & 0xff000000) >> 24;
	//str[number+2] = (ebp & 0x00ff0000) >> 16;
	//str[number+1] = (ebp & 0x0000ff00) >> 8;
	//str[number] = ebp & 0x000000ff;
	
	// way1
	uint32_t do_over = (uint32_t)do_overflow+6;
	str[(do_over & 0x000000ff)] = 0;
	cprintf("%s%n",str,str+number+4);
	str[(do_over & 0x000000ff)] = 1;
	str[(do_over & 0x0000ff00) >> 8] = 0;
	cprintf("%s%n",str,str+number+5);
	str[(do_over & 0x0000ff00) >> 8] = 1;	
	str[(do_over & 0x00ff0000) >> 16] = 0;
	cprintf("%s%n",str,str+number+6);	
	str[(do_over & 0x00ff0000) >> 16] = 1;
	str[(do_over & 0xff000000) >> 24] = 0;
	cprintf("%s%n",	str,str+number+7);
	str[(do_over & 0xff000000) >> 24] = 1;
	
	// way2
	//str[number+4] = (do_over & 0x000000ff);
	//str[number+5] = (do_over & 0x0000ff00) >> 8;
	//str[number+6] = (do_over & 0x00ff0000) >> 16;
	//str[number+7] = (do_over & 0xff000000) >> 24;
}

void
overflow_me(void)
{
        start_overflow();
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	uint32_t *ebp_addr;
	cprintf("Stack backtrace:\n");
	ebp_addr = (uint32_t*)read_ebp();
	while(ebp_addr)
	{
		uint32_t old_ebp = ebp_addr[0];
        uint32_t ret_addr = ebp_addr[1];
        struct Eipdebuginfo info;
        debuginfo_eip(ret_addr, &info);
        cprintf("  eip %08x  ebp %08x  args %08x %08x %08x %08x %08x\n"
        		"         %s:%d: %.*s+%u\n" ,
        		ret_addr, 
        		ebp_addr, 
        		ebp_addr[2], 
        		ebp_addr[3], 
        		ebp_addr[4],
        		ebp_addr[5],
        		ebp_addr[6],
        		info.eip_file,
        		info.eip_line,
        		info.eip_fn_namelen,
        		info.eip_fn_name,
        		ret_addr - info.eip_fn_addr);
        ebp_addr = (uint32_t *)old_ebp;
	}
    overflow_me();	
    cprintf("Backtrace success\n");
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
