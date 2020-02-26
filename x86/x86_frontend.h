/*
 * x86 llvm frontend exports to translator
 *
 * ergo720                Copyright (c) 2019
 */

#pragma once


FunctionType * create_tc_fntype(cpu_t *cpu, translated_code_t *tc);
Function *create_tc_prologue(cpu_t *cpu, translated_code_t *tc, FunctionType *fntype);
void create_tc_epilogue(cpu_t *cpu, translated_code_t *tc, FunctionType *fntype, disas_ctx_t *disas_ctx);
translated_code_t *tc_run_code(cpu_ctx_t *cpu_ctx, translated_code_t *tc);
void tc_link_direct(translated_code_t *prev_tc, translated_code_t *ptr_tc, addr_t pc);
translated_code_t *tc_cache_search(cpu_t *cpu, addr_t pc);
void tc_cache_insert(cpu_t *cpu, addr_t pc, std::unique_ptr<translated_code_t> &&tc);
void tc_cache_clear(cpu_t *cpu);
void optimize(translated_code_t *tc, Function *func);
Value *get_struct_member_pointer(Value *gep_start, const unsigned gep_index, translated_code_t *tc, BasicBlock *bb);
Value *get_r8h_pointer(Value *gep_start, translated_code_t *tc, BasicBlock *bb);
void get_ext_fn(cpu_t *cpu, translated_code_t *tc, Function *func);
Value *get_operand(cpu_t *cpu, x86_instr *instr, translated_code_t *tc, BasicBlock *bb, const unsigned opnum);
Value *mem_read_no_cpl_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *addr, Value *ptr_eip, const unsigned idx);
void mem_write_no_cpl_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *addr, Value *value, Value *ptr_eip, const unsigned idx);
void check_io_priv_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *port, Value *mask, Value *ptr_eip);
void stack_push_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, std::vector<Value *> &vec, Value *ptr_eip, uint32_t size_mode);
std::vector<Value *> stack_pop_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *ptr_eip, uint32_t size_mode, const unsigned num, const unsigned pop_at = 0);
Value *calc_next_pc_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *ptr_eip, size_t instr_size);
BasicBlock *raise_exception_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb2, uint8_t expno, Value *ptr_eip);
void ljmp_pe_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *sel, Value *eip, Value *ptr_eip);
Value *iret_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *ptr_eip, uint8_t size_mode);
std::vector<Value *> check_ss_desc_priv_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *sel, Value *ptr_eip, Value *cs = nullptr);
std::vector<Value *> check_seg_desc_priv_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *sel, Value *ptr_eip);
void set_access_flg_seg_desc_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *desc, Value *desc_addr, Value *ptr_eip);
std::vector<Value *> read_seg_desc_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *sel, Value *ptr_eip);
Value *read_seg_desc_base_emit(translated_code_t *tc, BasicBlock *bb, Value *desc);
Value *read_seg_desc_limit_emit(translated_code_t *tc, BasicBlock *&bb, Value *desc);
Value *read_seg_desc_flags_emit(translated_code_t *tc, BasicBlock *bb, Value *desc);
std::vector<Value *> read_tss_desc_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *&bb, Value *sel, Value *ptr_eip);
void write_seg_reg_emit(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, const unsigned reg, Value *sel, Value *base, Value *limit, Value *flags);
Value *get_immediate_op(translated_code_t *tc, x86_instr *instr, uint8_t idx, uint8_t size_mode);
Value *get_register_op(cpu_t *cpu, translated_code_t *tc, x86_instr *instr, BasicBlock *bb, uint8_t idx);
void set_flags_sum(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *sum, Value *a, Value *b, uint8_t size_mode);
void set_flags_sub(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *sub, Value *a, Value *b, uint8_t size_mode);
void set_flags(cpu_t *cpu, translated_code_t *tc, BasicBlock *bb, Value *res, Value *aux, uint8_t size_mode);


#define CTX() (*tc->ctx)
#define BB() BasicBlock::Create(CTX(), "", func, 0)
#define getIntegerType(x) (IntegerType::get(CTX(), x))
#define getPointerType(x) (PointerType::getUnqual(x))
#define getIntegerPointerType() (cpu->dl->getIntPtrType(CTX()))
#define getVoidType() (Type::getVoidTy(CTX()))
#define getArrayIntegerType(x, n) (ArrayType::get(getIntegerType(x), n))

