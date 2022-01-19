/***********************************************************************************************************************
MMBasic

timers.h

Include file that contains the globals and defines for memory allocation for MMBasic running on the Maximite.

Copyright 2011 - 2019 Geoff Graham.  All Rights Reserved.

This file and modified versions of this file are supplied to specific individuals or organisations under the following
provisions:

- This file, or any files that comprise the MMBasic source (modified or not), may not be distributed or copied to any other
  person or organisation without written permission.

- Object files (.o and .hex files) generated using this file (modified or not) may not be distributed or copied to any other
  person or organisation without written permission.

- This file is provided in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

************************************************************************************************************************/



/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// format:
//      void cmd_???(void)
//      void fun_???(void)
//      void op_???(void)

void cmd_memory(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is always T_CMD
// and P is the precedence (which is only used for operators and not commands)

	{ "Memory",		T_CMD,				0, cmd_memory	},

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is T_NA, T_FUN, T_FNA or T_OPER augmented by the types T_STR and/or T_NBR
// and P is the precedence (which is only used for operators)

#endif


#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef MEMORY_HEADER
#define MEMORY_HEADER

extern volatile char *StrTmp[];                                              // used to track temporary string space on the heap
extern int TempMemoryTop;                                           // this is the last index used for allocating temp memory
extern int TempMemoryIsChanged;						                // used to prevent unnecessary scanning of strtmp[]

typedef enum _M_Req {M_PROG, M_VAR} M_Req;

extern void m_alloc(int size);
extern void *GetMemory(size_t msize);
extern void *GetTempMemory(int NbrBytes);
extern void *GetTempStrMemory(void);
extern void ClearTempMemory(void);
extern void ClearSpecificTempMemory(void *addr);
extern void TestStackOverflow(void);
extern void FreeMemory(void *addr);
extern void InitHeap(void);
extern char *HeapBottom(void);
extern int FreeSpaceOnHeap(void);

extern unsigned int _stack;
extern unsigned int _splim;
extern unsigned int _heap;
extern unsigned int _min_stack_size;
extern unsigned int _text_begin;
extern  uint32_t SavedMemoryBufferSize;
extern int MemSize(void *addr);


// RAM parameters
// ==============
// The following settings will allow MMBasic to use all the free memory on the PIC32.  If you need some RAM for
// other purposes you can declare the space needed as a static variable -or- allocate space to the heap (which
// will reduce the memory available to MMBasic) -or- change the definition of RAMEND.
// NOTE: MMBasic does not use the heap.  It has its own heap which is allocated out of its own memory space.

// The virtual address that MMBasic can start using memory.  This must be rounded up to PAGESIZE.
// MMBasic uses just over 5K for static variables so, in the simple case, RAMBase could be set to 0xA001800.
// However, the PIC32 C compiler provides us with a convenient marker (see diagram above).
extern void *RAMBase;
#define RAMBASE         0x20000000
#define RAMEND          0x20020000
#define MEMSIZE        512 * 1024//stm32f4

// other (minor) memory management parameters
#define RAMPAGESIZE        256                                         // the allocation granuality
#define PAGEBITS        2                                           // nbr of status bits per page of allocated memory, must be a power of 2

#define PUSED           0b01                                        // flag that indicates that the page is in use
#define PLAST           0b10                                        // flag to show that this is the last page in a single allocation

#define PAGESPERWORD    ((sizeof(unsigned int) * 8)/PAGEBITS)
#define MRoundUp(a)     (((a) + (RAMPAGESIZE - 1)) & (~(RAMPAGESIZE - 1)))// round up to the nearest page size


#endif
#endif

