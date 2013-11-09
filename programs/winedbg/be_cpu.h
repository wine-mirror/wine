/*
 * Debugger CPU backend definitions
 *
 * Copyright 2004 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

enum be_cpu_addr {be_cpu_addr_pc, be_cpu_addr_stack, be_cpu_addr_frame};
enum be_xpoint_type {be_xpoint_break, be_xpoint_watch_exec, be_xpoint_watch_read,
                     be_xpoint_watch_write};
struct backend_cpu
{
    const DWORD         machine;
    const DWORD         pointer_size;
    /* ------------------------------------------------------------------------------
     * address manipulation
     * ------------------------------------------------------------------------------ */
    /* Linearizes an address. Only CPUs with segmented address model need this.
     * Otherwise, implementation is straightforward (be_cpu_linearize will do)
     */
    void*               (*linearize)(HANDLE hThread, const ADDRESS64*);
    /* Fills in an ADDRESS64 structure from a segment & an offset. CPUs without
     * segment address model should use 0 as seg. Required method to fill
     * in an ADDRESS64 (except an linear one).
     * Non segmented CPU shall use be_cpu_build_addr
     */
    unsigned            (*build_addr)(HANDLE hThread, const CONTEXT* ctx, 
                                      ADDRESS64* addr, unsigned seg,
                                      unsigned long offset);
    /* Retrieves in addr an address related to the context (program counter, stack
     * pointer, frame pointer)
     */
    unsigned            (*get_addr)(HANDLE hThread, const CONTEXT* ctx, 
                                    enum be_cpu_addr, ADDRESS64* addr);

    /* returns which kind of information a given register number refers to */
    unsigned            (*get_register_info)(int regno, enum be_cpu_addr* kind);

    /* -------------------------------------------------------------------------------
     * context manipulation 
     * ------------------------------------------------------------------------------- */
    /* Enables/disables CPU single step mode (depending on enable) */
    void                (*single_step)(CONTEXT* ctx, BOOL enable);
    /* Dumps out the content of the context */
    void                (*print_context)(HANDLE hThread, const CONTEXT* ctx, int all_regs);
    /* Prints information about segments. Non segmented CPU should leave this
     * function empty
     */
    void                (*print_segment_info)(HANDLE hThread, const CONTEXT* ctx);
    /* all the CONTEXT's relative variables, bound to this CPU */
    const struct dbg_internal_var* context_vars;

    /* -------------------------------------------------------------------------------
     * code inspection 
     * -------------------------------------------------------------------------------*/
    /* Check whether the instruction at addr is an insn to step over
     * (like function call, interruption...)
     */
    unsigned            (*is_step_over_insn)(const void* addr);
    /* Check whether instruction at 'addr' is the return from a function call */
    unsigned            (*is_function_return)(const void* addr);
    /* Check whether instruction at 'addr' is the CPU break instruction. On i386, 
     * it's INT3 (0xCC)
     */
    unsigned            (*is_break_insn)(const void*);
    /* Check whether instruction at 'addr' is a function call */
    unsigned            (*is_function_call)(const void* insn, ADDRESS64* callee);
    /* Check whether instruction at 'addr' is a jump */
    unsigned            (*is_jump)(const void* insn, ADDRESS64* jumpee);
    /* Ask for disassembling one instruction. If display is true, assembly code
     * will be printed. In all cases, 'addr' is advanced at next instruction
     */
    void                (*disasm_one_insn)(ADDRESS64* addr, int display);
    /* -------------------------------------------------------------------------------
     * break points / watchpoints handling 
     * -------------------------------------------------------------------------------*/
    /* Inserts an Xpoint in the CPU context and/or debuggee address space */
    unsigned            (*insert_Xpoint)(HANDLE hProcess, const struct be_process_io* pio,
                                         CONTEXT* ctx, enum be_xpoint_type type,
                                         void* addr, unsigned long* val, unsigned size);
    /* Removes an Xpoint in the CPU context and/or debuggee address space */
    unsigned            (*remove_Xpoint)(HANDLE hProcess, const struct be_process_io* pio,
                                         CONTEXT* ctx, enum be_xpoint_type type,
                                         void* addr, unsigned long val, unsigned size);
    /* Checks whether a given watchpoint has been triggered */
    unsigned            (*is_watchpoint_set)(const CONTEXT* ctx, unsigned idx);
    /* Clears the watchpoint indicator */
    void                (*clear_watchpoint)(CONTEXT* ctx, unsigned idx);
    /* After a break instruction is executed, in the corresponding exception handler,
     * some CPUs report the address of the insn after the break insn, some others 
     * report the address of the break insn itself.
     * This function lets adjust the context PC to reflect this behavior.
     */
    int                 (*adjust_pc_for_break)(CONTEXT* ctx, BOOL way);
    /* -------------------------------------------------------------------------------
     * basic type read/write 
     * -------------------------------------------------------------------------------*/
    /* Reads an integer from memory and stores it inside a long long int */
    int                 (*fetch_integer)(const struct dbg_lvalue* lvalue, unsigned size, unsigned is_signed, LONGLONG*);
    /* Reads a real from memory and stores it inside a long double */
    int                 (*fetch_float)(const struct dbg_lvalue* lvalue, unsigned size, long double*);
    /* Writes an integer to memory */
    int                 (*store_integer)(const struct dbg_lvalue* lvalue, unsigned size, unsigned is_signed, LONGLONG);
};

extern struct backend_cpu*      be_cpu;

/* some handy functions for non segmented CPUs */
void*    be_cpu_linearize(HANDLE hThread, const ADDRESS64*);
unsigned be_cpu_build_addr(HANDLE hThread, const CONTEXT* ctx, ADDRESS64* addr, 
                           unsigned seg, unsigned long offset);
