#include "zc/zasm_utils.h"
#include "base/zdefs.h"
#include "zc/ffscript.h"
#include "zc/script_debug.h"
#include "zasm/serialize.h"
#include "zasm/table.h"
#include <cstdint>
#include <fmt/format.h>
#include <xxhash.h>
#include <vector>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

StructuredZasm zasm_construct_structured(const zasm_script* script)
{
	// Find all function calls.
	std::set<pc_t> function_calls;
	std::set<pc_t> function_calls_goto_pc;
	std::map<pc_t, pc_t> function_calls_pc_to_pc;

	// Three forms of function calls over the ages:

	// 1) GOTO/GOTOR
	// The oldest looks like this:
	//
	//    SETV D2 (pc two after the GOTO)*10000
	//    PUSHR D2
	//    ... other ops to push the function args ...
	//    GOTO x
	// 
	// x: ...
	//    POP D3
	//    GOTOR D3
	//
	// GOTOR only ever used D3. POP D3 could instead be POPARGS.
	// Could also use D3 to set/push the return address.

	// 2) GOTO/RETURN
	//
	//    PUSHV (pc two after the GOTO)
	//    ... other ops to push the function args ...
	//    GOTO x
	// 
	// x: ...
	//    RETURN

	// 3) CALLFUNC/RETURNFUNC
	//
	//    CALLFUNC x
	// 
	// x: ...
	//    RETURNFUNC
	//
	// CALLFUNC pushes the return address onto a function call statck. RETURNFUNC pops from that stack.

	// There is nothing special marking the start or end of a function in ZASM. So,
	// the only way to contruct the bounds of each function is to search for function calls,
	// which gets the function starts. Then, the function ends are derived with these.
	// Note - if a function is not called at all, then the instructions for that function will
	// be part of an unreachable sequence of blocks in the prior function that was called.
	// Therefore, the instructions of uncalled functions should be pruned as part of zasm_optimize.

	// First determine if we have the simpler CALLFUNC instructions.
	auto calling_mode = StructuredZasm::CALLING_MODE_UNKNOWN;
	for (pc_t i = 0; i < script->size && !calling_mode; i++)
	{
		int command = script->zasm[i].command;
		switch (command)
		{
			case GOTOR:
				calling_mode = StructuredZasm::CALLING_MODE_GOTO_GOTOR;
				break;

			case RETURN:
				calling_mode = StructuredZasm::CALLING_MODE_GOTO_RETURN;
				break;

			case CALLFUNC:
			case RETURNFUNC:
				calling_mode = StructuredZasm::CALLING_MODE_CALLFUNC_RETURNFUNC;
				break;
		}
	}
	bool legacy_calling_mode =
		calling_mode == StructuredZasm::CALLING_MODE_GOTO_GOTOR || calling_mode == StructuredZasm::CALLING_MODE_GOTO_RETURN;

	// Starts with implicit first function ("run").
	std::set<pc_t> function_start_pcs_set = {0};

	for (pc_t i = 0; i < script->size; i++)
	{
		int command = script->zasm[i].command;

		bool is_function_call_like = false;
		if (command == CALLFUNC)
		{
			is_function_call_like = true;
		}
		else if (command == STARTDESTRUCTOR)
		{
			function_start_pcs_set.insert(i);
			continue;
		}
		else if (legacy_calling_mode && command == GOTO)
		{
			// Function calls are directly followed with a POP to restore the stack frame pointer.
			// PEEK is also possible via an optimization done by the compiler.
			int next_command = script->zasm[i + 1].command;
			is_function_call_like = (next_command == POP || next_command == PEEK) && script->zasm[i + 1].arg1 == D(4);
		}
		else
		{
			continue;
		}

		if (is_function_call_like && script->zasm[i].arg1 != -1)
		{
			function_calls.insert(i);
			function_start_pcs_set.insert(script->zasm[i].arg1);
			function_calls_pc_to_pc[i] = script->zasm[i].arg1;
			ASSERT(script->zasm[i].arg1 != i + 1);
		}
	}

	std::vector<pc_t> function_start_pcs(function_start_pcs_set.begin(), function_start_pcs_set.end());
	std::vector<pc_t> function_final_pcs;
	std::map<pc_t, pc_t> start_pc_to_function;
	{
		start_pc_to_function[0] = 0;

		pc_t next_fn_id = 1;
		for (int i = 1; i < function_start_pcs.size(); i++)
		{
			pc_t function_start_pc = function_start_pcs[i];
			function_final_pcs.push_back(function_start_pc - 1);
			start_pc_to_function[function_start_pc] = next_fn_id++;
		}

		// Don't include 0xFFFF as part of the last function.
		function_final_pcs.push_back(script->size - 2);

		// Just so std::lower_bound below will work for last function.
		function_start_pcs.push_back(script->size);
	}

	std::vector<ZasmFunction> functions;
	for (pc_t i = 0; i < function_final_pcs.size(); i++)
	{
		functions.push_back({i, "", function_start_pcs.at(i), function_final_pcs.at(i)});
	}

	for (auto [a, b] : function_calls_pc_to_pc)
	{
		auto it = std::lower_bound(function_start_pcs.begin(), function_start_pcs.end(), a);
		ASSERT(it != function_start_pcs.end());
		pc_t callee_pc = std::distance(function_start_pcs.begin(), it) - 1;

		it = std::lower_bound(function_start_pcs.begin(), function_start_pcs.end(), b);
		ASSERT(it != function_start_pcs.end());
		ASSERT(*it == b);
		pc_t call_pc = std::distance(function_start_pcs.begin(), it);

		functions[call_pc].called_by_functions.insert(callee_pc);
		// functions[callee_pc].calls_functions.insert(call_pc);
	}

	return {functions, function_calls, start_pc_to_function, calling_mode};
}

