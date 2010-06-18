// This class represents loaded graphite stack machine code.  It performs 
// basic sanity checks, on the incoming code to prevent more obvious problems
// from crashing graphite.
// Author: Tim Eves

#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include "code.h"
#include "machine.h"


code::code(bool constrained, const byte * bytecode_begin, const byte * const bytecode_end)
: _code(0), _size(0), _instr_count(0)
{
    assert(bytecode_begin != 0);
    assert(bytecode_end > bytecode_begin);
    
    const opcode_t *    op_to_fn = machine::get_opcode_table(constrained);
    const byte *        cd_ptr = bytecode_begin - 1;
    
    // Allocate a target buffer
    _code = static_cast<instr *>(std::malloc((bytecode_end - bytecode_begin)
                                             * sizeof(instr)));
    instr * ip = _code;
    
    do {
        const machine::opcode opc = machine::opcode(*++cd_ptr);
        
        // Do some basic sanity checks based on what we know about the opcodes.
        if (opc >= machine::MAX_OPCODE) {   // Is this even a valid opcode?
            free(_code);
            throw std::range_error("invalid opcode");
        }

        const opcode_t op = op_to_fn[opc];
        if (op.param_sz == NILOP) {      // Is it implemented?
            free(_code);
            throw std::runtime_error("attempt to execute unimplemented opcode");
        }

        if (opc == machine::CNTXT_ITEM)  // This is a really conditional forward jump,
        {                       // check it doesn't jump outside the program.
            const size_t skip = cd_ptr[2];
            if (cd_ptr + 2 + skip > bytecode_end) {
                free(_code);
                throw std::runtime_error("cntxt_item: jump past end of program");
            }
        }
        
        size_t param_sz = op.param_sz == VARARGS ? *++cd_ptr : op.param_sz;
        if (cd_ptr + param_sz > bytecode_end) { // Is the requested size possible
            free(_code);
            throw std::runtime_error("arguments exhausted");
        }
        
        // Add this instruction
        *ip++ = op.impl; ++_instr_count;
        // Grab the parameters
        if (param_sz)
        {
            std::copy(cd_ptr + 1, cd_ptr + 1 + param_sz, reinterpret_cast<byte *>(ip));
            cd_ptr += param_sz;
            ip += (param_sz + sizeof(instr)-1)/sizeof(instr);
        }
        
        // Was this a return? stop processing any further.
        if (opc == machine::POP_RET 
         || opc == machine::RET_ZERO 
         || opc == machine::RET_TRUE)
            break;
    } while (cd_ptr < bytecode_end);
    
    // Final sanity check: ensure that the program is correctly terminated.
    switch (*cd_ptr) {
        case machine::POP_RET: 
        case machine::RET_ZERO: 
        case machine::RET_TRUE: 
            break;
        default:
            throw std::runtime_error("No return instruction found");
    }

    _size = sizeof(instr)*(ip - _code);
    _code = static_cast<instr *>(std::realloc(_code, _size));
}



code::~code() throw ()
{
    std::free(_code);
}


uint32 code::run(uint32 * stack_base, const size_t length,
                    Segment & seg, const int islot_idx)
{
    assert(stack_base != 0);
    assert(length >= 32);
    
    return machine::run(_code, stack_base, length, seg, islot_idx);
}

