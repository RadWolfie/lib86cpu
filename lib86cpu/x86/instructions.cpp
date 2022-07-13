/*
 * instruction helpers
 *
 * ergo720                Copyright (c) 2022
 */

#include "instructions.h"


template<unsigned reg>
static void
write_seg_reg_helper(cpu_t *cpu, uint16_t sel, uint32_t base, uint32_t limit, uint32_t flags)
{
	switch (reg)
	{
	case CS_idx:
		cpu->cpu_ctx.regs.cs = sel;
		cpu->cpu_ctx.regs.cs_hidden.base = base;
		cpu->cpu_ctx.regs.cs_hidden.limit = limit;
		cpu->cpu_ctx.regs.cs_hidden.flags = flags;
		cpu->cpu_ctx.hflags = (((cpu->cpu_ctx.regs.cs_hidden.flags & SEG_HIDDEN_DB) >> 20) | sel) | (cpu->cpu_ctx.hflags & ~(HFLG_CS32 | HFLG_CPL));
		break;

	case SS_idx:
		cpu->cpu_ctx.regs.ss = sel;
		cpu->cpu_ctx.regs.ss_hidden.base = base;
		cpu->cpu_ctx.regs.ss_hidden.limit = limit;
		cpu->cpu_ctx.regs.ss_hidden.flags = flags;
		cpu->cpu_ctx.hflags = ((cpu->cpu_ctx.regs.ss_hidden.flags & SEG_HIDDEN_DB) >> 19) | (cpu->cpu_ctx.hflags & ~HFLG_SS32);
		break;

	case DS_idx:
		cpu->cpu_ctx.regs.ds = sel;
		cpu->cpu_ctx.regs.ds_hidden.base = base;
		cpu->cpu_ctx.regs.ds_hidden.limit = limit;
		cpu->cpu_ctx.regs.ds_hidden.flags = flags;
		break;

	case ES_idx:
		cpu->cpu_ctx.regs.es = sel;
		cpu->cpu_ctx.regs.es_hidden.base = base;
		cpu->cpu_ctx.regs.es_hidden.limit = limit;
		cpu->cpu_ctx.regs.es_hidden.flags = flags;
		break;

	case FS_idx:
		cpu->cpu_ctx.regs.fs = sel;
		cpu->cpu_ctx.regs.fs_hidden.base = base;
		cpu->cpu_ctx.regs.fs_hidden.limit = limit;
		cpu->cpu_ctx.regs.fs_hidden.flags = flags;
		break;

	case GS_idx:
		cpu->cpu_ctx.regs.gs = sel;
		cpu->cpu_ctx.regs.gs_hidden.base = base;
		cpu->cpu_ctx.regs.gs_hidden.limit = limit;
		cpu->cpu_ctx.regs.gs_hidden.flags = flags;
		break;

	default:
		LIB86CPU_ABORT();
	}
}

template<unsigned reg>
static void
validate_seg_helper(cpu_t *cpu)
{
	uint32_t flags;
	switch (reg)
	{
	case CS_idx:
		flags = cpu->cpu_ctx.regs.cs_hidden.flags;
		break;

	case SS_idx:
		flags = cpu->cpu_ctx.regs.ss_hidden.flags;
		break;

	case DS_idx:
		flags = cpu->cpu_ctx.regs.ds_hidden.flags;
		break;

	case ES_idx:
		flags = cpu->cpu_ctx.regs.es_hidden.flags;
		break;

	case FS_idx:
		flags = cpu->cpu_ctx.regs.fs_hidden.flags;
		break;

	case GS_idx:
		flags = cpu->cpu_ctx.regs.gs_hidden.flags;
		break;

	default:
		LIB86CPU_ABORT();
	}

	uint32_t c = flags & (1 << 10);
	uint32_t d = flags & (1 << 11);
	uint32_t s = flags & (1 << 12);
	uint32_t dpl = (flags & (3 << 13) >> 13);
	uint32_t cpl = cpu->cpu_ctx.hflags & HFLG_CPL;
	if ((cpl > dpl) && (s && ((d == 0) || (c == 0)))) {
		write_seg_reg_helper<reg>(cpu, 0, 0, 0, 0);
	}
}

