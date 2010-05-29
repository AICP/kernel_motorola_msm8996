/*
 * Copyright 2010 Tilera Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/backtrace.h>

#include <arch/chip.h>

#if TILE_CHIP < 10


#include <asm/opcode-tile.h>


#define TREG_SP 54
#define TREG_LR 55


/** A decoded bundle used for backtracer analysis. */
typedef struct {
	tile_bundle_bits bits;
	int num_insns;
	struct tile_decoded_instruction
	insns[TILE_MAX_INSTRUCTIONS_PER_BUNDLE];
} BacktraceBundle;


/* This implementation only makes sense for native tools. */
/** Default function to read memory. */
static bool
bt_read_memory(void *result, VirtualAddress addr, size_t size, void *extra)
{
	/* FIXME: this should do some horrible signal stuff to catch
	 * SEGV cleanly and fail.
	 *
	 * Or else the caller should do the setjmp for efficiency.
	 */

	memcpy(result, (const void *)addr, size);
	return true;
}


/** Locates an instruction inside the given bundle that
 * has the specified mnemonic, and whose first 'num_operands_to_match'
 * operands exactly match those in 'operand_values'.
 */
static const struct tile_decoded_instruction*
find_matching_insn(const BacktraceBundle *bundle,
		   tile_mnemonic mnemonic,
		   const int *operand_values,
		   int num_operands_to_match)
{
	int i, j;
	bool match;

	for (i = 0; i < bundle->num_insns; i++) {
		const struct tile_decoded_instruction *insn =
			&bundle->insns[i];

		if (insn->opcode->mnemonic != mnemonic)
			continue;

		match = true;
		for (j = 0; j < num_operands_to_match; j++) {
			if (operand_values[j] != insn->operand_values[j]) {
				match = false;
				break;
			}
		}

		if (match)
			return insn;
	}

	return NULL;
}

/** Does this bundle contain an 'iret' instruction? */
static inline bool
bt_has_iret(const BacktraceBundle *bundle)
{
	return find_matching_insn(bundle, TILE_OPC_IRET, NULL, 0) != NULL;
}

/** Does this bundle contain an 'addi sp, sp, OFFSET' or
 * 'addli sp, sp, OFFSET' instruction, and if so, what is OFFSET?
 */
static bool
bt_has_addi_sp(const BacktraceBundle *bundle, int *adjust)
{
	static const int vals[2] = { TREG_SP, TREG_SP };

	const struct tile_decoded_instruction *insn =
		find_matching_insn(bundle, TILE_OPC_ADDI, vals, 2);
	if (insn == NULL)
		insn = find_matching_insn(bundle, TILE_OPC_ADDLI, vals, 2);
	if (insn == NULL)
		return false;

	*adjust = insn->operand_values[2];
	return true;
}

/** Does this bundle contain any 'info OP' or 'infol OP'
 * instruction, and if so, what are their OP?  Note that OP is interpreted
 * as an unsigned value by this code since that's what the caller wants.
 * Returns the number of info ops found.
 */
static int
bt_get_info_ops(const BacktraceBundle *bundle,
		int operands[MAX_INFO_OPS_PER_BUNDLE])
{
	int num_ops = 0;
	int i;

	for (i = 0; i < bundle->num_insns; i++) {
		const struct tile_decoded_instruction *insn =
			&bundle->insns[i];

		if (insn->opcode->mnemonic == TILE_OPC_INFO ||
		    insn->opcode->mnemonic == TILE_OPC_INFOL) {
			operands[num_ops++] = insn->operand_values[0];
		}
	}

	return num_ops;
}

/** Does this bundle contain a jrp instruction, and if so, to which
 * register is it jumping?
 */
static bool
bt_has_jrp(const BacktraceBundle *bundle, int *target_reg)
{
	const struct tile_decoded_instruction *insn =
		find_matching_insn(bundle, TILE_OPC_JRP, NULL, 0);
	if (insn == NULL)
		return false;

	*target_reg = insn->operand_values[0];
	return true;
}

/** Does this bundle modify the specified register in any way? */
static bool
bt_modifies_reg(const BacktraceBundle *bundle, int reg)
{
	int i, j;
	for (i = 0; i < bundle->num_insns; i++) {
		const struct tile_decoded_instruction *insn =
			&bundle->insns[i];

		if (insn->opcode->implicitly_written_register == reg)
			return true;

		for (j = 0; j < insn->opcode->num_operands; j++)
			if (insn->operands[j]->is_dest_reg &&
			    insn->operand_values[j] == reg)
				return true;
	}

	return false;
}