#define MEM_LD8_idx  0
#define MEM_LD16_idx 1
#define MEM_LD32_idx 2
#define MEM_LD64_idx 3
#define IO_LD8_idx   4
#define IO_LD16_idx  5
#define IO_LD32_idx  6
#define MEM_ST8_idx  0
#define MEM_ST16_idx 1
#define MEM_ST32_idx 2
#define MEM_ST64_idx 3
#define IO_ST8_idx   4
#define IO_ST16_idx  5
#define IO_ST32_idx  6

#define GET_IMM() get_immediate_op(tc, &instr, OPNUM_SRC, size_mode)
#define GET_IMM8() get_immediate_op(tc, &instr, OPNUM_SRC, SIZE8)
#define GET_REG(idx) get_register_op(cpu, tc, &instr, bb, idx)
#define GET_OP(op) get_operand(cpu, &instr, tc, bb, op)
#define GET_RM(idx, r, m) 	rm = GET_OP(idx); \
switch (instr.operand[idx].type) \
{ \
case OPTYPE_REG: \
	r \
	break; \
\
case OPTYPE_MEM: \
case OPTYPE_MEM_DISP: \
case OPTYPE_SIB_MEM: \
case OPTYPE_SIB_DISP: \
	m \
	break; \
\
default: \
	LIB86CPU_ABORT_msg("Invalid operand type used in GET_RM macro!\n"); \
}

#define INTPTR(v) ConstantInt::get(getIntegerPointerType(), reinterpret_cast<uintptr_t>(v))
#define CONSTs(s, v) ConstantInt::get(getIntegerType(s), v)
#define CONSTptr(s, v) ConstantExpr::getIntToPtr(INTPTR(v), getPointerType(getIntegerType(s)))
#define CONST1(v) CONSTs(1, v)
#define CONST8(v) CONSTs(8, v)
#define CONST16(v) CONSTs(16, v)
#define CONST32(v) CONSTs(32, v)
#define CONST64(v) CONSTs(64, v)

#define ALLOCs(s) new AllocaInst(getIntegerType(s), 0, "", bb)
#define ALLOC8() ALLOCs(8)
#define ALLOC16() ALLOCs(16)
#define ALLOC32() ALLOCs(32)

#define ST(ptr, v) new StoreInst(v, ptr, bb)
#define LD(ptr) new LoadInst(ptr, "", false, bb)

#define UNREACH() new UnreachableInst(CTX(), bb)
#define INTRINSIC(id) CallInst::Create(Intrinsic::getDeclaration(tc->mod, Intrinsic::id), "", bb)

#define ZEXT(s, v) new ZExtInst(v, getIntegerType(s), "", bb)
#define ZEXT8(v) ZEXT(8, v)
#define ZEXT16(v) ZEXT(16, v)
#define ZEXT32(v) ZEXT(32, v)
#define ZEXT64(v) ZEXT(64, v)

#define SEXT(s, v) new SExtInst(v, getIntegerType(s), "", bb)
#define SEXT8(v) SEXT(8, v)
#define SEXT16(v) SEXT(16, v)
#define SEXT32(v) SEXT(32, v)
#define SEXT64(v) SEXT(64, v)

#define IBITCASTs(s, v) new BitCastInst(v, getPointerType(getIntegerType(s)), "", bb)
#define IBITCAST8(v) IBITCASTs(8, v)
#define IBITCAST16(v) IBITCASTs(16, v)
#define IBITCAST32(v) IBITCASTs(32, v)

#define TRUNCs(s,v) new TruncInst(v, getIntegerType(s), "", bb)
#define TRUNC8(v) TRUNCs(8, v)
#define TRUNC16(v) TRUNCs(16, v)
#define TRUNC32(v) TRUNCs(32, v)