template<bool is_iret>
uint8_t lret_pe_helper(cpu_ctx_t *cpu_ctx, uint8_t size_mode, uint32_t eip)
{
	cpu_t *cpu = cpu_ctx->cpu;
	uint32_t cpl = cpu->cpu_ctx.hflags & HFLG_CPL;
	uint32_t esp = cpu->cpu_ctx.regs.esp;
	uint32_t ret_eip, temp_eflags, eflags_mask;
	uint16_t cs;

	if constexpr (is_iret) {
		uint32_t eflags = cpu_ctx->regs.eflags;
		if (eflags & VM_MASK) {
			LIB86CPU_ABORT_msg("Virtual 8086 mode is not supported in iret instructions yet");
		}
		if (eflags & NT_MASK) {
			LIB86CPU_ABORT_msg("Task returns are not supported in iret instructions yet");
		}

		ret_eip = stack_pop_helper(cpu, size_mode, esp, eip);
		cs = stack_pop_helper(cpu, size_mode, esp, eip);
		temp_eflags = stack_pop_helper(cpu, size_mode, esp, eip);

		if (temp_eflags & VM_MASK) {
			LIB86CPU_ABORT_msg("Virtual 8086 mode returns are not supported in iret instructions yet");
		}

		if (size_mode == SIZE16) {
			eflags_mask = NT_MASK | DF_MASK | TF_MASK;
		}
		else {
			eflags_mask = ID_MASK | AC_MASK | RF_MASK | NT_MASK | DF_MASK | TF_MASK;
			if (cpl == 0) {
				eflags_mask |= (VIP_MASK | VIF_MASK | VM_MASK | IOPL_MASK);
			}
		}

		if (cpl <= ((cpu->cpu_ctx.regs.eflags & IOPL_MASK) >> 12)) {
			eflags_mask |= IF_MASK;
		}
	}
	else {
		ret_eip = stack_pop_helper(cpu, size_mode, esp, eip);
		cs = stack_pop_helper(cpu, size_mode, esp, eip);
	}
	
	if ((cs >> 2) == 0) { // sel == NULL
		return raise_exp_helper(cpu, 0, EXP_GP, eip);
	}
	
	addr_t desc_addr;
	uint64_t cs_desc;
	if (read_seg_desc_helper(cpu, cs, desc_addr, cs_desc, eip)) {
		return 1;
	}

	uint32_t s = (cs_desc & SEG_DESC_S) >> 44; // !(sys desc)
	uint32_t d = (cs_desc & SEG_DESC_DC) >> 42; // !(data desc)
	if ((s | d) != 3) {
		return raise_exp_helper(cpu, cs & 0xFFFC, EXP_GP, eip);
	}

	uint32_t rpl = cs & 3;
	if (rpl < cpl) { // rpl < cpl
		return raise_exp_helper(cpu, cs & 0xFFFC, EXP_GP, eip);
	}

	uint64_t c = cs_desc & SEG_DESC_C;
	uint32_t dpl = (cs_desc & SEG_DESC_DPL) >> 45;
	if (c && (dpl > rpl)) { // conf && dpl > rpl
		return raise_exp_helper(cpu, cs & 0xFFFC, EXP_GP, eip);
	}

	uint64_t p = cs_desc & SEG_DESC_P;
	if (p == 0) { // segment not present
		return raise_exp_helper(cpu, cs & 0xFFFC, EXP_NP, eip);
	}

	if (rpl > cpl) {
		// less privileged

		uint32_t ret_esp = stack_pop_helper(cpu, size_mode, esp, eip);
		uint16_t ss = stack_pop_helper(cpu, size_mode, esp, eip);
		addr_t ss_desc_addr;
		uint64_t ss_desc;
		if (check_ss_desc_priv_helper(cpu, ss, &cs, ss_desc_addr, ss_desc, eip)) {
			return 1;
		}

		set_access_flg_seg_desc_helper(cpu, ss_desc, ss_desc_addr, eip);
		set_access_flg_seg_desc_helper(cpu, cs_desc, desc_addr, eip);
		write_seg_reg_helper<SS_idx>(cpu, ss, read_seg_desc_base_helper(cpu, ss_desc), read_seg_desc_limit_helper(cpu, ss_desc), read_seg_desc_flags_helper(cpu, ss_desc));
		write_seg_reg_helper<CS_idx>(cpu, cs, read_seg_desc_base_helper(cpu, cs_desc), read_seg_desc_limit_helper(cpu, cs_desc), read_seg_desc_flags_helper(cpu, cs_desc));
		
		uint32_t stack_mask;
		if (cpu->cpu_ctx.regs.ss_hidden.flags & SEG_HIDDEN_DB) {
			stack_mask = 0xFFFFFFFF;
		}
		else {
			stack_mask = 0xFFFF;
		}
		cpu->cpu_ctx.regs.esp = (cpu->cpu_ctx.regs.esp & ~stack_mask) | (ret_esp & stack_mask);
		cpu->cpu_ctx.regs.eip = ret_eip;
		validate_seg_helper<DS_idx>(cpu);
		validate_seg_helper<ES_idx>(cpu);
		validate_seg_helper<FS_idx>(cpu);
		validate_seg_helper<GS_idx>(cpu);
	}
	else {
		// same privilege

		set_access_flg_seg_desc_helper(cpu, cs_desc, desc_addr, eip);
		cpu->cpu_ctx.regs.esp = esp;
		cpu->cpu_ctx.regs.eip = ret_eip;
		write_seg_reg_helper<CS_idx>(cpu, cs, read_seg_desc_base_helper(cpu, cs_desc), read_seg_desc_limit_helper(cpu, cs_desc), read_seg_desc_flags_helper(cpu, cs_desc));
	}

	if constexpr (is_iret) {
		write_eflags_helper(cpu, temp_eflags, eflags_mask);
	}

	return 0;
}