std::set<pc_t> zasm_find_yielding_functions(const zasm_script* script, StructuredZasm& structured_zasm)
{
	std::set<pc_t> yielding_function_ids;
	for (const auto& fn : structured_zasm.functions)
	{
		for (pc_t i = fn.start_pc; i <= fn.final_pc; i++)
		{
			if (command_is_wait(script->zasm[i].command))
			{
				yielding_function_ids.insert(fn.id);
				break;
			}
		}
	}

	std::set<pc_t> seen_ids;
	std::set<pc_t> pending_ids = yielding_function_ids;
	while (pending_ids.size())
	{
		pc_t id = *pending_ids.begin();
		pending_ids.erase(pending_ids.begin());
		seen_ids.insert(id);
		structured_zasm.functions[id].may_yield = true;

		for (auto called_by_id : structured_zasm.functions[id].called_by_functions)
		{
			if (!seen_ids.contains(called_by_id))
				pending_ids.insert(called_by_id);
		}
	}

	return seen_ids;
}

static bool is_in_ranges(pc_t pc, const std::vector<std::pair<pc_t, pc_t>> pc_ranges)
{
	for (auto [start_pc, final_pc] : pc_ranges)
	{
		if (pc >= start_pc && pc <= final_pc)
		{
			return true;
		}
	}

	return false;
}

ZasmCFG zasm_construct_cfg(const zasm_script* script, std::vector<std::pair<pc_t, pc_t>> pc_ranges)
{
	std::set<pc_t> block_starts;

	for (auto [start_pc, final_pc] : pc_ranges)
	{
		block_starts.insert(start_pc);
		for (pc_t i = start_pc; i <= final_pc; i++)
		{
			int command = script->zasm[i].command;
			int arg1 = script->zasm[i].arg1;

			if (command == CALLFUNC || command == GOTO || command == GOTOCMP || command == GOTOTRUE || command == GOTOFALSE || command == GOTOLESS || command == GOTOMORE)
			{
				// Ignore GOTO jumps outside provided bounds.
				// This allows for creating a CFG that is internal to this function only.
				if (!is_in_ranges(arg1, pc_ranges))
				{
					continue;
				}

				// This is a recursive function call to itself!
				if (arg1 == start_pc)
				{
					continue;
				}

				block_starts.insert(arg1);
				if (i + 1 <= final_pc)
					block_starts.insert(i + 1);
			}
			else if (command_is_wait(command))
			{
				if (i + 1 <= final_pc)
					block_starts.insert(i + 1);
			}
		}
	}

	std::map<pc_t, pc_t> start_pc_to_block_id;

	std::vector<pc_t> block_starts_vec(block_starts.begin(), block_starts.end());
	for (pc_t j = 0; j < block_starts_vec.size(); j++)
	{
		start_pc_to_block_id[block_starts_vec[j]] = j;
	}

	std::vector<std::vector<pc_t>> block_edges;

	block_edges.resize(block_starts.size());
	for (pc_t j = 1; j < block_starts_vec.size(); j++)
	{
		int i = block_starts_vec[j];
		int prev_command = script->zasm[i - 1].command;
		int prev_arg1 = script->zasm[i - 1].arg1;
		if (prev_command == GOTO || prev_command == CALLFUNC)
		{
			// Previous block unconditionally continues to some other block.
			auto other_block = start_pc_to_block_id[prev_arg1];
			block_edges[j - 1].push_back(other_block);
		}
		else if (prev_command == GOTOCMP || prev_command == GOTOTRUE || prev_command == GOTOFALSE || prev_command == GOTOLESS || prev_command == GOTOMORE)
		{
			// Previous block conditionally continues to this one, or some other block.
			block_edges[j - 1].push_back(j);
			auto other_block = start_pc_to_block_id[prev_arg1];
			block_edges[j - 1].push_back(other_block);
		}
		else if (prev_command != QUIT && prev_command != RETURN && prev_command != RETURNFUNC && prev_command != GOTOR)
		{
			// Previous block unconditionally continues to this one.
			block_edges[j - 1].push_back(j);
		}
	}

	return {block_starts, start_pc_to_block_id, block_edges};
}