/** Does this bundle modify sp? */
static inline bool
bt_modifies_sp(const BacktraceBundle *bundle)
{
	return bt_modifies_reg(bundle, TREG_SP);
}

/** Does this bundle modify lr? */
static inline bool
bt_modifies_lr(const BacktraceBundle *bundle)
{
	return bt_modifies_reg(bundle, TREG_LR);
}

/** Does this bundle contain the instruction 'move fp, sp'? */
static inline bool
bt_has_move_r52_sp(const BacktraceBundle *bundle)
{
	static const int vals[2] = { 52, TREG_SP };
	return find_matching_insn(bundle, TILE_OPC_MOVE, vals, 2) != NULL;
}

/** Does this bundle contain the instruction 'sw sp, lr'? */
static inline bool
bt_has_sw_sp_lr(const BacktraceBundle *bundle)
{
	static const int vals[2] = { TREG_SP, TREG_LR };
	return find_matching_insn(bundle, TILE_OPC_SW, vals, 2) != NULL;
}

/** Locates the caller's PC and SP for a program starting at the
 * given address.
 */
static void
find_caller_pc_and_caller_sp(CallerLocation *location,
			     const VirtualAddress start_pc,
			     BacktraceMemoryReader read_memory_func,
			     void *read_memory_func_extra)
{
	/* Have we explicitly decided what the sp is,
	 * rather than just the default?
	 */
	bool sp_determined = false;

	/* Has any bundle seen so far modified lr? */
	bool lr_modified = false;

	/* Have we seen a move from sp to fp? */
	bool sp_moved_to_r52 = false;

	/* Have we seen a terminating bundle? */
	bool seen_terminating_bundle = false;

	/* Cut down on round-trip reading overhead by reading several
	 * bundles at a time.
	 */
	tile_bundle_bits prefetched_bundles[32];
	int num_bundles_prefetched = 0;
	int next_bundle = 0;
	VirtualAddress pc;

	/* Default to assuming that the caller's sp is the current sp.
	 * This is necessary to handle the case where we start backtracing
	 * right at the end of the epilog.
	 */
	location->sp_location = SP_LOC_OFFSET;
	location->sp_offset = 0;

	/* Default to having no idea where the caller PC is. */
	location->pc_location = PC_LOC_UNKNOWN;

	/* Don't even try if the PC is not aligned. */
	if (start_pc % TILE_BUNDLE_ALIGNMENT_IN_BYTES != 0)
		return;

	for (pc = start_pc;; pc += sizeof(tile_bundle_bits)) {

		BacktraceBundle bundle;
		int num_info_ops, info_operands[MAX_INFO_OPS_PER_BUNDLE];
		int one_ago, jrp_reg;
		bool has_jrp;

		if (next_bundle >= num_bundles_prefetched) {
			/* Prefetch some bytes, but don't cross a page
			 * boundary since that might cause a read failure we
			 * don't care about if we only need the first few
			 * bytes. Note: we don't care what the actual page
			 * size is; using the minimum possible page size will
			 * prevent any problems.
			 */
			unsigned int bytes_to_prefetch = 4096 - (pc & 4095);
			if (bytes_to_prefetch > sizeof prefetched_bundles)
				bytes_to_prefetch = sizeof prefetched_bundles;

			if (!read_memory_func(prefetched_bundles, pc,
					      bytes_to_prefetch,
					      read_memory_func_extra)) {
				if (pc == start_pc) {
					/* The program probably called a bad
					 * address, such as a NULL pointer.
					 * So treat this as if we are at the
					 * start of the function prolog so the
					 * backtrace will show how we got here.
					 */
					location->pc_location = PC_LOC_IN_LR;
					return;
				}

				/* Unreadable address. Give up. */
				break;
			}

			next_bundle = 0;
			num_bundles_prefetched =
				bytes_to_prefetch / sizeof(tile_bundle_bits);
		}

		/* Decode the next bundle. */
		bundle.bits = prefetched_bundles[next_bundle++];
		bundle.num_insns =
			parse_insn_tile(bundle.bits, pc, bundle.insns);
		num_info_ops = bt_get_info_ops(&bundle, info_operands);

		/* First look at any one_ago info ops if they are interesting,
		 * since they should shadow any non-one-ago info ops.
		 */
		for (one_ago = (pc != start_pc) ? 1 : 0;
		     one_ago >= 0; one_ago--) {
			int i;
			for (i = 0; i < num_info_ops; i++) {
				int info_operand = info_operands[i];
				if (info_operand < CALLER_UNKNOWN_BASE)	{
					/* Weird; reserved value, ignore it. */
					continue;
				}

				/* Skip info ops which are not in the
				 * "one_ago" mode we want right now.
				 */
				if (((info_operand & ONE_BUNDLE_AGO_FLAG) != 0)
				    != (one_ago != 0))
					continue;

				/* Clear the flag to make later checking
				 * easier. */
				info_operand &= ~ONE_BUNDLE_AGO_FLAG;

				/* Default to looking at PC_IN_LR_FLAG. */
				if (info_operand & PC_IN_LR_FLAG)
					location->pc_location =
						PC_LOC_IN_LR;
				else
					location->pc_location =
						PC_LOC_ON_STACK;

				switch (info_operand) {
				case CALLER_UNKNOWN_BASE:
					location->pc_location = PC_LOC_UNKNOWN;
					location->sp_location = SP_LOC_UNKNOWN;
					return;

				case CALLER_SP_IN_R52_BASE:
				case CALLER_SP_IN_R52_BASE | PC_IN_LR_FLAG:
					location->sp_location = SP_LOC_IN_R52;
					return;

				default:
				{
					const unsigned int val = info_operand
						- CALLER_SP_OFFSET_BASE;
					const unsigned int sp_offset =
						(val >> NUM_INFO_OP_FLAGS) * 8;
					if (sp_offset < 32768) {
						/* This is a properly encoded
						 * SP offset. */
						location->sp_location =
							SP_LOC_OFFSET;
						location->sp_offset =
							sp_offset;
						return;
					} else {
						/* This looked like an SP
						 * offset, but it's outside
						 * the legal range, so this
						 * must be an unrecognized
						 * info operand.  Ignore it.
						 */
					}
				}
				break;
				}
			}
		}

		if (seen_terminating_bundle) {
			/* We saw a terminating bundle during the previous
			 * iteration, so we were only looking for an info op.
			 */
			break;
		}

		if (bundle.bits == 0) {
			/* Wacky terminating bundle. Stop looping, and hope
			 * we've already seen enough to find the caller.
			 */
			break;
		}

		/*
		 * Try to determine caller's SP.
		 */

		if (!sp_determined) {
			int adjust;
			if (bt_has_addi_sp(&bundle, &adjust)) {
				location->sp_location = SP_LOC_OFFSET;

				if (adjust <= 0) {
					/* We are in prolog about to adjust
					 * SP. */
					location->sp_offset = 0;
				} else {
					/* We are in epilog restoring SP. */
					location->sp_offset = adjust;
				}

				sp_determined = true;
			} else {
				if (bt_has_move_r52_sp(&bundle)) {
					/* Maybe in prolog, creating an
					 * alloca-style frame.  But maybe in
					 * the middle of a fixed-size frame
					 * clobbering r52 with SP.
					 */
					sp_moved_to_r52 = true;
				}

				if (bt_modifies_sp(&bundle)) {
					if (sp_moved_to_r52) {
						/* We saw SP get saved into
						 * r52 earlier (or now), which
						 * must have been in the
						 * prolog, so we now know that
						 * SP is still holding the
						 * caller's sp value.
						 */
						location->sp_location =
							SP_LOC_OFFSET;
						location->sp_offset = 0;
					} else {
						/* Someone must have saved
						 * aside the caller's SP value
						 * into r52, so r52 holds the
						 * current value.
						 */
						location->sp_location =
							SP_LOC_IN_R52;
					}
					sp_determined = true;
				}
			}
		}

		if (bt_has_iret(&bundle)) {
			/* This is a terminating bundle. */
			seen_terminating_bundle = true;
			continue;
		}

		/*
		 * Try to determine caller's PC.
		 */

		jrp_reg = -1;
		has_jrp = bt_has_jrp(&bundle, &jrp_reg);
		if (has_jrp)
			seen_terminating_bundle = true;

		if (location->pc_location == PC_LOC_UNKNOWN) {
			if (has_jrp) {
				if (jrp_reg == TREG_LR && !lr_modified) {
					/* Looks like a leaf function, or else
					 * lr is already restored. */
					location->pc_location =
						PC_LOC_IN_LR;
				} else {
					location->pc_location =
						PC_LOC_ON_STACK;
				}
			} else if (bt_has_sw_sp_lr(&bundle)) {
				/* In prolog, spilling initial lr to stack. */
				location->pc_location = PC_LOC_IN_LR;
			} else if (bt_modifies_lr(&bundle)) {
				lr_modified = true;
			}
		}
	}
}