void
iret_real_helper(cpu_ctx_t *cpu_ctx, uint8_t size_mode, uint32_t eip)
{
	cpu_t *cpu = cpu_ctx->cpu;
	uint32_t esp = cpu->cpu_ctx.regs.esp;
	uint32_t eflags_mask;

	uint32_t ret_eip = stack_pop_helper(cpu, size_mode, esp, eip);
	uint16_t cs = stack_pop_helper(cpu, size_mode, esp, eip);
	uint32_t temp_eflags = stack_pop_helper(cpu, size_mode, esp, eip);

	if (size_mode == SIZE16) {
		eflags_mask = NT_MASK | IOPL_MASK | DF_MASK | IF_MASK | TF_MASK;
	}
	else {
		eflags_mask = ID_MASK | AC_MASK | RF_MASK | NT_MASK | IOPL_MASK | DF_MASK | IF_MASK | TF_MASK;
	}

	cpu->cpu_ctx.regs.esp = esp;
	cpu_ctx->regs.eip = ret_eip;
	cpu_ctx->regs.cs = cs;
	cpu_ctx->regs.cs_hidden.base = cs << 4;
	write_eflags_helper(cpu, temp_eflags, eflags_mask);
}

uint8_t
ljmp_pe_helper(cpu_ctx_t *cpu_ctx, uint16_t sel, uint8_t size_mode, uint32_t jmp_eip, uint32_t eip)
{
	cpu_t *cpu = cpu_ctx->cpu;
	uint16_t cpl = cpu->cpu_ctx.hflags & HFLG_CPL;

	if ((sel >> 2) == 0) { // sel == NULL
		return raise_exp_helper(cpu, 0, EXP_GP, eip);
	}

	addr_t desc_addr;
	uint64_t desc;
	if (read_seg_desc_helper(cpu, sel, desc_addr, desc, eip)) {
		return 1;
	}

	if (desc & SEG_DESC_S) {
		// non-system desc

		if ((desc & SEG_DESC_DC) == 0) { // !(data desc)
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
		}

		uint16_t dpl = (desc & SEG_DESC_DPL) >> 45;

		if (desc & SEG_DESC_C) {
			// conforming

			if (dpl > cpl) { // dpl > cpl
				return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
			}
		}
		else {
			// non-conforming

			uint16_t rpl = sel & 3;
			if ((rpl > cpl) || (dpl != cpl)) { // rpl > cpl || dpl != cpl
				return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
			}
		}

		// commmon path for conf/non-conf

		if ((desc & SEG_DESC_P) == 0) { // segment not present
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_NP, eip);
		}

		set_access_flg_seg_desc_helper(cpu, desc, desc_addr, eip);
		write_seg_reg_helper<CS_idx>(cpu, (sel & 0xFFFC) | cpl, read_seg_desc_base_helper(cpu, desc), read_seg_desc_limit_helper(cpu, desc), read_seg_desc_flags_helper(cpu, desc));
		cpu_ctx->regs.eip = size_mode == SIZE16 ? jmp_eip & 0xFFFF : jmp_eip;
	}
	else {
		// system desc

		uint8_t sys_ty = (desc & SEG_DESC_TY) >> 40;
		switch (sys_ty)
		{
		case 1:
		case 5:
		case 9:
			LIB86CPU_ABORT_msg("Task and tss gates in jmp instructions are not supported yet");

		case 4: // call gate, 16 bit
		case 12: // call gate, 32 bit
			break;

		default:
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
		}

		sys_ty >>= 3;
		uint16_t dpl = (desc & SEG_DESC_DPL) >> 45;
		uint16_t rpl = sel & 3;

		if ((dpl < cpl) || (rpl > dpl)) { // dpl < cpl || rpl > dpl
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
		}

		if ((desc & SEG_DESC_P) == 0) { // segment not present
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_NP, eip);
		}

		uint16_t code_sel = (desc & 0xFFFF0000) >> 16;
		if ((code_sel >> 2) == 0) { // code_sel == NULL
			return raise_exp_helper(cpu, 0, EXP_GP, eip);
		}

		if (read_seg_desc_helper(cpu, code_sel, desc_addr, desc, eip)) { // read code desc pointed to by the call gate sel
			return 1;
		}

		dpl = (desc & SEG_DESC_DPL) >> 45;
		if (((((desc & SEG_DESC_S) >> 43) | ((desc & SEG_DESC_DC) >> 43)) != 3) || // !(code desc) || (conf && dpl > cpl) || (non-conf && dpl != cpl)
			((desc & SEG_DESC_C) && (dpl > cpl)) ||
			(((desc & SEG_DESC_C) == 0) && (dpl != cpl))) {
			return raise_exp_helper(cpu, code_sel & 0xFFFC, EXP_GP, eip);
		}

		if ((desc & SEG_DESC_P) == 0) { // segment not present
			return raise_exp_helper(cpu, code_sel & 0xFFFC, EXP_NP, eip);
		}

		set_access_flg_seg_desc_helper(cpu, desc, desc_addr, eip);
		write_seg_reg_helper<CS_idx>(cpu, (sel & 0xFFFC) | cpl, read_seg_desc_base_helper(cpu, desc), read_seg_desc_limit_helper(cpu, desc), read_seg_desc_flags_helper(cpu, desc));

		if (sys_ty == 0) {
			jmp_eip &= 0xFFFF;
		}
		cpu_ctx->regs.eip = jmp_eip;
	}

	return 0;
}