#define ADD(a, b) BinaryOperator::Create(Instruction::Add, a, b, "", bb)
#define SUB(a, b) BinaryOperator::Create(Instruction::Sub, a, b, "", bb)
#define MUL(a, b) BinaryOperator::Create(Instruction::Mul, a, b, "", bb)
#define UDIV(a, b) BinaryOperator::Create(Instruction::UDiv, a, b, "", bb)
#define UREM(a, b) BinaryOperator::Create(Instruction::URem, a, b, "", bb)
#define AND(a, b) BinaryOperator::Create(Instruction::And, a, b, "", bb)
#define XOR(a, b) BinaryOperator::Create(Instruction::Xor, a, b, "", bb)
#define OR(a, b) BinaryOperator::Create(Instruction::Or, a, b, "", bb)
#define NOT(a) BinaryOperator::CreateNot(a, "", bb)
#define SHR(a, sh) BinaryOperator::Create(Instruction::LShr, a, sh, "", bb)
#define SHL(a, sh) BinaryOperator::Create(Instruction::Shl, a, sh, "", bb)
#define BR_COND(t, f, val, bb) BranchInst::Create(t, f, val, bb)
#define BR_UNCOND(t, bb) BranchInst::Create(t, bb)
#define ICMP_EQ(a, b) new ICmpInst(*bb, ICmpInst::ICMP_EQ, a, b, "")
#define ICMP_NE(a, b) new ICmpInst(*bb, ICmpInst::ICMP_NE, a, b, "")
#define ICMP_UGT(a, b) new ICmpInst(*bb, ICmpInst::ICMP_UGT, a, b, "")
#define ICMP_ULT(a, b) new ICmpInst(*bb, ICmpInst::ICMP_ULT, a, b, "")
#define NOT_ZERO(s, v) AND(SHR(OR(v, SUB(CONSTs(s, 0), v)), CONSTs(s, s-1)), CONSTs(s, 1))

#define GEP(ptr, idx)  get_struct_member_pointer(ptr, idx, tc, bb)
#define GEP_R32(idx)   GEP(cpu->ptr_regs, idx)
#define GEP_R16(idx)   IBITCAST16(GEP(cpu->ptr_regs, idx))
#define GEP_R8L(idx)   IBITCAST8(GEP(cpu->ptr_regs, idx))
#define GEP_R8H(idx)   get_r8h_pointer(IBITCAST8(GEP(cpu->ptr_regs, idx)), tc, bb)
#define GEP_SEL(idx)   GEP(GEP(cpu->ptr_regs, idx), SEG_SEL_idx)
#define GEP_EAX()      GEP_R32(EAX_idx)
#define GEP_ECX()      GEP_R32(ECX_idx)
#define GEP_EDX()      GEP_R32(EDX_idx)
#define GEP_EBX()      GEP_R32(EBX_idx)
#define GEP_ESP()      GEP_R32(ESP_idx)
#define GEP_EBP()      GEP_R32(EBP_idx)
#define GEP_ESI()      GEP_R32(ESI_idx)
#define GEP_EDI()      GEP_R32(EDI_idx)
#define GEP_ES()       GEP_SEL(ES_idx)
#define GEP_CS()       GEP_SEL(CS_idx)
#define GEP_SS()       GEP_SEL(SS_idx)
#define GEP_DS()       GEP_SEL(DS_idx)
#define GEP_FS()       GEP_SEL(FS_idx)
#define GEP_GS()       GEP_SEL(GS_idx)
#define GEP_CR0()      GEP_R32(CR0_idx)
#define GEP_CR1()      GEP_R32(CR1_idx)
#define GEP_CR2()      GEP_R32(CR2_idx)
#define GEP_CR3()      GEP_R32(CR3_idx)
#define GEP_CR4()      GEP_R32(CR4_idx)
#define GEP_DR0()      GEP_R32(DR0_idx)
#define GEP_DR1()      GEP_R32(DR1_idx)
#define GEP_DR2()      GEP_R32(DR2_idx)
#define GEP_DR3()      GEP_R32(DR3_idx)
#define GEP_DR4()      GEP_R32(DR4_idx)
#define GEP_DR5()      GEP_R32(DR5_idx)
#define GEP_DR6()      GEP_R32(DR6_idx)
#define GEP_DR7()      GEP_R32(DR7_idx)
#define GEP_EFLAGS()   GEP_R32(EFLAGS_idx)
#define GEP_EIP()      GEP_R32(EIP_idx)
#define GEP_PARITY()   GEP(cpu->ptr_eflags, 2)

#define ST_R32(val, idx) ST(GEP(cpu->ptr_regs, idx), val)
#define ST_R16(val, idx) ST(GEP_R16(idx), val)
#define ST_R8L(val, idx) ST(GEP_R8L(idx), val)
#define ST_R8H(val, idx) ST(GEP_R8H(idx), val)
#define ST_REG_val(val, reg) ST(reg, val)
#define ST_SEG(val, seg) ST(GEP_SEL(seg), val)
#define ST_SEG_HIDDEN(val, seg, idx) ST(GEP(GEP(GEP(cpu->ptr_regs, seg), SEG_HIDDEN_idx), idx), val)
#define LD_R32(idx) LD(GEP(cpu->ptr_regs, idx))
#define LD_R16(idx) LD(GEP_R16(idx))
#define LD_R8L(idx) LD(GEP_R8L(idx))
#define LD_R8H(idx) LD(GEP_R8H(idx))
#define LD_REG_val(reg) LD(reg)
#define LD_SEG(seg) LD(GEP_SEL(seg))
#define LD_SEG_HIDDEN(seg, idx) LD(GEP(GEP(GEP(cpu->ptr_regs, seg), SEG_HIDDEN_idx), idx))