static std::string zasm_fn_get_name(const ZasmFunction& function)
{
	if (function.name.empty())
		return fmt::format("Function #{}", function.id);
	else
		return fmt::format("Function #{} ({})", function.id, function.name);
}

static std::string zasm_to_string(const zasm_script* script, const StructuredZasm& structured_zasm, const ZasmCFG& cfg, std::set<pc_t> function_ids)
{
	std::stringstream ss;

	int block_id = 0;
	for (auto fn_id : function_ids)
	{
		auto& function = structured_zasm.functions[fn_id];

		for (pc_t i = function.start_pc; i <= function.final_pc; i++)
		{
			const auto& op = script->zasm[i];
			pc_t command = op.command;
			std::string str = zasm_op_to_string(op);
			ss <<
				std::setw(5) << std::right << i << ": " <<
				std::left << std::setw(45) << str;
			if (cfg.block_starts.contains(i))
			{
				auto& edges = cfg.block_edges[block_id];
				ss << fmt::format("[Block {} -> {}]", block_id++, fmt::join(edges, ", "));
			}
			if ((command == GOTO && structured_zasm.start_pc_to_function.contains(op.arg1)) || command == CALLFUNC)
			{
				ss << fmt::format("[Call {}]", zasm_fn_get_name(structured_zasm.functions[structured_zasm.start_pc_to_function.at(op.arg1)]));
			}
			ss << '\n';
		}
		ss << '\n';
	}

	return ss.str();
}

std::string zasm_to_string(const zasm_script* script, bool top_functions, bool generate_yielder)
{
	std::stringstream ss;

	auto structured_zasm = zasm_construct_structured(script);

	std::vector<std::pair<pc_t, size_t>> fn_lengths;

	std::set<pc_t> yielding_fns;
	size_t yielding_fn_length = 0;
	if (generate_yielder)
	{
		yielding_fns = zasm_find_yielding_functions(script, structured_zasm);
	}
	if (!yielding_fns.empty())
	{
		std::vector<std::pair<pc_t, pc_t>> pc_ranges;
		for (auto fn_id : yielding_fns)
		{
			auto& fn = structured_zasm.functions[fn_id];
			pc_ranges.emplace_back(fn.start_pc, fn.final_pc);
			yielding_fn_length += fn.final_pc - fn.start_pc + 1;
		}
		auto cfg = zasm_construct_cfg(script, pc_ranges);
		ss << "yielder" << '\n';
		ss << zasm_to_string(script, structured_zasm, cfg, yielding_fns);
		ss << '\n';

		fn_lengths.emplace_back(-1, yielding_fn_length);
	}

	for (pc_t fn_id = 0; fn_id < structured_zasm.functions.size(); fn_id++)
	{
		if (yielding_fns.contains(fn_id))
			continue;

		auto& fn = structured_zasm.functions[fn_id];
		auto cfg = zasm_construct_cfg(script, {{fn.start_pc, fn.final_pc}});
		ss << zasm_fn_get_name(fn) << '\n';
		ss << zasm_to_string(script, structured_zasm, cfg, {fn_id});
		ss << '\n';

		fn_lengths.emplace_back(fn_id, fn.final_pc - fn.start_pc + 1);
	}

	if (top_functions)
	{
		ss << "Top functions:\n\n";
		std::sort(fn_lengths.begin(), fn_lengths.end(), [](auto &left, auto &right) {
			return left.second > right.second;
		});
		int lengths_printed = 0;
		for (auto [fn_id, length] : fn_lengths)
		{
			std::string name = fn_id == -1 ? "yielder" : zasm_fn_get_name(structured_zasm.functions.at(fn_id));
			double percent = (double)length / script->size * 100;
			ss << std::setw(15) << std::left << name + ": " << std::setw(6) << std::left << length << " " << (int)percent << '%' << '\n';
			if (++lengths_printed == 5) break;
		}
		ss << '\n';
	}

	return ss.str();
}

