const u8 MAX_LEGACY_PREFIXES = 4U;
const u8 legacy_prefixes[] = {
    // Operand-size override
    0x66,
    0x67,
    // Address-size override
    0x2E,
    0x3E,
    0x26,
    0x64,
    0x65,
    0x36,
    // Lock
    0xF0,
    // Repeat
    0xF2,
    0xF3,
};

const u8 REX_prefixes[] = {
    0x40,
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4A,
    0x4B,
    0x4C,
    0x4D,
    0x4E,
    0x4F,
};

const char* ModRM_registers[8U][5U] = {
    { "YMM0", "XMM0", "MMX0", "RAX" },
    { "YMM1", "XMM1", "MMX1", "RCX" },
    { "YMM2", "XMM2", "MMX2", "RDX" },
    { "YMM3", "XMM3", "MMX3", "RBX" },
    { "YMM4", "XMM4", "MMX4", "RSP", "AH" },
    { "YMM5", "XMM5", "MMX5", "RBP", "CH" },
    { "YMM6", "XMM6", "MMX6", "RSI", "DH" },
    { "YMM7", "XMM7", "MMX7", "RDI", "BH" }
};

const char* SIB_index_registers[8U] = {
    "RAX",
    "RCX",
    "RDX",
    "RBX",
    "",
    "RBP",
    "RSI",
    "RDI"
};

const char* SIB_base_registers[8U] = {
    "RAX",
    "RCX",
    "RDX",
    "RBX",
    "RSP",
    "RBP",
    "RSI",
    "RDI"
};

struct operand {
    // Default operand size in 64bit mode is 32bit.
    // If 66h legacy prefix is present 16bit override the default operand size
    // If REX.W prefix is present 64bit override the default operand size
    // In some cases instruction forces 64bit operand size
    enum OPERAND_TYPE {
        None,
        Inst_FAR,   // A 
        TYPE_E,     // E
        ModRM_Reg,  // G
        Imm,        // I
        ModRM_MEM,  // M, mod != 0b11
        Vector_REG, // L
        DS_rSI,     // X
        ES_rSI,     // Y
    } src; // Handle eventual SIB byte following ModRM byte

    enum DATA_TYPE {
        TYPE_BYTE   = 0x1, // b
        TYPE_WORD   = 0x2,
        TYPE_DWORD  = 0x4,
        TYPE_QWORD  = 0x8,
        TYPE_FP     = 0x10,
        TYPE_v      = 0x1f, // v
        TYPE_p      = 0x20, // 32 or 48 bit depending on 16 or 32 bit operand size
        TYPE_z      = 0x1000, // z
        TYPE_1      = 0x2000, // Hardcoded 1
    } type;
    // SSE stuff
};

struct opcode {
    const char* name;
    struct operand src_operand;
    struct operand dst_operand;
};

const struct opcode rep_f3_opcode_map[] = {
    { "PAUSE", { .src = None }, { .src = None }}, // 0x90
}