#define LD_MEM(idx, addr) CallInst::Create(cpu->ptr_mem_ldfn[idx], std::vector<Value *> { cpu->ptr_cpu_ctx, addr, ptr_eip }, "", bb)
#define ST_MEM(idx, addr, val) CallInst::Create(cpu->ptr_mem_stfn[idx], std::vector<Value *> { cpu->ptr_cpu_ctx, addr, val, ptr_eip }, "", bb)
#define LD_MEM_PRIV(idx, addr) mem_read_no_cpl_emit(cpu, tc, bb, addr, ptr_eip, idx)
#define ST_MEM_PRIV(idx, addr, val) mem_write_no_cpl_emit(cpu, tc, bb, addr, val, ptr_eip, idx)
#define MEM_PUSH(vec) stack_push_emit(cpu, tc, bb, vec, ptr_eip, size_mode)
#define MEM_POP(n) stack_pop_emit(cpu, tc, bb, ptr_eip, size_mode, n)
#define MEM_POP_AT(n, at) stack_pop_emit(cpu, tc, bb, ptr_eip, size_mode, n, at)

#define LD_PARITY(idx) LD(GetElementPtrInst::CreateInBounds(GEP_PARITY(), std::vector<Value *> { CONST8(0), idx }, "", bb))
#define RAISE(expno) CallInst::Create(cpu->exp_fn, std::vector<Value *> { cpu->ptr_cpu_ctx, CONST8(expno), ptr_eip }, "", bb)
#define SET_FLG_SUM(sum, a, b) set_flags_sum(cpu, tc, bb, sum, a, b, size_mode)
#define SET_FLG_SUB(sub, a, b) set_flags_sub(cpu, tc, bb, sub, a, b, size_mode)
#define SET_FLG(res, aux) set_flags(cpu, tc, bb, res, aux, size_mode)

#define REP_start() disas_ctx->bb = BB(); \
Value *ecx, *zero; \
if (addr_mode == ADDR16) { \
	ecx = LD_R16(ECX_idx); \
	zero = CONST16(0); \
} \
else { \
	ecx = LD_R32(ECX_idx); \
	zero = CONST32(0); \
} \
BR_COND(disas_ctx->bb, bb_next, ICMP_NE(ecx, zero), bb); \
bb = disas_ctx->bb

#define REP() Value *ecx, *zero, *one; \
if (addr_mode == ADDR16) { \
	ecx = LD_R16(ECX_idx); \
	zero = CONST16(0); \
	one = CONST16(1); \
	ecx = SUB(ecx, one); \
	ST_R16(ecx, ECX_idx); \
} \
else { \
	ecx = LD_R32(ECX_idx); \
	zero = CONST32(0); \
	one = CONST32(1); \
	ecx = SUB(ecx, one); \
	ST_R32(ecx, ECX_idx); \
} \
BR_COND(disas_ctx->bb, bb_next, ICMP_NE(ecx, zero), bb)

#define REPNZ() Value *ecx, *zero, *one; \
if (addr_mode == ADDR16) { \
	ecx = LD_R16(ECX_idx); \
	zero = CONST16(0); \
	one = CONST16(1); \
	ecx = SUB(ecx, one); \
	ST_R16(ecx, ECX_idx); \
} \
else { \
	ecx = LD_R32(ECX_idx); \
	zero = CONST32(0); \
	one = CONST32(1); \
	ecx = SUB(ecx, one); \
	ST_R32(ecx, ECX_idx); \
} \
BR_COND(disas_ctx->bb, bb_next, AND(ICMP_NE(ecx, zero), ICMP_NE(LD_ZF(), CONST32(0))), bb)