void
backtrace_init(BacktraceIterator *state,
	       BacktraceMemoryReader read_memory_func,
	       void *read_memory_func_extra,
	       VirtualAddress pc, VirtualAddress lr,
	       VirtualAddress sp, VirtualAddress r52)
{
	CallerLocation location;
	VirtualAddress fp, initial_frame_caller_pc;

	if (read_memory_func == NULL) {
		read_memory_func = bt_read_memory;
	}

	/* Find out where we are in the initial frame. */
	find_caller_pc_and_caller_sp(&location, pc,
				     read_memory_func, read_memory_func_extra);

	switch (location.sp_location) {
	case SP_LOC_UNKNOWN:
		/* Give up. */
		fp = -1;
		break;

	case SP_LOC_IN_R52:
		fp = r52;
		break;

	case SP_LOC_OFFSET:
		fp = sp + location.sp_offset;
		break;

	default:
		/* Give up. */
		fp = -1;
		break;
	}

	/* The frame pointer should theoretically be aligned mod 8. If
	 * it's not even aligned mod 4 then something terrible happened
	 * and we should mark it as invalid.
	 */
	if (fp % 4 != 0)
		fp = -1;

	/* -1 means "don't know initial_frame_caller_pc". */
	initial_frame_caller_pc = -1;

	switch (location.pc_location) {
	case PC_LOC_UNKNOWN:
		/* Give up. */
		fp = -1;
		break;

	case PC_LOC_IN_LR:
		if (lr == 0 || lr % TILE_BUNDLE_ALIGNMENT_IN_BYTES != 0) {
			/* Give up. */
			fp = -1;
		} else {
			initial_frame_caller_pc = lr;
		}
		break;

	case PC_LOC_ON_STACK:
		/* Leave initial_frame_caller_pc as -1,
		 * meaning check the stack.
		 */
		break;

	default:
		/* Give up. */
		fp = -1;
		break;
	}

	state->pc = pc;
	state->sp = sp;
	state->fp = fp;
	state->initial_frame_caller_pc = initial_frame_caller_pc;
	state->read_memory_func = read_memory_func;
	state->read_memory_func_extra = read_memory_func_extra;
}