static uint64_t generate_function_hash(const zasm_script* script, const StructuredZasm& structured_zasm, const ZasmFunction& function)
{
	std::vector<uint8_t> data;

	for (pc_t i = function.start_pc; i <= function.final_pc; i++)
	{
		int command = script->zasm[i].command;
		int arg1 = script->zasm[i].arg1;
		int arg2 = script->zasm[i].arg2;
		int arg3 = script->zasm[i].arg3;

		data.push_back(command);

		if (command == GOTO && structured_zasm.function_calls.contains(i))
		{
			const auto& function_call = structured_zasm.functions.at(structured_zasm.start_pc_to_function.at(arg1));
			// TODO: just an estimate.
			data.push_back(function_call.final_pc - function_call.start_pc + 1);
		}
		else if (command == GOTO || command == GOTOLESS || command == GOTOMORE || command == GOTOTRUE || command == GOTOFALSE)
		{
			data.push_back(arg1 - function.start_pc);
		}
		else if (command == SETV)
		{
			// ...
			bool is_return_address = false;
			is_return_address = arg2/10000 >= function.start_pc && arg2/10000 <= function.final_pc;

			data.push_back(arg1);
			if (is_return_address)
			{
				data.push_back(arg2 - function.start_pc);
			}
			else
			{
				data.push_back(arg2);
			}
		}
		else if (command == PUSHV)
		{
			// TODO
			// get block id. if we leave it before GOTO, it's not a ret addr
			// 
			bool is_return_address = false;
			is_return_address = arg1 >= function.start_pc && arg1 <= function.final_pc;
			// for (pc_t j = i + 1; j <= function.final_pc; j++)
			// {
			// 	if (script->zasm[j].command == LOADD) continue;
			// 	if (script->zasm[j].command == PUSHR) continue;
			// 	if (script->zasm[j].command == GOTO && structured_zasm.function_calls.contains(j))
			// 	{
			// 		// bounds check may not be necessary.
			// 		is_return_address = arg1 >= function.start_pc && arg1 <= function.final_pc;
			// 	}
			// 	break;
			// }

			if (is_return_address)
			{
				data.push_back(arg1 - function.start_pc);
			}
			else
			{
				data.push_back(arg1);
			}
		}
		else
		{
			data.push_back(arg1);
			data.push_back(arg2);
			data.push_back(arg3);
		}
	}

	return XXH64(data.data(), data.size(), 0);
}

struct FunctionSummary {
	size_t length;
	size_t count;
};

static void hash_all_functions(const zasm_script* script, std::map<uint64_t, FunctionSummary>& function_counts, size_t& total_length)
{
	auto structured_zasm = zasm_construct_structured(script);
	for (auto& function : structured_zasm.functions)
	{
		auto hash = generate_function_hash(script, structured_zasm, function);
		if (function_counts.contains(hash))
		{
			function_counts.at(hash).count += 1;
		}
		else
		{
			function_counts[hash] = {function.final_pc - function.start_pc + 1, 1};
		}

		total_length += function.final_pc - function.start_pc + 1;
	}
}