#define REPZ() Value *ecx, *zero, *one; \
if (addr_mode == ADDR16) { \
	ecx = LD_R16(ECX_idx); \
	zero = CONST16(0); \
	one = CONST16(1); \
	ecx = SUB(ecx, one); \
	ST_R16(ecx, ECX_idx); \
} \
else { \
	ecx = LD_R32(ECX_idx); \
	zero = CONST32(0); \
	one = CONST32(1); \
	ecx = SUB(ecx, one); \
	ST_R32(ecx, ECX_idx); \
} \
BR_COND(disas_ctx->bb, bb_next, AND(ICMP_NE(ecx, zero), ICMP_EQ(LD_ZF(), CONST32(0))), bb)

// the lazy eflags idea comes from reading these two papers:
// How Bochs Works Under the Hood (2nd edition) http://bochs.sourceforge.net/How%20the%20Bochs%20works%20under%20the%20hood%202nd%20edition.pdf
// A Proposal for Hardware-Assisted Arithmetic Overflow Detection for Array and Bitfield Operations http://www.emulators.com/docs/LazyOverflowDetect_Final.pdf
#define SUM_COUT_VEC(a, b, s) OR(AND(a, b), AND(OR(a, b), NOT(s)))
#define SUB_COUT_VEC(a, b, d) OR(AND(NOT(a), b), AND(NOT(XOR(a, b)), d))
#define MASK_FLG8(a) AND(OR(SHL(a, CONST32(24)), a), CONST32(0xC0000008))
#define MASK_FLG16(a) AND(OR(SHL(a, CONST32(16)), a), CONST32(0xC0000008))
#define MASK_FLG32(a) AND(a, CONST32(0xC0000008))
#define GEN_SUM_VEC8(a, b, r) MASK_FLG8(ZEXT32(SUM_COUT_VEC(a, b, r)))
#define GEN_SUM_VEC16(a, b, r) MASK_FLG16(ZEXT32(SUM_COUT_VEC(a, b, r)))
#define GEN_SUM_VEC32(a, b, r) MASK_FLG32(SUM_COUT_VEC(a, b, r))
#define GEN_SUB_VEC8(a, b, r) MASK_FLG8(ZEXT32(SUB_COUT_VEC(a, b, r)))
#define GEN_SUB_VEC16(a, b, r) MASK_FLG16(ZEXT32(SUB_COUT_VEC(a, b, r)))
#define GEN_SUB_VEC32(a, b, r) MASK_FLG32(SUB_COUT_VEC(a, b, r))
#define ST_FLG_SUM_AUX8(a, b, r) ST(GEP(cpu->ptr_eflags, 1), GEN_SUM_VEC8(a, b, r))
#define ST_FLG_SUM_AUX16(a, b, r) ST(GEP(cpu->ptr_eflags, 1), GEN_SUM_VEC16(a, b, r))
#define ST_FLG_SUM_AUX32(a, b, r) ST(GEP(cpu->ptr_eflags, 1), GEN_SUM_VEC32(a, b, r))
#define ST_FLG_SUB_AUX8(a, b, r) ST(GEP(cpu->ptr_eflags, 1), GEN_SUB_VEC8(a, b, r))
#define ST_FLG_SUB_AUX16(a, b, r) ST(GEP(cpu->ptr_eflags, 1), GEN_SUB_VEC16(a, b, r))
#define ST_FLG_SUB_AUX32(a, b, r) ST(GEP(cpu->ptr_eflags, 1), GEN_SUB_VEC32(a, b, r))
#define ST_FLG_AUX(val) ST(GEP(cpu->ptr_eflags, 1), val)
#define ST_FLG_RES_ext(val) ST(GEP(cpu->ptr_eflags, 0), SEXT32(val))
#define ST_FLG_RES(val) ST(GEP(cpu->ptr_eflags, 0), val)
#define LD_FLG_RES() LD(GEP(cpu->ptr_eflags, 0))
#define LD_FLG_AUX() LD(GEP(cpu->ptr_eflags, 1))
#define LD_CF() AND(LD_FLG_AUX(), CONST32(0x80000000))
#define LD_OF() AND(XOR(LD_FLG_AUX(), SHL(LD_FLG_AUX(), CONST32(1))), CONST32(0x80000000))
#define LD_ZF() LD_FLG_RES()
#define LD_SF() XOR(SHR(LD_FLG_RES(), CONST32(31)), AND(LD_FLG_AUX(), CONST32(1)))
#define LD_PF() LD_PARITY(TRUNC8(XOR(LD_FLG_RES(), SHR(LD_FLG_AUX(), CONST32(8)))))
#define LD_AF() AND(LD_FLG_AUX(), CONST32(8))