const struct opcode primary_opcode_map[256U] = {
    { "ADD", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "ADD", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "ADD", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "ADD", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "ADD", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},             // AL, need to understand and handle
    { "ADD", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},           // TODO
    { "OR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "OR", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "OR", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "OR", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "OR", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},             // AL, need to understand and handle
    { "OR", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    {},                                                                                                                            // Escape to secondary op code map
    { "ADC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "ADC", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "ADC", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "ADC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "ADC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "ADC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "SBB", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "SBB", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "SBB", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "SBB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "SBB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "ADC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "AND", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "AND", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "AND", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "AND", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "AND", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "AND", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "seg", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "DAA", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "SUB", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "SUB", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "SUB", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "SUB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "SUB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "SUB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "seg", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "DAS", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "XOR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "XOR", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "XOR", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "XOR", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "XOR", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "XOR", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "seg", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "AAA", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "CMP", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},
    { "CMP", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},
    { "CMP", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},
    { "CMP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "CMP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "CMP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},
    { "seg", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "AAS", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "INC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},    // TODO
    { "INC", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},           // TODO
    { "INC", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO
    { "INC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "INC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "INC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "INC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "INC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "DEC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},    // TODO
    { "DEC", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},           // TODO
    { "DEC", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO
    { "DEC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "DEC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // AL, need to understand and handle
    { "DEC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "DEC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "DEC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "PUSH", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},   // TODO
    { "PUSH", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},   // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "POP", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},    // TODO
    { "POP", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},           // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "POP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "PUSHA", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},  // TODO
    { "POPA", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "BOUND", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},  // TODO
    { "ARPL", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "seg", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "seg", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    {},                                                                                                                           // Operand size overide prefix
    {},                                                                                                                           // Address size override prefix
    { "PUSH", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},  // TODO
    { "IMUL", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},         // TODO
    { "PUSH", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},  // TODO
    { "IMUL", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "INSB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},          // TODO
    { "INSW/D", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},      // TODO
    { "OUTS/B", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},        // TODO
    { "OUTS/W/D", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},    // TODO
    { "JO", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},    // TODO
    { "JNO", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "JB", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO
    { "JNB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "JZ", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "JNZ", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "JBE", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "JNBE", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "JS", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},    // TODO
    { "JNS", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "JP", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO
    { "JNP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "JL", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},            // TODO
    { "JNL", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "JLE", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "JNLE", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    {},                                                                                                                           // Group 1 Opcode
    {},                                                                                                                           // Group 1 Opcode
    {},                                                                                                                           // Group 1 Opcode
    {},                                                                                                                           // Group 1 Opcode
    { "TEST", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},          // TODO
    { "TEST", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},          // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "MOV", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},   // TODO
    { "MOV", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},   // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "LEA", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    {},                                                                                                                           // Group 1 Opcode
    { "NOP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "XCHG", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "CBW/WDE/DQE", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }}, // TODO
    { "CWD/DQ/QO", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "CALL", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "F/WAIT", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},      // TODO
    { "PUSHF/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "POPF/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},    // TODO
    { "SAHF", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "LAHF", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOVSB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},       // TODO
    { "MOBSW/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "CMPSB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},       // TODO
    { "CMPSW/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "TEST", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "TEST", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "STOSB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},       // TODO
    { "STOW/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},    // TODO
    { "LODSB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},       // TODO
    { "LODSW/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "SCASB", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},       // TODO
    { "SCASW/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "MOV", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    {},                                                                                                                           // Group 2 opcode
    {},                                                                                                                           // Group 2 opcode
    { "RET", { .src = ModRM_Reg, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},   // TODO
    { "RET", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "LES", { .src = ModRM_Reg, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},           // TODO
    { "LDS", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    {},                                                                                                                           // Group 11 opcode
    {},                                                                                                                           // Group 11 opcode
    { "ENTER", { .src = ModRM_Reg, .type = TYPE_WORD }, { .src = Imm, .type = TYPE_BYTE }},
    { "LEAVE", { .src = None }, { .src = None }},
    { "RET far", { .src = Imm, .type = TYPE_WORD }, { .src = None }},
    { "RET far", { .src = None }, { .src = None }},
    { "INT 3", { .src = None }, { .src = None }},
    { "INT", { .src = Imm, .type = TYPE_BYTE }, { .src = None }},
    { "INTO", { .src = None }, { .src = None }},                                                 // Invalid in 64bit mode
    { "IRET/D/Q", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},    // TODO
    {},                                                                                                                           // Group 2 Opcode
    {},                                                                                                                           // Group 2 Opcode
    {},                                                                                                                           // Group 2 Opcode
    {},                                                                                                                           // Group 2 Opcode
    { "AAM", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},   // TODO
    { "AAD", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    {},                                                                                                                           // invalid
    { "XLAT", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},         // TODO
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    {},                                                                                                                           // TODO x87 instructions
    { "LOOPNE/NZ", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},   // TODO
    { "LOOPE/Z", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},     // TODO
    { "LOOP", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "JrCXZ", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},       // TODO
    { "IN", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "IN", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "OUT", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "OUT", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "CALL", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "JMP", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "JMP", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},   // TODO
    { "JMP", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "IN", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "IN", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},          // TODO
    { "OUT", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "OUT", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "LOCK", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},        // TODO
    { "INT1", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},         // TODO
    { "REPNE", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }}, // TODO
    { "REP/E", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},        // TODO
    { "HLT", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "CMC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    {},                                                                                                                           // Group 3 opcode
    {},                                                                                                                           // Group 3 opcode
    { "CLC", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "STC", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "CLI", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = ModRM_Reg, .type = TYPE_BYTE }},   // TODO
    { "STI", { .src = TYPE_E, .type = TYPE_v}, { .src = ModRM_Reg, .type = TYPE_v }},          // TODO
    { "CLD", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "STD", { .src = ModRM_Reg, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    {},                                                                                                                           // Group 4 opcode
    {},                                                                                                                           // Group 5 opcode
};

// TODO: Think about a smart way to retrieve groups opcodes
const struct opcode group_1[32U] = {
    // Opcode 0x80
    { "ADD", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "OR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "ADC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SBB", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "AND", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SUB", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "XOR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "CMP", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    // Opcode 0x81
    { "ADD", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "OR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "ADC", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "SBB", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "AND", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "SUB", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "XOR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "CMD", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    // Opcode = 0x82
    { "ADD", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "OR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "ADC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SBB", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "AND", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SUB", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "XOR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "CMP", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    // Opcode 0x83
    { "ADD", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "OR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "ADC", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "SBB", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "AND", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "SUB", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "XOR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "CMP", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
};

const struct opcode group_1a[8U] = {
    { "POP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
    { "XOP", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_v }},         // TODO
};

const struct opcode group_2[48U] = {
    // Opcode 0xC0
    { "ROL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "ROR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "RCL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "RCR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SHR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "SAR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    // Opcode 0xC1
    { "ROL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "ROR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "RCL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "RCR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "SHR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    { "SAR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_BYTE }},
    // Opcode 0xD0
    { "ROL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "ROR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "RCL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "RCR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "SHR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    { "SAR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_1 }},
    // Opcode 0xD1
    { "ROL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "ROR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "RCL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "RCR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "SHR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    { "SAR", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_1 }},
    // Opcode 0xD2
    { "ROL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},        // TODO handle CL
    { "ROR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},        // TODO handle CL
    { "RCL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},        // TODO handle CL
    { "RCR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},        // TODO handle CL
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO handle CL
    { "SHR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},        // TODO handle CL
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},    // TODO handle CL
    { "SAR", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = TYPE_E, .type = TYPE_BYTE }},        // TODO handle CL
    // Opcode 0xD3
    { "ROL", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},           // TODO handle CL
    { "ROR", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},           // TODO handle CL
    { "RCL", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},           // TODO handle CL
    { "RCR", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},           // TODO handle CL
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},       // TODO handle CL
    { "SHR", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},           // TODO handle CL
    { "SHL/SAL", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},       // TODO handle CL
    { "SAR", { .src = TYPE_E, .type = TYPE_v }, { .src = TYPE_E, .type = TYPE_BYTE }},           // TODO handle CL
};

const struct opcode group_3[16U] = {
    { "TEST", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "TEST", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "NOT", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "NEG", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "MUL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "IMUL", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "DIV", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "IDIV", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "TEST", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
    { "TEST", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "NOT", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "NEG", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "MUL", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "IMUL", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "DIV", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "IDIV", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
};

const struct opcode group_4[2U] = {
    { "INC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
    { "DEC", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = None }},
};

const struct opcode group_5[7U] = {
    { "INC", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "DEC", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "CALL", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "CALL", { .src = ModRM_MEM, .type = TYPE_p }, { .src = None }},
    { "JMP", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
    { "JMP", { .src = ModRM_MEM, .type = TYPE_p }, { .src = None }},
    { "PUSH", { .src = TYPE_E, .type = TYPE_v }, { .src = None }},
};

const struct opcode group_11[2U] = {
    { "MOV", { .src = TYPE_E, .type = TYPE_BYTE }, { .src = Imm, .type = TYPE_BYTE }},
    { "MOV", { .src = TYPE_E, .type = TYPE_v }, { .src = Imm, .type = TYPE_z }},
};

u8 is_legacy_prefix(u8 byte) {
    for (u8 index = 0U; index < sizeof(legacy_prefixes); index++) {
        if (byte == legacy_prefixes[index]) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 is_REX_prefix(u8 byte) {
    for (u8 index = 0U; index < sizeof(REX_prefixes); index++) {
        if (byte == REX_prefixes[index]) {
            return TRUE;
        }
    }
    return FALSE;
}

const struct opcode* primary_map_group(u8 opcode, u8 ModRM) {
    const u8 reg_size = 0x8;
    const u8 reg_byte_mask = 0b111;
    u8 reg = (ModRM >> 3U) & reg_byte_mask;
    if (0x80 <= opcode && opcode <= 0x83) {
        return &group_1[(opcode - 0x80) * reg_size + reg];
    } else if (opcode == 0x8F) {
        return &group_1a[reg];
    } else if (opcode == 0xC0 || opcode == 0xC1) {
        return &group_2[(opcode == 0xC1) * reg_size + reg];
    } else if (0xD0 <= opcode && opcode <= 0xD3) {
        return &group_2[((opcode - 0xD0) + 2U) * reg_size + reg];
    } else if (opcode == 0xF6 || opcode == 0xF7) {
        return &group_3[(opcode - 0xF6) * reg_size + reg];
    } else if (opcode == 0xFE) {
        return &group_4[reg];
    } else if (opcode == 0xFF) {
        return &group_5[reg];
    } else if (opcode == 0xC6 || opcode == 0xC7) {
        return &group_11[opcode == 0xC7];
    } else {
        return NULL;
    }
}

u64 get_immediate(u8** bytes, enum DATA_TYPE type, u8 operand_effective_size) {
    u64 imm = 0U;
    if (type == TYPE_BYTE) {
        imm = (u8)**bytes;
        *bytes += 1U;
    } else if (type == TYPE_WORD) {
        imm = (u16)**bytes;
        *bytes += 2U;
    } else if (type == TYPE_DWORD) {
        imm = (u32)**bytes;
        *bytes += 4U;
    } else if (type == TYPE_QWORD) {
        imm = (u64)**bytes;
        *bytes += 8U;
    } else if (type == TYPE_v) {
        if (operand_effective_size == 16U) {
            imm = (u16)**bytes;
        } else if (operand_effective_size == 32U) {
            imm = (u32)**bytes;
        } else if (operand_effective_size == 64U) {
            imm = (u64)**bytes;
        } else {
            print("ERROR: Unhandled operand type\n");
        }

        *bytes += operand_effective_size / 8U;
    } else if (type == TYPE_z) {
        if (operand_effective_size == 16U) {
            imm = (u16)**bytes;
            *bytes += 2U;
        } else {
            imm = (u32)**bytes;
            *bytes += 2U;
        }
    } else {
        print("ERROR: Unhandled operand type\n");
    }
    return imm;
}

u8 get_immediate_size(enum OPERAND_TYPE type, u8 operand_effective_size) {
    if (type == TYPE_BYTE) {
        return 1U;
    } else if (type == TYPE_WORD) {
        return 2U;
    } else if (type == TYPE_DWORD) {
        return 4U;
    } else if (type == TYPE_QWORD) {
        return 8U;
    } else if (type == TYPE_v) {
        return operand_effective_size / 8U;
    } else {
        print("ERROR: Unhandled operand type\n");
        return 0U;
    }
}

#define MODRM_MOD(ModRM) (ModRM >> 6U)
#define MODRM_REG(ModRM) (ModRM & 0b00111000) >> 0x3
#define MODRM_RM(ModRM)  (ModRM & 0b00000111)

#define SIB_SCALE(SIB) (SIB >> 6U)
#define SIB_INDEX(SIB) (SIB & 0b00111000) >> 0x3
#define SIB_BASE(SIB)  (SIB & 0b00000111)

#define REXW(REX) (REX & 0b00001000) >> 3U
#define REXR(REX) (REX & 0b00000100) >> 2U
#define REXX(REX) (REX & 0b00000010) >> 1U
#define REXB(REX) (REX & 0b00000001)

// void unassemble(CONTEXT* ctx, u32 count) {
void unassemble(u8* start, u64 size) {
    // u8* current_byte = (u8*)ctx->Rip;
    u8* inst_buffer = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!read_memory((const void*)start, (void*)inst_buffer, size)) {
        print("Failed to read instructions\n");
    }

    u8* current_byte = inst_buffer;
    while ((u64)(current_byte - inst_buffer) < size) {
        u8* inst_start = current_byte;
        u8 legacy_prefixes_count = 0U;
        u8 effective_operand_size = 32U; // 32 by default in 64bit mode
        i32 effective_address = -1;
        u8 operand_size_override_66 = FALSE;
        u8 operand_size_override_REX = FALSE;
        u8 REX = 0U;
        while (is_legacy_prefix(*current_byte) && legacy_prefixes_count < MAX_LEGACY_PREFIXES) {
            if (*current_byte == 0x66) { // Override effective operand size
                effective_operand_size = 16U;
                operand_size_override_66 = TRUE;
            }
            legacy_prefixes_count++;
            current_byte++;
        }

        if (is_REX_prefix(*current_byte)) {
            REX = *current_byte;

            if (REXW(REX) && operand_size_override_66 == FALSE) {
                effective_operand_size = 64U;
                operand_size_override_REX = TRUE;
            }

            current_byte++;
        }

        // TODO: VEX, XOP prefixes

        u8 op_byte = *current_byte;
        current_byte++;
        u8 ModRM = *current_byte;
        const struct opcode* op = NULL;
        if (op_byte ==  0x0F) { // Secondary op code map needed
            current_byte++;
        } else if ((op = primary_map_group(op_byte, ModRM)) != NULL) {
        } else {
            op = &primary_opcode_map[op_byte];
        }

        u8 has_displacement = FALSE;
        u8 has_src_SIB = FALSE;
        u8 has_dst_SIB = FALSE;
        if (op->src_operand.src & (TYPE_E | ModRM_Reg) || op->dst_operand.src & (TYPE_E | ModRM_Reg)) {
            u8 Mod = MODRM_MOD(ModRM);
            u8 RM = MODRM_RM(ModRM);

            if (Mod == 0b10 || Mod == 0b01) {
                has_displacement = TRUE;
            }

            if (Mod != 0b11 && RM == 0b100) { // This may not be the only way to has an SIB byte
                if (op->src_operand.src & (TYPE_E | ModRM_Reg)) {
                    has_src_SIB = TRUE;
                }
                if (op->dst_operand.src & (TYPE_E | ModRM_Reg)) {
                    has_dst_SIB = TRUE;
                }
            }

            current_byte++;
        }

        // SIB
        u8 SIB = *current_byte;
        if (has_SIB) {
            current_byte++;
        }

        // Displacement bits
        if (has_displacement) {
            // Read displacement bits
            current_byte++;
        }

        u64 src_imm = 0U;
        u64 dst_imm = 0U;
        if (op->src_operand.src == Imm) {
            // Read immediate
            src_imm = get_immediate(&current_byte, op->src_operand.type, effective_operand_size);
        }

        if (op->dst_operand.src == Imm) {
            // Read immediate
            dst_imm = get_immediate(&current_byte, op->dst_operand.type, effective_operand_size);
        }

        u8 inst_size = (u8)(current_byte - inst_start);

        for (u8 index = 0U; index < inst_size; index++) {
            print("%2xb ", inst_start[index]);
        }
        print("\t\t%s", op->name);

        if (op->src_operand.src == Imm) {
            print(" %u", src_imm);
        } else if (op->src_operand.src == ModRM_Reg) {
            u8 Reg = MODRM_REG(ModRM);
            print(" %s", ModRM_registers[Reg][3U]);
        } else if (op->src_operand.src == TYPE_E) {
            u8 Mod = MODRM_MOD(ModRM);
            u8 RM = MODRM_RM(ModRM);
            if (Mod == 0b11) {
                print(" %s", ModRM_registers[RM][3U]);
            }
        }

        if (has_src_SIB) {
            // Read SIB
            i32 scale_factors[4U] = {
                1, 2, 4, 8,
            }
            effective_address = scale_factors[SIB_SCALE(SIB)] * SIB_INDEX(SIB) + SIB_BASE(SIB) + off;
        }

        if (op->dst_operand.src == Imm) {
            print(" %xu", dst_imm);
        }

        if (has_dst_SIB) {
            // Read SIB
            i32 scale_factors[4U] = {
                1, 2, 4, 8,
            }

            u8 index = SIB_INDEX(SIB);
            if (index != 0b100) {
                print(" [%d * %s", scale_factors[SIB_SCALE(SIB)], SIB_index_registers[index]);
            }

            u8 base = SIB_BASE(SIB);
            if (base != 0b101 && MODRM_MOD(ModRM) != 0U) {
                print(" + %s", SIB_base_registers[base]);
            }

            if (has_displacement) {
                print("+ %d]", );
            } else {
                print("]");
            }
        }

        print("\n");

        // u8 low_nibble = current_byte & 0xf;
        // u8 high_nibble = (current_byte >> 4U) & 0xf;
    }
}