uint8_t
lcall_pe_helper(cpu_ctx_t *cpu_ctx, uint16_t sel, uint32_t call_eip, uint8_t size_mode, uint32_t ret_eip, uint32_t eip)
{
	cpu_t *cpu = cpu_ctx->cpu;
	uint16_t cpl = cpu->cpu_ctx.hflags & HFLG_CPL;

	if ((sel >> 2) == 0) { // sel == NULL
		return raise_exp_helper(cpu, 0, EXP_GP, eip);
	}

	addr_t cs_desc_addr;
	uint64_t cs_desc;
	if (read_seg_desc_helper(cpu, sel, cs_desc_addr, cs_desc, eip)) {
		return 1;
	}

	if (cs_desc & SEG_DESC_S) {
		// non-system desc

		if ((cs_desc & SEG_DESC_DC) == 0) { // !(data desc)
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
		}

		uint16_t dpl = (cs_desc & SEG_DESC_DPL) >> 45;

		if (cs_desc & SEG_DESC_C) {
			// conforming

			if (dpl > cpl) { // dpl > cpl
				return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
			}
		}
		else {
			// non-conforming

			uint16_t rpl = sel & 3;
			if ((rpl > cpl) || (dpl != cpl)) { // rpl > cpl || dpl != cpl
				return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
			}
		}

		// commmon path for conf/non-conf

		if ((cs_desc & SEG_DESC_P) == 0) { // segment not present
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_NP, eip);
		}

		uint32_t esp = cpu->cpu_ctx.regs.esp;
		stack_push_helper(cpu, cpu_ctx->regs.cs, size_mode, esp, eip);
		stack_push_helper(cpu, ret_eip, size_mode, esp, eip);
		set_access_flg_seg_desc_helper(cpu, cs_desc, cs_desc_addr, eip);
		write_seg_reg_helper<CS_idx>(cpu, (sel & 0xFFFC) | cpl, read_seg_desc_base_helper(cpu, cs_desc), read_seg_desc_limit_helper(cpu, cs_desc), read_seg_desc_flags_helper(cpu, cs_desc));
		cpu->cpu_ctx.regs.esp = esp;
		cpu_ctx->regs.eip = call_eip; // call_eip is already appropriately masked by the caller
	}
	else {
		// system desc

		uint8_t sys_ty = (cs_desc & SEG_DESC_TY) >> 40;
		switch (sys_ty)
		{
		case 1:
		case 5:
		case 9:
			LIB86CPU_ABORT_msg("Task and tss gates in call instructions are not supported yet");

		case 4: // call gate, 16 bit
		case 12: // call gate, 32 bit
			break;

		default:
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
		}

		sys_ty >>= 3;
		uint16_t dpl = (cs_desc & SEG_DESC_DPL) >> 45;
		uint16_t rpl = sel & 3;

		if ((dpl < cpl) || (rpl > dpl)) { // dpl < cpl || rpl > dpl
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_GP, eip);
		}

		if ((cs_desc & SEG_DESC_P) == 0) { // segment not present
			return raise_exp_helper(cpu, sel & 0xFFFC, EXP_NP, eip);
		}

		uint32_t num_param = (cs_desc >> 32) & 0x1F;
		uint32_t new_eip = ((cs_desc & 0xFFFF000000000000) >> 32) | (cs_desc & 0xFFFF);
		uint16_t code_sel = (cs_desc & 0xFFFF0000) >> 16;
		if ((code_sel >> 2) == 0) { // code_sel == NULL
			return raise_exp_helper(cpu, 0, EXP_GP, eip);
		}

		addr_t code_desc_addr;
		uint64_t code_desc;
		if (read_seg_desc_helper(cpu, code_sel, code_desc_addr, code_desc, eip)) { // read code desc pointed to by the call gate sel
			return 1;
		}

		dpl = (code_desc & SEG_DESC_DPL) >> 45;
		if (((((code_desc & SEG_DESC_S) >> 43) | ((code_desc & SEG_DESC_DC) >> 43)) != 3) || // !(code desc) || dpl > cpl
			(dpl > cpl)) {
			return raise_exp_helper(cpu, code_sel & 0xFFFC, EXP_GP, eip);
		}

		if ((code_desc & SEG_DESC_P) == 0) { // segment not present
			return raise_exp_helper(cpu, code_sel & 0xFFFC, EXP_NP, eip);
		}

		uint32_t eip_mask, esp;
		if (((code_desc & SEG_DESC_C) == 0) && (dpl < cpl)) {
			// more privileged

			uint16_t ss;
			if (read_stack_ptr_from_tss_helper(cpu, dpl, esp, ss, eip)) {
				return 1;
			}

			if ((ss >> 2) == 0) { // sel == NULL
				return raise_exp_helper(cpu, ss & 0xFFFC, EXP_TS, eip);
			}

			addr_t ss_desc_addr;
			uint64_t ss_desc;
			if (read_seg_desc_helper(cpu, ss, ss_desc_addr, ss_desc, eip)) { // read data (stack) desc pointed to by ss
				return 1;
			}

			uint16_t s = (ss_desc & SEG_DESC_S) >> 44; // !(sys desc)
			uint16_t d = (ss_desc & SEG_DESC_DC) >> 42; // data desc
			uint16_t w = (ss_desc & SEG_DESC_W) >> 39;	// writable
			uint16_t dpl_ss = (ss_desc & SEG_DESC_DPL) >> 42; // dpl(ss) == dpl(code)
			uint16_t rpl_ss = (ss & 3) << 5; // rpl(ss) == dpl(code)
			if (((((s | d) | w) | dpl_ss) | rpl_ss) ^ ((5 | (dpl << 3)) | (dpl << 5))) {
				return raise_exp_helper(cpu, ss & 0xFFFC, EXP_TS, eip);
			}

			if ((ss_desc & SEG_DESC_P) == 0) { // segment not present
				return raise_exp_helper(cpu, ss & 0xFFFC, EXP_SS, eip);
			}

			uint32_t stack_mask = ss_desc & SEG_DESC_DB ? 0xFFFFFFFF : 0xFFFF;
			uint32_t stack_base = read_seg_desc_base_helper(cpu, ss_desc);
			int32_t i = num_param - 1;
			if (sys_ty) { // 32 bit push
				eip_mask = 0xFFFFFFFF;
				esp -= 4;
				mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.ss, eip, 2); // push ss
				esp -= 4;
				mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.esp, eip, 2); // push esp
				while (i >= 0) {
					uint32_t param32 = mem_read<uint32_t>(cpu, cpu_ctx->regs.ss_hidden.base + ((cpu_ctx->regs.esp + i * 4) & stack_mask), eip, 2); // read param from src stack
					esp -= 4;
					mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), param32, eip, 2); // push param to dst stack
					--i;
				}
				esp -= 4;
				mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.cs, eip, 2); // push cs
				esp -= 4;
				mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), ret_eip, eip, 2); // push eip
			}
			else { // 16 bit push
				eip_mask = 0xFFFF;
				esp -= 2;
				mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.ss, eip, 2); // push ss
				esp -= 2;
				mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.esp, eip, 2); // push sp
				while (i >= 0) {
					uint16_t param16 = mem_read<uint16_t>(cpu, cpu_ctx->regs.ss_hidden.base + ((cpu_ctx->regs.esp + i * 2) & stack_mask), eip, 2); // read param from src stack
					esp -= 2;
					mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), param16, eip, 2); // push param to dst stack
					--i;
				}
				esp -= 2;
				mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.cs, eip, 2); // push cs
				esp -= 2;
				mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), ret_eip, eip, 2); // push ip
			}

			set_access_flg_seg_desc_helper(cpu, ss_desc, ss_desc_addr, eip);
			write_seg_reg_helper<SS_idx>(cpu, (ss & 0xFFFC) | dpl, read_seg_desc_base_helper(cpu, ss_desc), read_seg_desc_limit_helper(cpu, ss_desc), read_seg_desc_flags_helper(cpu, ss_desc));
		}
		else {
			// same privilege

			esp = cpu_ctx->regs.esp;
			uint32_t stack_base = cpu_ctx->regs.ss_hidden.base;
			uint32_t stack_mask = cpu->cpu_ctx.hflags & HFLG_SS32 ? 0xFFFFFFFF : 0xFFFF;
			if (sys_ty) { // 32 bit push
				eip_mask = 0xFFFFFFFF;
				esp -= 4;
				mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.cs, eip, 0); // push cs
				esp -= 4;
				mem_write<uint32_t>(cpu, stack_base + (esp & stack_mask), ret_eip, eip, 0); // push eip
			}
			else { // 16 bit push
				eip_mask = 0xFFFF;
				esp -= 2;
				mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), cpu_ctx->regs.cs, eip, 0); // push cs
				esp -= 2;
				mem_write<uint16_t>(cpu, stack_base + (esp & stack_mask), ret_eip, eip, 0); // push ip
			}
		}

		set_access_flg_seg_desc_helper(cpu, code_desc, code_desc_addr, eip);
		write_seg_reg_helper<CS_idx>(cpu, (code_sel & 0xFFFC) | dpl, read_seg_desc_base_helper(cpu, code_desc), read_seg_desc_limit_helper(cpu, code_desc), read_seg_desc_flags_helper(cpu, code_desc));
		cpu->cpu_ctx.regs.esp = esp;
		cpu_ctx->regs.eip = (new_eip & ~eip_mask) | (new_eip & eip_mask);
	}

	return 0;
}

template uint8_t lret_pe_helper<true>(cpu_ctx_t *cpu_ctx, uint8_t size_mode, uint32_t eip);
template uint8_t lret_pe_helper<false>(cpu_ctx_t *cpu_ctx, uint8_t size_mode, uint32_t eip);
