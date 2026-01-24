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

#include <kern/hidden.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// LAB 1: add your command to here...
static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "hidden", "Run hidden test cases", exec_hidden_cases},
	{ "show", "Display ASCII art for Lab 1 extra credit", mon_show },
	{ "backtrace", "Display stack backtrace", mon_backtrace }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// LAB 1: Your code here.
    // HINT 1: use read_ebp().
    // HINT 2: print the current ebp on the first line (not current_ebp[0])

	/*
	FORMAT
	Stack backtrace:
		ebp f0109e58  eip f0100a62  args 00000001 f0109e80 f0109e98 f0100ed2 00000031
		ebp f0109ed8  eip f01000d6  args 00000000 00000000 f0100058 f0109f28 00000061
	*/

	cprintf("Stack backtrace:\n");
	uint32_t *curr_ebp = (uint32_t*) read_ebp();
	struct Eipdebuginfo info;
	uint32_t curr_eip;
	// according to example, each line has 2 spaces in front
	while (curr_ebp != 0) {
		curr_eip = *(curr_ebp + 1);

		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
			curr_ebp,
			curr_eip,
			*(curr_ebp + 2),
			*(curr_ebp + 3),
			*(curr_ebp + 4),
			*(curr_ebp + 5),
			*(curr_ebp + 6)
		);

		// implement debuginfo_eip to get more info
		// according to example, debuginfo has 9 spaces in front for print statement
		debuginfo_eip(curr_eip, &info);
		cprintf("         %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, curr_eip - info.eip_fn_addr);

		curr_ebp = (uint32_t*) *(curr_ebp); // next ebp
	}

	
	return 0;
}

int
mon_show(int argc, char **argv, struct Trapframe *tf)
{	
	// \033 == ESC, then [, then number (30 to 37), then m
    cprintf("\033[31m   _______   \n");
    cprintf("\033[32m  /       \\  \n");
    cprintf("\033[33m |  o   o  | \n");
    cprintf("\033[34m |    ^    | \n");
    cprintf("\033[35m |  \\___/  | \n");
    cprintf("\033[36m  \\_______/  \n");
    cprintf("\033[0m"); // reset
    return 0;
}

int exec_hidden_cases(int argc, char **argv, struct Trapframe *tf) {
	hidden_test_cases();
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
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
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


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