std::string zasm_analyze_duplication()
{
	std::map<uint64_t, FunctionSummary> function_counts;
	size_t total_length = 0;

	zasm_for_every_script(false, [&](auto script){
		hash_all_functions(script, function_counts, total_length);
	});

	size_t all_duplicates = 0;
	for (auto [a, b] : function_counts)
	{
		if (b.count > 1)
		{
			size_t dupe = (b.count - 1) * b.length;
			all_duplicates += dupe;
			printf("count: %zu length: %zu dupe: %zu\n", b.count, b.length, dupe);
		}
	}

	printf("all_duplicates: %zu (%d%%)\n", all_duplicates, (int)(all_duplicates * 100.0 / total_length));

	std::stringstream ss;
	return ss.str();
}

void zasm_for_every_script(bool parallel, std::function<void(zasm_script*)> fn)
{
	extern std::vector<std::shared_ptr<zasm_script>> zasm_scripts;

	std::vector<zasm_script*> scripts;
	scripts.reserve(zasm_scripts.size());
	for (auto& script : zasm_scripts)
		if (script->valid())
			scripts.push_back(script.get());

	// TODO: debug issues on web build.
	if (parallel && !is_web())
		std::for_each(std::execution::par_unseq, scripts.begin(), scripts.end(), fn);
	else
		std::for_each(std::execution::seq, scripts.begin(), scripts.end(), fn);
}

// TODO: Broken / not yet needed code for determining how many params a function has, and if it returns something.
// size_t entry_user_fn = -1;
// for (int fid = 0; fid < function_start_pcs.size(); fid++)
// {
// 	size_t start_pc = function_start_pcs[fid];
// 	size_t last_pc = function_last_pcs[fid];

// 	printf("fn: %zu - %zu\n", start_pc, last_pc);
// 	int num_params_v1 = 0;
// 	int max = std::min(script->size, last_pc);
// 	for (int i = start_pc; i < max; i++)
// 	{
// 		int command = script->zasm[i].command;
// 		int arg1 = script->zasm[i].arg1;
// 		if (command != LOADD || arg1 != D(2))
// 			continue;

// 		int arg2 = script->zasm[i].arg2;
// 		num_params_v1 = std::max(num_params_v1, arg2 / 10000);
// 	}

// 	int num_params_v2 = 0;
// 	max = std::min(script->size - 1, last_pc);
// 	for (int i = start_pc; i < max; i++)
// 	{
// 		if (script->zasm[i].command != POPARGS || (script->zasm[i + 1].command != RETURN && script->zasm[i + 1].command != QUIT))
// 			continue;

// 		int arg2 = script->zasm[i].arg2;
// 		num_params_v2 = arg2 - 1;
// 		break;
// 	}

// 	printf("num_params_v1: %d\n", num_params_v1);
// 	printf("num_params_v2: %d\n", num_params_v2);
// 	if (num_params_v1 != num_params_v2)
// 		printf("SAD\n");

// 	bool has_ret_value = false;
// 	max = std::min(script->size - 1, last_pc);
// 	for (int i = start_pc; i < max; i++)
// 	{
// 		int command = script->zasm[i].command;
// 		int arg1 = script->zasm[i].arg1;
// 		if (command != SETV || arg1 != D(2))
// 			continue;

// 		i++;
// 		command = script->zasm[i].command;
// 		arg1 = script->zasm[i].arg1;
// 		if (command != GOTO || arg1 >= last_pc)
// 			continue;

// 		int j = arg1;
// 		if (j >= script->size)
// 			continue;

// 		if (script->zasm[j].command == POPARGS && (script->zasm[j + 1].command == RETURN || script->zasm[j + 1].command == QUIT))
// 		{
// 			has_ret_value = true;
// 			break;
// 		}
// 	}

// 	printf("has_ret_val: %d\n", has_ret_value?1:0);

// 	int num_returns = has_ret_value?1:0;
// 	int num_params = num_params_v1;

std::pair<bool, bool> get_command_rw(int command, int arg)
{
	bool read = false;
	bool write = false;

	auto& sc = get_script_command(command);
	if (sc.args >= arg)
	{
		if (sc.arg_type[arg] == ARGTY::READ_REG)
			read = true;
		if (sc.arg_type[arg] == ARGTY::WRITE_REG)
			write = true;
		if (sc.arg_type[arg] == ARGTY::READWRITE_REG)
			read = write = true;
	}
	
	return {read, write};
}