bool
backtrace_next(BacktraceIterator *state)
{
	VirtualAddress next_fp, next_pc, next_frame[2];

	if (state->fp == -1) {
		/* No parent frame. */
		return false;
	}

	/* Try to read the frame linkage data chaining to the next function. */
	if (!state->read_memory_func(&next_frame, state->fp, sizeof next_frame,
				     state->read_memory_func_extra)) {
		return false;
	}

	next_fp = next_frame[1];
	if (next_fp % 4 != 0) {
		/* Caller's frame pointer is suspect, so give up.
		 * Technically it should be aligned mod 8, but we will
		 * be forgiving here.
		 */
		return false;
	}

	if (state->initial_frame_caller_pc != -1) {
		/* We must be in the initial stack frame and already know the
		 * caller PC.
		 */
		next_pc = state->initial_frame_caller_pc;

		/* Force reading stack next time, in case we were in the
		 * initial frame.  We don't do this above just to paranoidly
		 * avoid changing the struct at all when we return false.
		 */
		state->initial_frame_caller_pc = -1;
	} else {
		/* Get the caller PC from the frame linkage area. */
		next_pc = next_frame[0];
		if (next_pc == 0 ||
		    next_pc % TILE_BUNDLE_ALIGNMENT_IN_BYTES != 0) {
			/* The PC is suspect, so give up. */
			return false;
		}
	}

	/* Update state to become the caller's stack frame. */
	state->pc = next_pc;
	state->sp = state->fp;
	state->fp = next_fp;

	return true;
}

#else /* TILE_CHIP < 10 */

void
backtrace_init(BacktraceIterator *state,
	       BacktraceMemoryReader read_memory_func,
	       void *read_memory_func_extra,
	       VirtualAddress pc, VirtualAddress lr,
	       VirtualAddress sp, VirtualAddress r52)
{
	state->pc = pc;
	state->sp = sp;
	state->fp = -1;
	state->initial_frame_caller_pc = -1;
	state->read_memory_func = read_memory_func;
	state->read_memory_func_extra = read_memory_func_extra;
}

bool backtrace_next(BacktraceIterator *state) { return false; }

#endif /* TILE_CHIP < 10 */
