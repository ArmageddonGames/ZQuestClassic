// There are multiple tests that ensure the correctness of these optimizations:
//
// 1. python tests/test_optimize_zasm_unit.py
//
//    Optimizes small synthetic snippets of ZASM, each saved as a snapshot test.
//    See the files in tests/optimize_zasm and their expected values in tests/snapshots/optimize_zasm_unit
//    To see all at once: python tests/test_optimize_zasm_unit.py --print
//
// 2. python tests/test_optimize_zasm.py
//
//    Optimizes the scripts in all the quests in tests/replays, and creates a snapshot test for the summary
//    of the optimizer
//
// 3. zplayer -test-zc will run a few unit tests, located in this file.
//
// 4. python scripts/run_for_every_qst.py --starting_index 235 ./build/Debug/zplayer -extract-zasm %s -optimize-zasm 2>&1 | code -
//
//    Run in debug mode (for asserts) on every quest in the database.
//
// 5. Finally, all the replay tests run with these optimizations applied, and since as of this writing no replays
//    were recorded w/ these optimizations, they grant a high confidence that these optimizations are sound.

#include "zc/zasm_optimize.h"
#include "base/general.h"
#include "base/util.h"
#include "base/zapp.h"
#include "base/zdefs.h"
#include "parser/parserDefs.h"
#include "zc/ffscript.h"
#include "zc/script_debug.h"
#include "zc/zasm_utils.h"
#include "zasm/table.h"
#include "zasm/serialize.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <stdbool.h>
#include <string>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <mutex>

static bool verbose = false;

bool zasm_optimize_enabled()
{
	static int compat_enabled = zc_get_config("zeldadx", "optimize_zasm", -100);
	static bool enabled = get_flag_bool("-optimize-zasm").value_or(false) ?
		true :
		compat_enabled != -100 ?
			compat_enabled :
			zc_get_config("ZSCRIPT", "optimize_zasm", true);
	return enabled;
}

// TODO: remove when more stable.
// Need to clean up the code for what registers/commands have side effects, use implicit registers, etc.
// Need to verify nothing was missed.
static bool should_run_experimental_passes()
{
	static bool enabled = get_flag_bool("-optimize-zasm-experimental").has_value() || get_flag_bool("-test-optimize-zasm").has_value() || get_flag_bool("-extract-zasm").has_value() || get_flag_bool("-replay-exit-when-done").has_value() || is_ci();
	return enabled;
}

// Very useful tool for identifying a single bad optimization.
// Use with a tool like `find-first-fail`: https://gitlab.com/ole.tange/tangetools/-/blob/master/find-first-fail/find-first-fail
// 1. Enable ENABLE_BISECT_TOOL below.
// 2. Make a new script `tmp.sh` calling a failing replay:
//        python tests/run_replay_tests.py --filter stellar --frame 40000 --extra_args="-replay-fail-assert-instant -test-bisect $1"
// 3. Run the bisect script (may need to increase the end range up to 100000 or more):
//        bash ~/tools/find-first-fail.sh -s 0 -e 1000 -v -q bash tmp.sh
// 4. For the number given, set `-test-bisect` to that, and set a breakpoint
//    where specified in bisect_tool_should_skip. Whatever block being processed is the one to focus on.
// #define ENABLE_BISECT_TOOL
static bool bisect_tool_should_skip()
{
#ifdef ENABLE_BISECT_TOOL
	static int c = 0;
	static int x = get_flag_int("-test-bisect").value();
	// Skip the first x calls.
	bool skip = 0 <= c && c < x;
	c++;
	if (!skip)
		// Set a breakpoint here.
		x = x;
	return skip;
#else
	return false;
#endif
}

enum class ValueType
{
	Uninitialized,
	Number,
	Register,
	Expression,
	Unknown,
};

struct SimulationValue
{
	bool operator==(const SimulationValue& other) const
	{
		if (type != other.type) return false;
		if (data != other.data) return false;
		if (type == ValueType::Expression)
		{
			if (!(*op1.get()).operator==(*other.op1.get())) return false;
			if (!(*op2.get()).operator==(*other.op2.get())) return false;
		}
		return true;
	}

	static SimulationValue num(int x)
	{
		return {ValueType::Number, x};
	}

	static SimulationValue reg(int x)
	{
		return {ValueType::Register, x};
	}

	static SimulationValue expr(SimulationValue x, int cmp, SimulationValue y)
	{
		auto op1 = std::make_shared<SimulationValue>(x);
		auto op2 = std::make_shared<SimulationValue>(y);
		return {ValueType::Expression, cmp, op1, op2};
	}

	bool is_register() const
	{
		return type == ValueType::Register;
	}

	bool is_number() const
	{
		return type == ValueType::Number;
	}

	bool is_expression() const
	{
		return type == ValueType::Expression;
	}

	bool is_equality_expression() const
	{
		return type == ValueType::Expression && (data == CMP_EQ);
	}

	SimulationValue negate() const
	{
		switch (type)
		{
			case ValueType::Number:
				return num(-data);
			case ValueType::Expression:
				return expr(*op1, INVERT_CMP(data), *op2);
			default:
				return {ValueType::Unknown};
		}
	}

	bool is_mutually_exclusive(const SimulationValue& other) const
	{
		ASSERT(type == ValueType::Expression);
		if (other.type != ValueType::Expression)
			return false;
		return *this == other.negate();
	}

	std::string to_string() const
	{
		if (type == ValueType::Expression)
		{
			if (is_bool_cast())
				return fmt::format("{}({})", data & CMP_SETI ? "IBool" : "Bool", op1->to_string());
			if (is_bool_cast_negated())
				return fmt::format("!{}({})", data & CMP_SETI ? "IBool" : "Bool", op1->to_string());

			std::string lhs = op1->type == ValueType::Expression ? fmt::format("({})", op1->to_string()) : op1->to_string();
			std::string rhs = op2->type == ValueType::Expression ? fmt::format("({})", op2->to_string()) : op2->to_string();

			if (data & CMP_BOOL)
				return fmt::format("Bool({}) {} Bool({})", lhs, CMP_STR(data | ~CMP_BOOL), rhs);
			return fmt::format("{} {} {}", lhs, CMP_STR(data), rhs);
		}
		if (type == ValueType::Register)
			return zasm_var_to_string(data);
		if (type == ValueType::Number)
			return fmt::format("{}", data);
		return "?";
	}

	bool is_bool_cast() const
	{
		if (type != ValueType::Expression || !(data & CMP_FLAGS))
			return false;
		if (!op2->is_number())
			return false;
		return op2->data == 0 && (data & CMP_FLAGS) == CMP_NE;
	}

	bool is_bool_cast_negated() const
	{
		if (type != ValueType::Expression || !(data & CMP_FLAGS))
			return false;
		if (!op2->is_number())
			return false;
		return op2->data == 0 && (data & CMP_FLAGS) == CMP_EQ;
	}

	void replace_value(const SimulationValue& match, const SimulationValue& replace_with)
	{
		if (*this == match)
			*this = replace_with;

		if (type == ValueType::Expression)
		{
			op1->replace_value(match, replace_with);
			op2->replace_value(match, replace_with);
		}
	}

	ValueType type = ValueType::Uninitialized;
	// [number] integer value
	// [register] register
	// [expression] cmp
	int data;
	// [expression]
	std::shared_ptr<SimulationValue> op1;
	std::shared_ptr<SimulationValue> op2;
};

#define num(x) SimulationValue::num(x)
#define reg(x) SimulationValue::reg(x)
#define expr(x, cmp, y) SimulationValue::expr(x, cmp, y)
#define boolean_cast(x) SimulationValue::expr(x, CMP_NE, num(0))
#define num_one num(1)
#define num_zero num(0)

template<class... Args>
bool one_of(const unsigned int var, const Args&... args)
{
  return ((var == args) || ...);
}

static const char *expect_file;
static int expect_line;
static bool tests_passed;
#define EXPECT(...) {\
	expect_file = __FILE__;\
	expect_line = __LINE__;\
	expect(__VA_ARGS__);\
}

static void print_string_delta(const std::string& expected, const std::string& got)
{
	std::vector<std::string> expected_lines, got_lines;
	util::split(expected, expected_lines, '\n');
	util::split(got, got_lines, '\n');
	if (expected_lines.size() <= 1 && got_lines.size() <= 1)
		return;

	fmt::println("= delta\n");
	for (int i = 0; i < std::max(expected_lines.size(), got_lines.size()); i++)
	{
		std::string expected_line = i < expected_lines.size() ? expected_lines[i] : "EOF";
		std::string got_line = i < got_lines.size() ? got_lines[i] : "EOF";
		if (expected_line == got_line)
		{
			fmt::println("  {}", expected_lines[i]);
		}
		else
		{
			fmt::println("- {}", expected_lines[i]);
			fmt::println("+ {}", got_lines[i]);
		}
	}
}

static void normalize_whitespace(std::string& text)
{
	util::trimstr(text);
	std::vector<std::string> lines;
	util::split(text, lines, '\n');
	for (auto& line : lines)
		line.erase(line.find_last_not_of("\t\n\v\f\r ") + 1);

    std::ostringstream os;
    for (int i = 0; i < lines.size() - 1; ++i) {
        os << lines.at(i) << "\n";
    }
    os << lines.back();
    text = os.str();
}

std::string zasm_to_string_clean(const zasm_script* script)
{
	std::string str = zasm_to_string(script);
	normalize_whitespace(str);
	util::trimstr(str);
	return str + "\n";
}

template <typename T1, typename T2>
static void expect(std::string name, const T1& expected, const T2& got)
{
	if (expected != got)
	{
		tests_passed = false;
		fmt::println("failure: {}\n{}:{}\n", name, expect_file, expect_line);
		fmt::println("expected {}, but got: {}\n", expected, got);
	}
}

static void expect(std::string name, const SimulationValue& expected, const SimulationValue& got)
{
	if (expected != got)
	{
		tests_passed = false;
		fmt::println("failure: {}\n{}:{}\n", name, expect_file, expect_line);
		fmt::println("expected {}, but got: {}\n", expected.to_string(), got.to_string());
	}
}

static void expect(std::string name, zasm_script* script, std::vector<ffscript>&& s)
{
	bool success = script->size == s.size();
	for (int i = 0; i < s.size(); i++)
	{
		if (!success) break;
		if (script->zasm[i].command == NOP && s[i].command == NOP)
			continue;
		if (script->zasm[i] != s[i])
			success = false;
	}

	if (!success)
	{
		tests_passed = false;
		auto expected_script = zasm_script{std::move(s)};
		std::string expected = zasm_to_string_clean(&expected_script);
		std::string got = zasm_to_string_clean(script);
		fmt::println("failure: {}\n{}:{}\n", name, expect_file, expect_line);
		fmt::println("= expected:\n\n{}", expected);
		fmt::println("= got:\n\n{}", got);
		print_string_delta(expected, got);
	}
}

struct OptContext
{
	uint32_t saved;
	zasm_script* script;
	ZasmFunction fn;
	ZasmCFG cfg;
	bool cfg_stale;
	std::vector<pc_t> block_starts;
	std::set<pc_t> block_unreachable;
	StructuredZasm* structured_zasm;
	bool debug;
};

#define C(i) (ctx.script->zasm[i])
#define E(i) (ctx.cfg.block_edges.at(i))

static OptContext create_context_no_cfg(StructuredZasm& structured_zasm, zasm_script* script, const ZasmFunction& fn)
{
	OptContext ctx{};
	ctx.structured_zasm = &structured_zasm;
	ctx.script = script;
	ctx.fn = fn;
	ctx.debug = verbose;
	ctx.cfg_stale = true;
	return ctx;
}

static void add_context_cfg(OptContext& ctx)
{
	if (!ctx.cfg_stale)
		return;

	ctx.cfg = zasm_construct_cfg(ctx.script, {{ctx.fn.start_pc, ctx.fn.final_pc}});
	ctx.block_starts = std::vector<pc_t>(ctx.cfg.block_starts.begin(), ctx.cfg.block_starts.end());
	ctx.block_unreachable.clear();
	ctx.cfg_stale = false;
}

static OptContext create_context(StructuredZasm& structured_zasm, zasm_script* script, const ZasmFunction& fn)
{
	auto ctx = create_context_no_cfg(structured_zasm, script, fn);
	add_context_cfg(ctx);
	return ctx;
}

static void remove(OptContext& ctx, pc_t start, pc_t final)
{
	for (int i = start; i <= final; i++)
		C(i).command = NOP;
	ctx.saved += final - start + 1;
}

static void remove(OptContext& ctx, pc_t pc)
{
	C(pc).command = NOP;
	ctx.saved += 1;
}

template <typename T>
static void for_every_command_register_arg(const ffscript& instr, T fn)
{
	auto& sc = get_script_command(instr.command);

	if (sc.is_register(0))
	{
		auto [read, write] = get_command_rw(instr.command, 0);
		fn(read, write, instr.arg1, 0);
	}

	if (sc.is_register(1))
	{
		auto [read, write] = get_command_rw(instr.command, 1);
		fn(read, write, instr.arg2, 1);
	}

	if (sc.is_register(2))
	{
		auto [read, write] = get_command_rw(instr.command, 2);
		fn(read, write, instr.arg3, 2);
	}
}

template <typename T>
static void for_every_command_arg(ffscript& instr, T fn)
{
	auto& sc = get_script_command(instr.command);

	if (sc.args >= 1)
	{
		auto [read, write] = get_command_rw(instr.command, 0);
		fn(read, write, instr.arg1, 0);
	}

	if (sc.args >= 2)
	{
		auto [read, write] = get_command_rw(instr.command, 1);
		fn(read, write, instr.arg2, 1);
	}

	if (sc.args >= 3)
	{
		auto [read, write] = get_command_rw(instr.command, 2);
		fn(read, write, instr.arg3, 2);
	}
}

// TODO: finish implementing these in get_register_ref_dependency,
// then delete this function.
static bool has_implemented_register_invalidations(int reg)
{
	switch (reg)
	{
		case CLASS_THISKEY:
		case CLASS_THISKEY2:
		case REFBITMAP:
		case REFBOTTLESHOP:
		case REFBOTTLETYPE:
		case REFCHEATS:
		case REFDIRECTORY:
		case REFDMAPDATA:
		case REFDOORS:
		case REFDROPS:
		case REFEWPN:
		case REFFILE:
		case REFGAMEDATA:
		case REFGENERICDATA:
		case REFITEMCLASS:
		case REFLWPN:
		case REFMAPDATA:
		case REFMSGDATA:
		case REFNPCCLASS:
		case REFPALCYCLE:
		case REFPALDATA:
		case REFPALETTE:
		case REFPONDS:
		case REFRGB:
		case REFRNG:
		case REFSCREENDATA:
		case REFSHOPDATA:
		case REFSPRITEDATA:
		case REFSTACK:
		case REFSUBSCREEN:
		case REFSUBSCREENPAGE:
		case REFSUBSCREENWIDG:
		case REFTUNES:
		case REFUICOLOURS:
		case REFUNTYPED:
		case REFWARPRINGS:
		case REFWEBSOCKET:
			return false;
	}

	return true;
}

template <typename T>
static void for_every_command_register_arg_d(const ffscript& instr, T fn)
{
	std::array<bool, 8> r{}, w{};

	for (auto [reg, rw] : get_command_implicit_dependencies(instr.command))
	{
		if (reg >= 8)
			continue;

		r[reg] |= rw == ARGTY::READWRITE_REG || rw == ARGTY::READ_REG;
		w[reg] |= rw == ARGTY::READWRITE_REG || rw == ARGTY::WRITE_REG;
	}

	for_every_command_register_arg(instr, [&](bool read, bool write, int reg, int argn){
		for (auto reg2 : get_register_dependencies(reg))
			if (reg2 < 8) r[reg2] |= true;

		if (reg >= 8)
			return;

		r[reg] |= read;
		w[reg] |= write;
	});

	for (int i = 0; i < 8; i++)
	{
		bool read = r[i];
		bool write = w[i];
		if (read || write)
			fn(read, write, i);
	}
}

template <typename T>
static void for_every_command_register_arg_include_indices(const ffscript& instr, T fn)
{
	for (auto [reg, rw] : get_command_implicit_dependencies(instr.command))
	{
		bool read = rw == ARGTY::READWRITE_REG || rw == ARGTY::READ_REG;
		bool write = rw == ARGTY::READWRITE_REG || rw == ARGTY::WRITE_REG;
		fn(read, write, reg, -1);
	}

	for_every_command_register_arg(instr, [&](bool read, bool write, int reg, int argn){
		if (auto r = get_register_ref_dependency(reg))
			fn(true, false, *r, -1);
		for (auto r : get_register_dependencies(reg))
			fn(true, false, r, -1);
		fn(read, write, reg, argn);
	});
}

template<typename T>
static void optimize_by_block(OptContext& ctx, T cb)
{
	for (pc_t i = 0; i < ctx.block_starts.size(); i++)
	{
		if (ctx.block_unreachable.contains(i))
			continue;

		pc_t start_pc = ctx.block_starts[i];
		pc_t final_pc = i == ctx.block_starts.size() - 1 ?
			ctx.fn.final_pc :
			ctx.block_starts[i + 1] - 1;
		cb(i, start_pc, final_pc);
	}
}

static void optimize_goto_next_instruction(OptContext& ctx)
{
	for (pc_t i = 0; i < ctx.script->size; i++)
	{
		// If this is a GOTO to the next instruction, remove it.
		// This produces better blocks in the CFG construction because:
		// - usages of the CFG may assume blocks with a conditional branch MUST have two edges to distinct blocks
		// - it also is a minor optimization
		if (command_is_goto(C(i).command) && C(i).arg1 == i + 1)
		{
			if (bisect_tool_should_skip())
				continue;

			remove(ctx, i);
		}
	}
}

static void optimize_conseq_additive_impl(OptContext& ctx, word from, word to, bool write_at_end = false)
{
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		for (pc_t j = final_pc; j > start_pc; j--)
		{
			int command = C(j).command;
			int arg1 = C(j).arg1;
			if (command != from) continue;

			if ((from == PUSHV || from == PUSHR) && ctx.structured_zasm->function_calls.contains(j + 1))
			{
				// Don't break the thing that function calls are derived from in older ZASM.
				continue;
			}

			pc_t start = j;
			pc_t end = j;
			while (start >= start_pc)
			{
				int prev_command = C(start - 1).command;
				int prev_arg1 = C(start - 1).arg1;
				if (prev_command != from)
					break;
				// D5 is a special "null" register - it is never valid to read
				// from it, so we are free to remove writes.
				// ...except for the initial script call, which may have set initd[5].
				if (!(prev_arg1 == arg1 || (ctx.fn.id != 0 && prev_arg1 == D(5))))
					break;

				start--;
			}

			int count = end - start + 1;
			if (count > 1)
			{
				if (bisect_tool_should_skip())
					continue;

				if (write_at_end)
				{
					C(end) = {to, arg1, count};
					remove(ctx, start, end - 1);
				}
				else
				{
					C(start) = {to, arg1, count};
					remove(ctx, start + 1, end);
				}

				j = start;
			}
		}
	});
}

static void optimize_conseq_additive(OptContext& ctx)
{
	add_context_cfg(ctx);
	optimize_conseq_additive_impl(ctx, PUSHR, PUSHARGSR);
	optimize_conseq_additive_impl(ctx, PUSHV, PUSHARGSV);
	optimize_conseq_additive_impl(ctx, PUSHVARGR, PUSHVARGSR);
	optimize_conseq_additive_impl(ctx, PUSHVARGV, PUSHVARGSV);
	optimize_conseq_additive_impl(ctx, POP, POPARGS, true);
}

// SETR, ADDV, LOADI -> LOAD
// SETR, ADDV, STOREI -> STORE
// Ex:
//   SETR            D6              D4
//   ADDV            D6              40000
//   LOADI           D2              D6
// ->
//   LOAD            D2              4
static void optimize_load_store(OptContext& ctx)
{
	if (bisect_tool_should_skip())
		return;

	for (pc_t i = ctx.fn.start_pc + 2; i < ctx.fn.final_pc; i++)
	{
		int command = C(i).command;
		int arg1 = C(i).arg1;
		int arg2 = C(i).arg2;
		if (!one_of(command, LOADI, STOREI) || arg2 != rSFTEMP) continue;

		bool bail = false;
		int setr_pc = i - 2;
		while (!(C(setr_pc).command == SETR && C(setr_pc).arg1 == rSFTEMP && C(setr_pc).arg2 == rSFRAME))
		{
			if (setr_pc == ctx.fn.start_pc)
			{
				bail = true;
				break;
			}
			setr_pc -= 1;
		}

		ASSERT(!bail);
		if (bail)
			continue;

		int addv_arg2 = C(setr_pc + 1).arg2;
		ASSERT(C(setr_pc + 1).command == ADDV);

		word new_command = command == LOADI ? LOADD : STORED;
		C(i) = {new_command, arg1, addv_arg2};
		remove(ctx, setr_pc, setr_pc + 1);
	}

	for (pc_t i = ctx.fn.start_pc; i < ctx.fn.final_pc; i++)
	{
		int command = C(i).command;
		int arg1 = C(i).arg1;
		int arg2 = C(i).arg2;

		if (command == SETR && arg1 == rSFRAME && arg2 == SP)
		{
			C(i).arg2 = SP2;
			continue;
		}

		if (!one_of(command, LOADD, STORED, STOREDV)) continue;

		word new_command;
		switch (command)
		{
			case LOADD: new_command = LOAD; break;
			case STORED: new_command = STORE; break;
			case STOREDV: new_command = STOREV; break;
		}
		ASSERT(arg2 % 10000 == 0);
		C(i) = {new_command, arg1, arg2 / 10000};
	}
}

// SETV, PUSHR -> PUSHV
// Ex:
//   SETV            D2              5420000
//   PUSHR           D2
// ->
//   PUSHV           D2              5420000
static void optimize_setv_pushr(OptContext& ctx)
{
	add_context_cfg(ctx);
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		for (int j = start_pc; j < final_pc; j++)
		{
			if (C(j).command != SETV) continue;
			if (C(j + 1).command != PUSHR) continue;
			if (C(j).arg1 != C(j + 1).arg1) continue;
			if (bisect_tool_should_skip()) continue;

			// `zasm_construct_structured` is sensitive to a PUSH being just before
			// a function call, so unlike other places assign the NOP to the first
			// instruction.
			C(j + 1) = {PUSHV, C(j).arg2};
			remove(ctx, j);
		}
	});
}

// SETR, PUSHR -> PUSHV
// Ex:
//   SETR            D2              GD0
//   PUSHR           D2
// ->
//   PUSHR           GD0
static void optimize_setr_pushr(OptContext& ctx)
{
	add_context_cfg(ctx);
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		for (int j = start_pc; j < final_pc; j++)
		{
			if (C(j).command != SETR) continue;
			if (C(j + 1).command != PUSHR) continue;
			if (C(j).arg1 == rSFRAME) continue;
			if (C(j).arg1 != C(j + 1).arg1) continue;
			if (j + 2 <= final_pc)
			{
				bool reads_from_reg = false;
				for_every_command_register_arg_include_indices(C(j + 2), [&](bool read, bool write, int reg, int arg){
					if (reg == C(j).arg1 && read)
						reads_from_reg = true;
				});
				if (reads_from_reg) continue;
			}
			if (bisect_tool_should_skip()) continue;

			// `zasm_construct_structured` is sensitive to a PUSH being just before
			// a function call, so unlike other places assign the NOP to the first
			// instruction.
			C(j + 1) = {PUSHR, C(j).arg2};
			remove(ctx, j);
		}
	});
}

static void optimize_stack(OptContext& ctx)
{
	add_context_cfg(ctx);
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		for (int j = start_pc; j < final_pc; j++)
		{
			if (C(j).command != PUSHR) continue;

			int reg = C(j).arg1;
			int count = 0;
			for (int k = j + 1; k <= final_pc; k++)
			{
				int command = C(k).command;
				int arg2 = C(k).arg2;
				if (command == POP && C(k).arg1 == reg)
				{
					if (count != 0)
						break;
					if (bisect_tool_should_skip())
						break;
					remove(ctx, j);
					remove(ctx, k);
					break;
				}

				switch (command)
				{
					case POP:
						count--;
						break;
					case POPARGS:
						count -= arg2;
						break;
					case PUSHR:
					case PUSHV:
						count++;
						break;
					case PUSHARGSR:
					case PUSHARGSV:
						count += arg2;
						break;
				}

				if (count < 0)
					break;

				bool writes_to_reg = false;
				for_every_command_register_arg(C(k), [&](bool read, bool write, int arg, int argn){
					if (arg == reg && write)
						writes_to_reg = true;
				});
				if (writes_to_reg)
					break;

				if (command == CALLFUNC)
					break;
			}
		}
	});
}

static pc_t get_block_final(const OptContext& ctx, int block)
{
	return block == ctx.block_starts.size() - 1 ?
		ctx.fn.final_pc :
		ctx.block_starts.at(block + 1) - 1;
}

static std::pair<pc_t, pc_t> get_block_bounds(const OptContext& ctx, int block)
{
	return {ctx.block_starts.at(block), get_block_final(ctx, block)};
}

struct SimulationState
{
	pc_t block = 0;
	pc_t pc = 0;
	pc_t final_pc = 0;
	SimulationValue d[8];
	SimulationValue operand_1;
	int operand_1_backing_reg = -1;
	SimulationValue operand_2;
	int operand_2_backing_reg = -1;
	std::vector<SimulationValue> stack;
	bool side_effects = false;
	bool bail = false;

	SimulationState()
	{
		for (int i = 0; i < 8; i++)
			d[i] = {ValueType::Register, D(i)};
	}

	void set_block(const OptContext& ctx, pc_t target_block)
	{
		block = target_block;
		std::tie(pc, final_pc) = get_block_bounds(ctx, target_block);
	}
};

static SimulationValue evaluate_binary_op(int cmp, SimulationValue a, SimulationValue b)
{
	ASSERT(cmp <= (CMP_FLAGS|CMP_BOOL|CMP_SETI));
	int flags = cmp & CMP_FLAGS;
	bool seti = cmp & CMP_SETI;
	bool boolcast = cmp & CMP_BOOL;

	if (cmp == 0)
		return num_zero;
	if (flags == CMP_FLAGS)
		return seti ? num(10000) : num_one;

	if (a.type == ValueType::Uninitialized || b.type == ValueType::Uninitialized)
		return {ValueType::Uninitialized};
	if (a.type == ValueType::Unknown || b.type == ValueType::Unknown)
		return {ValueType::Unknown};

	if (a.is_number() && b.is_number())
	{
		if (boolcast)
		{
			ASSERT(flags == CMP_EQ || flags == CMP_NE);
			bool v = !!a.data == !!b.data;
			if (flags == CMP_NE) v = !v;
			return num(v ? 1 : 0);
		}

		bool b_val = false;
		if (flags & CMP_EQ) b_val |= a.data == b.data;
		if (flags & CMP_LT) b_val |= a.data < b.data;
		if (flags & CMP_GT) b_val |= a.data > b.data;

		int v = b_val ? 1 : 0;
		if (seti) v *= 10000;
		return num(v);
	}

	bool should_swap = false;
	if (a.type != ValueType::Expression && b.type == ValueType::Expression)
		should_swap = true;
	if (a.is_number())
		should_swap = true;
	if (should_swap)
	{
		std::swap(a, b);

		// Flip the comparison.
		bool lt = flags & CMP_LT;
		bool gt = flags & CMP_GT;
		bool eq = flags & CMP_EQ;
		std::swap(lt, gt);
		cmp = 0;
		cmp |= (lt ? CMP_LT : 0);
		cmp |= (gt ? CMP_GT : 0);
		cmp |= (eq ? CMP_EQ : 0);
		cmp |= (seti ? CMP_SETI : 0);
		cmp |= (boolcast ? CMP_BOOL : 0);
		flags = cmp & CMP_FLAGS;
	}

	int true_as_int = (a.data & CMP_SETI) ? 10000 : 1;
	if (!seti)
	{
		if (a.type == ValueType::Expression) a.data = a.data & ~CMP_SETI;
		if (b.type == ValueType::Expression) b.data = b.data & ~CMP_SETI;
	}

	if (flags == CMP_EQ && a.type == ValueType::Expression && b == num_one)
		return a;
	if (flags == CMP_EQ && a.type == ValueType::Expression && b == num_zero)
	{
		if (!seti)
			a.data = a.data & ~CMP_SETI;
		auto result = a.negate();
		if (seti)
			result.data |= CMP_SETI;
		return result;
	}

	if (flags == CMP_EQ && ((a.is_number() && a.data == 0) || (b.is_number() && b.data == 0)))
	{
		const auto& not_int = a.is_number() ? b : a;
		if (not_int.type == ValueType::Expression)
			return not_int.negate();
	}

	if (flags == CMP_NE && ((a.is_number() && a.data) || (b.is_number() && b.data)))
	{
		const auto& not_int = a.is_number() ? b : a;
		if (not_int.type == ValueType::Expression)
			return not_int.negate();
	}

	if (flags == CMP_NE && ((a.is_number() && a.data == 0) || (b.is_number() && b.data == 0)))
	{
		const auto& not_int = a.is_number() ? b : a;
		if (not_int.type == ValueType::Expression)
			return not_int;
		return expr(not_int, CMP_NE, num(0));
	}

	if (a.type == ValueType::Register && b.is_number())
		return expr(a, cmp, b);

	if (a.type == b.type && ((flags & CMP_EQ) || flags == CMP_NE) && a == b)
	{
		if ((flags & CMP_EQ))
			return seti ? num(10000) : num_one;
		return num_zero;
	}

	if (!(flags & CMP_EQ) && a == b)
		return num_zero;

	if (a.type == ValueType::Register && b.type == ValueType::Register)
	{
		auto result = SimulationValue{ValueType::Expression, cmp};
		result.op1 = std::make_shared<SimulationValue>(a);
		result.op2 = std::make_shared<SimulationValue>(b);
		return result;
	}

	if (a.type == ValueType::Expression && b.type == ValueType::Expression)
	{
		if (flags == CMP_EQ && a.is_mutually_exclusive(b))
			return num_zero;

		auto result = SimulationValue{ValueType::Expression, cmp};
		result.op1 = std::make_shared<SimulationValue>(a);
		result.op2 = std::make_shared<SimulationValue>(b);
		return result;
	}

	if (a.type == ValueType::Expression && b.is_number())
	{
		int b_val = boolcast ? (b.data ? 1 : 0) : b.data;

		// Check the integer value of the expression against b.
		bool could_gt = true_as_int > b_val || 0 > b_val;
		bool could_lt = true_as_int < b_val || 0 < b_val;
		bool could_eq = true_as_int == b_val || 0 == b_val;

		if (flags == CMP_EQ && !could_eq)
			return num_zero;
		if (flags == CMP_GT && !could_gt)
			return num_zero;
		if (flags == CMP_LT && !could_lt)
			return num_zero;

		bool must_gt = 0 > b_val;
		bool must_lt = true_as_int < b_val;

		if ((flags & CMP_GT) && must_gt)
			return num_one;
		if ((flags & CMP_LT) && must_lt)
			return num_one;

		bool must_ge = 0 >= b_val;
		bool must_le = true_as_int <= b_val;

		if (flags == CMP_GE && must_ge)
			return num_one;
		if (flags == CMP_LE && must_le)
			return num_one;

		return a;
	}

	return {ValueType::Unknown};
}

static void infer_assign(OptContext& ctx, SimulationState& state, int reg, const SimulationValue& rhs)
{
	// If this is <reg == any>, just set the register to op2.
	if (rhs.type == ValueType::Expression && rhs.data == CMP_EQ)
	{
		bool just_one_register = (rhs.op1->type == ValueType::Register) ^ (rhs.op2->type == ValueType::Register);
		ASSERT(just_one_register);
		if (just_one_register)
		{
			if (rhs.op1->type == ValueType::Register && rhs.op1->data == reg)
			{
				state.d[reg] = *rhs.op2;
				return;
			}
			if (rhs.op2->type == ValueType::Register && rhs.op2->data == reg)
			{
				state.d[reg] = *rhs.op1;
				return;
			}
		}
	}

	state.d[reg] = rhs;
}

static void infer(SimulationValue& v, const SimulationValue& given)
{
	if (given.is_equality_expression() && *given.op1 == v)
		v = *given.op2;
	else if (given.is_mutually_exclusive(v))
		v = num_zero;
	else
		v.replace_value(given, num_one);
}

static void infer_values_given_branch(OptContext& ctx, SimulationState& state)
{
	int branch_command = C(state.pc).command;
	int branch_arg2 = C(state.pc).arg2;
	int branch_cmp = command_to_cmp(branch_command, branch_arg2);
	auto expression = evaluate_binary_op(branch_cmp, state.operand_1, state.operand_2);
	ASSERT(expression.is_expression() || expression.is_number());

	// If branching comparison is between a register and a value,
	// assign that register to what it must be to pass the condition.
	// TODO: this feels like it could be improved.
	if (!(state.operand_1_backing_reg != -1 && state.operand_2_backing_reg != -1))
	{
		if (state.operand_1_backing_reg != -1 && state.operand_2_backing_reg == -1)
		{
			bool seti = state.d[state.operand_1_backing_reg].is_expression() && state.d[state.operand_1_backing_reg].data & CMP_SETI;
			if ((branch_cmp & CMP_FLAGS) == CMP_GE && state.operand_2 == num_one)
				state.d[state.operand_1_backing_reg] = state.operand_1 = seti ? num(10000) : num_one;
			else if ((branch_cmp & CMP_FLAGS) == CMP_EQ && state.operand_2 == num_zero)
				state.d[state.operand_1_backing_reg] = state.operand_1 = num_zero;
			else
				infer_assign(ctx, state, state.operand_1_backing_reg,
					expr(reg(state.operand_1_backing_reg), branch_cmp, state.operand_2));
		}
		// Doing the other case hasn't seemed necessary yet.
	}

	if (expression.is_number())
		return;

	for (int i = 0; i < 8; i++)
		infer(state.d[i], expression);
	infer(state.operand_1, expression);
	infer(state.operand_2, expression);
}

static void simulate_set_value(OptContext& ctx, SimulationState& state, int reg, SimulationValue value)
{
	for (auto& v : state.stack)
	{
		if (v == reg(reg))
			v = {ValueType::Unknown};
	}
	for (int i = 0; i < 8; i++)
	{
		if (state.d[i] == reg(reg) && i != reg)
			state.d[i] = {ValueType::Unknown};
	}

	if (!(reg >= D(0) && reg < D(8)))
	{
		state.side_effects = true;
		return;
	}

	state.d[reg] = value;
}

// Simulates execution of a single instruction, updating values for each register
// and the comparison result.
static void simulate(OptContext& ctx, SimulationState& state)
{
	#define IS_GENERIC_REG(x) (x >= D(0) && x < D(8))

	int command = C(state.pc).command;
	int arg1 = C(state.pc).arg1;
	int arg2 = C(state.pc).arg2;

	// Function calls invalidate all registers.
	if (command == CALLFUNC)
	{
		for (int i = 0; i < 8; i++)
			state.d[i] = {ValueType::Unknown};
		for (auto& v : state.stack)
		{
			if (!v.is_number())
				v = {ValueType::Unknown};
		}
		return;
	}

	switch (command)
	{
		case COMPARER:
			if (arg1 == SWITCHKEY || arg2 == SWITCHKEY)
			{
				state.bail = true;
				return;
			}
			// Lost Isle is an example of a quest with handwritten ZASM that uses
			// z-registers directly in compare statements, instead of via a generic
			// register.
			if (!IS_GENERIC_REG(arg1) || !IS_GENERIC_REG(arg2))
			{
				state.bail = true;
				return;
			}
			ASSERT(arg1 >= D(0) && arg1 < D(8));
			ASSERT(arg2 >= D(0) && arg2 < D(8));
			state.operand_1 = state.d[arg1];
			state.operand_2 = state.d[arg2];
			state.operand_1_backing_reg = arg1;
			state.operand_2_backing_reg = arg2;
			return;
		case COMPAREV:
			if (arg1 == SWITCHKEY)
			{
				state.bail = true;
				return;
			}
			if (!IS_GENERIC_REG(arg1))
			{
				state.bail = true;
				return;
			}
			state.operand_1 = state.d[arg1];
			state.operand_2 = num(arg2);
			state.operand_1_backing_reg = arg1;
			state.operand_2_backing_reg = -1;
			return;
		case COMPAREV2:
			if (arg2 == SWITCHKEY)
			{
				state.bail = true;
				return;
			}
			if (!IS_GENERIC_REG(arg2))
			{
				state.bail = true;
				return;
			}
			state.operand_1 = state.d[arg2];
			state.operand_2 = num(arg1);
			state.operand_1_backing_reg = arg2;
			state.operand_2_backing_reg = -1;
			return;
	}

	if (command_writes_comparison_result(command))
	{
		int cmp = command_to_cmp(command, arg2);
		auto e = evaluate_binary_op(cmp, state.operand_1, state.operand_2);
		simulate_set_value(ctx, state, arg1, e);
		return;
	}

	if (!command_is_pure(command) && !command_is_goto(command))
	{
		state.side_effects = true;
	}

	bool command_handled = true;
	switch (command)
	{
		case PUSHR:
		{
			auto value = IS_GENERIC_REG(arg1) ? state.d[arg1] : reg(arg1);
			// Registers whose value depends on the value of other registers (like indexing into an array
			// based on D0/D1) should be represented as unknown on the stack, because their dependant
			// registers are assumed to be invalidated soon. Theoretically they could be tracked too, as they often
			// aren't modified. Would need to invalidate the stack value on a write to any dependant register.
			if (value.is_register() && has_register_dependency(value.data))
				value = {ValueType::Unknown};
			state.stack.push_back(value);
			state.side_effects = true;
			break;
		}
		case PUSHARGSR:
		{
			auto value = IS_GENERIC_REG(arg1) ? state.d[arg1] : reg(arg1);
			if (value.is_register() && has_register_dependency(value.data))
				value = {ValueType::Unknown};
			for (int i = 0; i < arg2; i++)
				state.stack.push_back(value);
			state.side_effects = true;
			break;
		}
		case PUSHV:
		{
			state.stack.push_back(num(arg1));
			state.side_effects = true;
			break;
		}
		case PUSHARGSV:
		{
			for (int i = 0; i < arg2; i++)
				state.stack.push_back(num(arg1));
			state.side_effects = true;
			break;
		}
		case POP:
		{
			state.side_effects = true;
			SimulationValue value;
			if (!state.stack.empty())
			{
				value = state.stack.back();
				state.stack.pop_back();
				if (value == reg(arg1))
					break;
				if (value.type == ValueType::Unknown)
					value = reg(arg1);
			}
			else
			{
				value = reg(arg1);
			}
			simulate_set_value(ctx, state, arg1, value);
			break;
		}
		case POPARGS:
		{
			for (int i = 0; i < arg2; i++)
			{
				SimulationValue value;
				if (!state.stack.empty())
				{
					value = state.stack.back();
					state.stack.pop_back();
					if (value == reg(arg1))
						continue;
					if (value.type == ValueType::Unknown)
						value = reg(arg1);
				}
				else
				{
					value = reg(arg1);
				}
				simulate_set_value(ctx, state, arg1, value);
			}
			state.side_effects = true;
			break;
		}
		case SETR:
		{
			auto value = IS_GENERIC_REG(arg2) ? state.d[arg2] : reg(arg2);
			simulate_set_value(ctx, state, arg1, value);
			break;
		}
		case SETV:
		{
			if (!IS_GENERIC_REG(arg1))
			{
				state.bail = true;
				return;
			}
			simulate_set_value(ctx, state, arg1, num(arg2));
			break;
		}
		case CASTBOOLF:
		{
			if (!IS_GENERIC_REG(arg1))
			{
				state.bail = true;
				return;
			}
			auto value = evaluate_binary_op(CMP_NE, state.d[arg1], num(0));
			simulate_set_value(ctx, state, arg1, value);
			break;
		}
		case CASTBOOLI:
		{
			if (!IS_GENERIC_REG(arg1))
			{
				state.bail = true;
				return;
			}
			auto value = evaluate_binary_op(CMP_NE | CMP_SETI, state.d[arg1], num(0));
			simulate_set_value(ctx, state, arg1, value);
			break;
		}

		case STRCMPR:
		case STRICMPR:
		{
			// TODO: handle.
			state.bail = true;
			return;
		}

		// TODO: handle
		// case STACKWRITEATRV:
		// case STACKWRITEATVV_IF:
		// case STACKWRITEATVV:

		default:
			command_handled = false;
	}

	for_every_command_register_arg_include_indices(C(state.pc), [&](bool read, bool write, int reg, int argn){
		if (!write)
			return;

		if (!command_handled || argn == -1)
			simulate_set_value(ctx, state, reg, reg(reg));
		if (!IS_GENERIC_REG(reg))
			state.side_effects = true;
		if (!has_implemented_register_invalidations(reg))
			state.bail = true;
	});

	return;
}

static void simulate_and_advance(OptContext& ctx, SimulationState& state)
{
	simulate(ctx, state);
	if (state.pc != state.final_pc)
		state.pc += 1;
}

static void simulate_block(OptContext& ctx, SimulationState& state)
{
	while (true)
	{
		simulate(ctx, state);
		if (ctx.debug)
		{
			fmt::println("{}: {}", state.pc, zasm_op_to_string(C(state.pc)));
			for (int i = 0; i < 8; i++)
			{
				if (!state.d[i].is_register())
					fmt::println("\tD{} = {}", i, state.d[i].to_string());
			}
			if (state.side_effects) fmt::println("\tside_effects");
			if (state.bail) fmt::println("\tbail");
		}
		if (state.pc == state.final_pc)
			break;

		state.pc += 1;
	}
}

// Take the branch at state.pc, and set simulation values accordingly.
static void simulate_infer_branch(OptContext& ctx, SimulationState& state)
{
	// Given we take the branch, infer what the other values must be.
	infer_values_given_branch(ctx, state);
	if (ctx.debug)
		fmt::println("inferred D2: {}", state.d[2].to_string());
	state.pc = C(state.pc).arg1;
	state.block = ctx.cfg.start_pc_to_block_id.at(state.pc);
	state.final_pc = get_block_final(ctx, state.block);
}

static bool simulate_block_advance(OptContext& ctx, SimulationState& state)
{
	if (!command_is_goto(C(state.pc).command))
	{
		if (E(state.block).size() == 0)
			return false;

		pc_t next_block = ctx.cfg.start_pc_to_block_id.at(state.pc + 1);
		ASSERT(E(state.block).size() == 1);
		ASSERT(E(state.block).at(0) == next_block);
		state.pc += 1;
		state.block = next_block;
		state.final_pc = get_block_final(ctx, state.block);
		return true;
	}

	if (C(state.pc).command == GOTO)
	{
		ASSERT(!ctx.structured_zasm->function_calls.contains(state.pc));
		state.pc = C(state.pc).arg1;
		pc_t next_block = ctx.cfg.start_pc_to_block_id.at(state.pc);
		state.block = next_block;
		state.final_pc = get_block_final(ctx, state.block);
		return true;
	}

	ASSERT(one_of(C(state.pc-1).command, COMPAREV));

	int command = C(state.pc).command;
	ASSERT(command_is_goto(command));
	int arg2 = C(state.pc).arg2;
	int cmp = command_to_cmp(command, arg2);
	auto e = evaluate_binary_op(cmp, state.operand_1, state.operand_2);

	// Can we determine which branch to take? If so, continue.
	if (!e.is_number())
		return false;

	if (e.data)
		state.pc = C(state.pc).arg1;
	else
		state.pc += 1;

	state.block = ctx.cfg.start_pc_to_block_id.at(state.pc);
	state.final_pc = get_block_final(ctx, state.block);
	return true;
}

static SimulationValue compile_value(const SimulationValue& value, std::vector<ffscript>& result)
{
	if (value.is_bool_cast())
	{
		result.emplace_back(CASTBOOLF, value.op1->data, 0);
		return *value.op1;
	}

	ASSERT(value.type != ValueType::Expression);
	return value;
}

// TODO: This is pretty jank. Feels too hardcoded.
static bool compile_conditional(SimulationValue& expression, std::vector<ffscript>& result)
{
	ASSERT(expression.type == ValueType::Expression);

	// Special case for comparison between registers that are first cast to boolean.
	if (expression.op1->is_bool_cast() && expression.op2->is_bool_cast())
	{
		if (expression.op1->op1->is_register() && expression.op2->op1->is_register())
		{
			result.emplace_back(COMPARER, expression.op1->op1->data, expression.op2->op1->data);
			expression.data |= CMP_BOOL;
			return true;
		}
	}

	auto lhs = compile_value(*expression.op1, result);
	auto rhs = compile_value(*expression.op2, result);

	int arg1 = lhs.data;
	int arg2 = rhs.data;
	if (lhs.is_register() && rhs.is_register())
		result.emplace_back(COMPARER, arg1, arg2);
	else if (lhs.is_register() && rhs.is_number())
		result.emplace_back(COMPAREV, arg1, arg2);
	else if (lhs.is_number() && rhs.is_register())
		result.emplace_back(COMPAREV, arg1, arg2);
	else
	{
		ASSERT(false);
		return false;
	}

	return true;
}

static std::vector<ffscript> compile_conditional(const ffscript& instr, const SimulationValue& op1, const SimulationValue& op2)
{
	int cmp = command_to_cmp(instr.command, instr.arg2);
	auto expression = evaluate_binary_op(cmp, op1, op2);

	std::vector<ffscript> result;
	if (!compile_conditional(expression, result))
		return result;

	if (command_is_goto(instr.command))
		result.emplace_back(GOTOCMP, instr.arg1, expression.data);
	else
		result.emplace_back(SETCMP, instr.arg1, expression.data);

	return result;
}

// Assigns a register to its final value directly, without intermediate assignments
// or using the stack.
// Ex:
//    SETR            D3              LINKX
//    TRACER          D3
// ->
//    TRACER LINKX
//
// Also works for pushing to stack and reading value later.
//
// However, do not propagate the register if read twice from a D-register, since there is
// significant overhead in calling `get_register`.
// Ex, do not optimize the following:
//    SETR            D3              LINKY
//    TRACER          D3
//    TRACER          D3
//
// Also do not propagate the value if there is some write on the target register (ex: LINKX)
// between setting to a D-register and using it.
static void optimize_propagate_values(OptContext& ctx)
{
	if (!should_run_experimental_passes())
		return;

	add_context_cfg(ctx);
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		SimulationState state{};
		state.set_block(ctx, block_index);

		struct propagate_candidate
		{
			int32_t* arg;
			int32_t new_arg;
		};
		std::vector<propagate_candidate> candidates;
		bool reg_reads[8] = {};
		for (int i = 0; i < 8; i++) reg_reads[i] = false;

		auto flush = [&](int reg){
			bool skip = false;
			bool checked_skip = false;
			candidates.erase(
				std::remove_if(
					candidates.begin(), candidates.end(),
					[&](auto& c){
						if (*c.arg != reg && reg != -1)
							return false;

						if (!checked_skip)
						{
							skip = bisect_tool_should_skip();
							checked_skip = true;
						}
						if (!skip)
							*c.arg = c.new_arg;
						return true;
					}),
				candidates.end());
		};

		auto remove_candidates = [&](int reg){
			candidates.erase(
				std::remove_if(
					candidates.begin(), candidates.end(),
					[&](auto& c){ return *c.arg == reg;}),
				candidates.end());
		};

		while (true)
		{
			for_every_command_register_arg_include_indices(C(state.pc), [&](bool read, bool write, int reg, int argn){
				if (read && !write)
				{
					if (!(reg >= D(0) && reg < D(8)))
						return;

					if (state.d[reg] == reg(reg))
						return;

					if (!state.d[reg].is_register())
						return;

					if (reg_reads[reg])
					{
						remove_candidates(reg);
						return;
					}

					if (!has_register_dependency(state.d[reg].data))
					{
						if (argn == 0) candidates.emplace_back(propagate_candidate{&C(state.pc).arg1, state.d[reg].data});
						if (argn == 1) candidates.emplace_back(propagate_candidate{&C(state.pc).arg2, state.d[reg].data});
						if (argn == 2) candidates.emplace_back(propagate_candidate{&C(state.pc).arg3, state.d[reg].data});
					}

					reg_reads[reg] = true;
					return;
				}

				if (write && reg >= D(0) && reg < D(8))
				{
					flush(reg);
					reg_reads[reg] = false;
				}

				if (write && !has_implemented_register_invalidations(reg))
					state.bail = true;
			});
			if (state.bail)
				break;

			if (C(state.pc).command == RETURNFUNC)
			{
				if (reg_reads[D(2)])
					remove_candidates(D(2));
				flush(-1);
			}

			simulate(ctx, state);
			if (state.bail)
				break;

			int command = C(state.pc).command;
			int arg1 = C(state.pc).arg1;

			if (command == POP && arg1 >= D(0) && arg1 < D(8))
			{
				if (state.d[arg1].is_register() && state.d[arg1].data != arg1)
				{
					pc_t j = state.pc - 1;
					bool found = false;
					while (true)
					{
						if (one_of(C(j).command, PUSHR, PUSHV))
						{
							found = true;
							break;
						}

						// If reads from stack, bail.
						// TODO: is it worth keeping track of all commands that read stack? For now, use command_is_pure as standin...
						// if (one_of(C(j).command, PRINTFVARG, SPRINTFVARG))
						if (!command_is_pure(C(j).command))
						{
							break;
						}

						if (j == start_pc)
							break;
						j -= 1;
					}
					
					if (found && !bisect_tool_should_skip())
					{
						C(state.pc) = {SETR, arg1, state.d[arg1].data};
						remove(ctx, j);
					}
				}
			}

			if (command == CALLFUNC)
			{
				for (int i = 0; i < 8; i++) reg_reads[i] = false;
				flush(-1);
			}

			if (state.pc == state.final_pc)
				break;
			state.pc += 1;
		}

		if (!state.bail)
			flush(-1);
	});
}

// 1. If following a branch is guaranteed to jump to some other block given the initial
//    branch condition, rewrite the branch to jump directly to that end block. This can
//    only be done when there are no side effects. This removes all spurious branches.
// 2. Convert GOTOX to GOTOCMP
static void optimize_spurious_branches(OptContext& ctx)
{
	add_context_cfg(ctx);
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		// Only consider blocks with a conditional branch.
		// TODO: experiment with checking conditionless branches too.
		const auto& edges = ctx.cfg.block_edges.at(block_index);
		if (edges.size() != 2)
			return;

		int command = C(final_pc).command;
		if (!command_is_goto(command))
		{
			ASSERT(false);
			return;
		}

		if (command == GOTO && ctx.structured_zasm->function_calls.contains(final_pc))
		{
			ASSERT(false);
			return;
		}

		// If the first command is SETCMP, then some other block jumps to this
		// and reuses the comparison state. As that will be undefined, quit early.
		if (command_writes_comparison_result(C(start_pc).command))
			return;

		// COMPARER is left out because it doesn't seem to introduce any optimizations.
		if (!final_pc || !one_of(C(final_pc - 1).command, COMPAREV, COMPAREV2))
		{
			ASSERT(C(final_pc - 1).command == COMPARER);
			return;
		}

		if (ctx.debug)
			fmt::println("\n[spurious_branches] Block #{}\n", block_index);

		SimulationState state{};
		state.block = block_index;
		state.pc = start_pc;
		state.final_pc = final_pc;
		pc_t goto_pc = C(final_pc).arg1;

		// Simulate running the head block.
		simulate_block(ctx, state);

		if (state.bail)
			return;

		if (state.operand_1.type == ValueType::Unknown || state.operand_2.type == ValueType::Unknown)
			return;
		
		// Don't care about side effects in the head block.
		state.side_effects = false;
		simulate_infer_branch(ctx, state);

		while (true)
		{
			simulate_block(ctx, state);
			if (state.bail)
				return;
			if (state.side_effects)
				break;

			bool is_conditionless = E(state.block).size() == 1;
			if (!simulate_block_advance(ctx, state))
				break;

			// If just followed a branch, then save progress so far.
			// It's certain that jumping to this instead of the
			// original pc will not result in any observable difference.
			// Note: This only commits the progress so far when making decisions,
			//       ignoring fall-thrus or unconditional branches. Need better
			//       side effect tracking to handle that. For example case that
			//       could be improved, see "comparisons, random stuff (1)"
			if (!is_conditionless)
			{
				goto_pc = state.pc;
			}
		}

		// If this make the original target block unreachable, the final `unreachable_blocks` pass
		// will remove it.
		if (bisect_tool_should_skip())
			return;
		if (goto_pc != C(final_pc).arg1)
			ctx.cfg_stale = true;
		C(final_pc) = {GOTOCMP, (int)goto_pc, command_to_cmp(command, C(final_pc).arg2)};
		if (ctx.debug)
			fmt::println("rewrite {}: {}", final_pc, zasm_op_to_string(C(final_pc)));
	});
}

static void optimize_reduce_comparisons(OptContext& ctx)
{
	add_context_cfg(ctx);
	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		bool bail_comp_reduction = false;
		for (int j = start_pc; j < final_pc; j++)
		{
			int command = C(j).command;
			int final_command = C(final_pc).command;
			if (!one_of(command, COMPARER, COMPAREV, COMPAREV2, CASTBOOLF, CASTBOOLI)) continue;
			if (!command_is_goto(final_command)) continue;
			// if (!command_is_goto(final_command) && !one_of(final_command, GOTOR, RETURN, RETURNFUNC)) continue;
			if (final_command == GOTO) continue;

			// TODO support more than just when final command is a GOTO. Ex:
			/*
				11472: COMPARER        D3              D2           
				11473: SETLESS         D2                           
				11474: COMPAREV        D2              0            
				11475: SETTRUEI        D2                           
				11476: PUSHR           D2                           
			*/

			bool writes_comparison_result_to_d2 = false;
			pc_t k = j + 1;
			for (; k < final_pc; k++)
			{
				int command = C(k).command;
				if (!(command_uses_comparison_result(command) || one_of(command, NOP, COMPARER, COMPAREV, COMPAREV2, CASTBOOLF, CASTBOOLI)))
				{
					bail_comp_reduction = true;
					break;
				}

				for_every_command_register_arg(C(k), [&](bool read, bool write, int arg, int argn){
					if (arg == D(2) && write)
					{
						writes_comparison_result_to_d2 = true;
					}
				});
			}

			if (bail_comp_reduction)
				break;

			if (ctx.debug)
				fmt::println("\n[reduce_comparisons] Block #{}\n", block_index);

			std::vector<ffscript> expression_zasm;
			SimulationState state{};
			{
				state.block = block_index;
				state.pc = j;
				state.final_pc = final_pc;
				simulate_block(ctx, state);
				if (state.bail || state.side_effects)
					break;

				expression_zasm = compile_conditional(C(state.pc), state.operand_1, state.operand_2);
				if (expression_zasm.empty())
					break;

				// If not enough room to replace instructions in this block, bail.
				if (expression_zasm.size() > final_pc - start_pc + 1)
				{
					// Should never happen.
					ASSERT(false);
					break;
				}
			}

			// If the comparison operands are compared again after the branch, then reducing the comparison
			// would break the code.
			pc_t target_pc = C(final_pc).arg1;
			{
				bool target_block_reuses_comparison_operands = false;
				auto [s, e] = get_block_bounds(ctx, ctx.cfg.start_pc_to_block_id.at(target_pc));
				for (pc_t i = s; i <= e; i++)
				{
					int command = C(i).command;
					if (command_writes_comparison_result(command))
					{
						target_block_reuses_comparison_operands = true;
						break;
					}
					if (one_of(command, COMPAREV, COMPAREV2, COMPARER))
						break;
				}
				if (target_block_reuses_comparison_operands)
					continue;
			}

			// Determine if D2 is reused after the branch. If so, and the original code
			// sets the comparison result to D2, we need continue setting it (we are removing all but the GOTOCMP).
			// Note: big assumption here: that only the branch might use D2. Not checking if the fall through block
			//       might use D2.

			bool target_block_uses_d2 = false;
			if (writes_comparison_result_to_d2)
			{
				auto [s, e] = get_block_bounds(ctx, ctx.cfg.start_pc_to_block_id.at(target_pc));
				for (pc_t i = s; i <= ctx.fn.final_pc; i++)
				{
					int command = C(i).command;

					if (command == NOP)
						continue;

					// Functions return their value by setting D2.
					if (command == RETURNFUNC)
					{
						target_block_uses_d2 = true;
						break;
					}

					// Function calls invalidate D2.
					if (command == CALLFUNC)
						break;

					bool writes_d2 = false;
					for_every_command_register_arg(C(i), [&](bool read, bool write, int arg, int argn){
						if (arg == D(2))
						{
							if (read && !writes_d2)
								target_block_uses_d2 = true;
							else if (write)
								writes_d2 = true;
						}
					});

					if (writes_d2 || target_block_uses_d2)
						break;
				}
			}

			if (bisect_tool_should_skip())
				return;

			if (target_block_uses_d2 && state.d[2].is_expression())
			{
				// TODO: wasm jit backend currently can only handle pairs of a COMPARE with a single SETX/GOTOX.
				if (is_web())
					break;

				expression_zasm.insert(expression_zasm.end() - 1, ffscript{SETCMP, D(2), state.d[2].data});
			}

			std::copy(expression_zasm.begin(), expression_zasm.end(), &C(j));
			remove(ctx, j + expression_zasm.size(), final_pc);
			ctx.cfg_stale = true;
			if (ctx.debug)
			{
				fmt::println("rewrite {}: {} -> {} commands", j, final_pc - j + 1, expression_zasm.size());
				for (int i = j; i <= final_pc; i++)
					fmt::println("{}: {}", i, zasm_op_to_string(C(i)));
			}

			// TODO: Will need to be a loop when more than just final command being GOTO is handled.
			break;
		}
	});
}

static void optimize_unreachable_blocks(OptContext& ctx)
{
	add_context_cfg(ctx);

	std::set<pc_t> seen_ids = ctx.block_unreachable;
	std::vector<pc_t> pending_ids = {0};
	while (!pending_ids.empty())
	{
		pc_t id = pending_ids.back();
		pending_ids.pop_back();
		seen_ids.insert(id);

		for (auto calls_id : ctx.cfg.block_edges.at(id))
		{
			if (!seen_ids.contains(calls_id))
				pending_ids.push_back(calls_id);
		}
	}
	for (pc_t i = 0; i < ctx.cfg.block_starts.size(); i++)
	{
		if (!seen_ids.contains((i)))
		{
			if (bisect_tool_should_skip())
				continue;

			auto [block_start, block_final] = get_block_bounds(ctx, i);
			for (pc_t k = block_start; k <= block_final; k++)
			{
				// Avoid double counting already removed instructions.
				if (C(k).command != NOP)
				{
					C(k).command = NOP;
					ctx.saved += 1;
				}
			}

			// Don't ever remove the end marker.
			if (block_final == ctx.script->size - 1)
			{
				C(block_final) = {0xFFFF};
				ctx.saved -= 1;
			}

			// Prevent needing to rebuild the CFG.
			ctx.block_unreachable.insert(i);
		}
	}
}

static void optimize_calling_mode(OptContext& ctx)
{
	if (ctx.structured_zasm->calling_mode == StructuredZasm::CALLING_MODE_CALLFUNC_RETURNFUNC)
		return;

	int return_command =
		ctx.structured_zasm->calling_mode == StructuredZasm::CALLING_MODE_GOTO_GOTOR ? GOTOR : RETURN;

	std::vector<pc_t> store_stack_pcs;
	for (pc_t i = 0; i < ctx.script->size; i++)
	{
		auto& instr = C(i);

		if (instr.command == return_command)
		{
			if (ctx.structured_zasm->calling_mode == StructuredZasm::CALLING_MODE_GOTO_GOTOR)
			{
				ASSERT(C(i - 1).command == POP && C(i - 1).arg1 == instr.arg1);
				remove(ctx, i - 1);
			}
			instr.command = RETURNFUNC;
			continue;
		}

		if ((instr.command == PUSHR || instr.command == PEEK) && instr.arg1 == rSFRAME)
		{
			store_stack_pcs.push_back(i);
			continue;
		}

		if (instr.command == SETR && instr.arg1 == rSFRAME && (instr.arg2 == SP || instr.arg2 == SP2))
		{
			store_stack_pcs.clear();
			continue;
		}

		if (instr.command != GOTO || !ctx.structured_zasm->function_calls.contains(i))
			continue;

		ASSERT(one_of(C(i + 1).command, POP, PEEK) && C(i + 1).arg1 == rSFRAME);

		pc_t store_stack_pc = store_stack_pcs.back();
		store_stack_pcs.pop_back();

		pc_t set_ret_addr = -1;
		pc_t push_ret_addr = -1;

		pc_t k = store_stack_pc + 1;
		while (C(k).command == NOP)
			k++;
		if (C(k).command == SETV && C(k + 1).command == PUSHR)
		{
			set_ret_addr = k;
			push_ret_addr = k + 1;
		}
		else if (C(k).command == PUSHV)
		{
			push_ret_addr = k;
		}

		instr.command = CALLFUNC;
		if (push_ret_addr != -1)
			remove(ctx, push_ret_addr);
		if (set_ret_addr != -1)
			remove(ctx, set_ret_addr);
	}
}

static void optimize_inline_functions(OptContext& ctx)
{
	struct InlineFunctionData
	{
		const ZasmFunction& fn;
		ffscript inline_instr;
		int internal_reg_to_type[8]; // 0 - stack index, 1 - z-register/number
		int internal_reg_to_value[8];
		bool all_uses_inlined = true;
	};

	std::map<pc_t, InlineFunctionData> functions_to_inline;
	for (const auto& fn : ctx.structured_zasm->functions)
	{
		size_t length = fn.final_pc - fn.start_pc + 1;
		if (length > 4)
			continue;

		InlineFunctionData data = {fn};
		for (int i = 0; i < 8; i++) data.internal_reg_to_type[i] = -1;
		for (int i = 0; i < 8; i++) data.internal_reg_to_value[i] = -1;

		int stack = 0;
		bool bail = true;
		bool found_instr = false;
		for (pc_t k = fn.start_pc; k <= fn.final_pc; k++)
		{
			int command = C(k).command;
			int arg1 = C(k).arg1;
			int arg2 = C(k).arg2;

			if (one_of(command, NOP, RETURNFUNC))
				continue;

			if (command == POP)
			{
				if (found_instr)
					continue;

				int reg = arg1;
				if (reg >= D(8))
				{
					bail = true;
					break;
				}

				// TODO: support more than 1 parameter.
				if (stack == 1)
				{
					bail = true;
					break;
				}

				data.internal_reg_to_value[reg] = stack++;
				data.internal_reg_to_type[reg] = 0;
				continue;
			}

			if (command == POPARGS)
			{
				bail = true;
				break;
			}

			if (command == SETR)
			{
				if (found_instr)
					continue;

				int reg = arg1;
				data.internal_reg_to_value[reg] = arg2;
				data.internal_reg_to_type[reg] = 1;
				continue;
			}

			if (found_instr)
			{
				bail = true;
				break;
			}

			bail = false;
			found_instr = true;
			C(k).copy(data.inline_instr);
		}
		if (bail)
			continue;
		if (command_is_goto(data.inline_instr.command))
			continue;
		// TODO: why does inlining a QUIT break things? It does for crucible_quest.zplay
		if (data.inline_instr.command == QUIT)
			continue;

		functions_to_inline.emplace(fn.id, data);
	}

	std::vector<pc_t> store_stack_pcs;
	for (pc_t i = 0; i < ctx.script->size; i++)
	{
		auto& instr = C(i);

		if (one_of(instr.command, PUSHR) && instr.arg1 == rSFRAME)
		{
			store_stack_pcs.push_back(i);
			continue;
		}

		if (instr.command == PUSHARGSR && instr.arg1 == rSFRAME)
		{
			for (int k = 0; k < instr.arg2; k++)
				store_stack_pcs.push_back(i);
			continue;
		}

		if (instr.command == POP && instr.arg1 == rSFRAME)
		{
			store_stack_pcs.pop_back();
			continue;
		}

		if (instr.command != CALLFUNC)
			continue;

		pc_t fn_id = ctx.structured_zasm->start_pc_to_function.at(instr.arg1);
		auto it = functions_to_inline.find(fn_id);
		if (it == functions_to_inline.end())
			continue;

		auto& data = it->second;
		if (bisect_tool_should_skip())
		{
			data.all_uses_inlined = false;
			continue;
		}

		int stack_to_external_value[8];
		for (int i = 0; i < 8; i++) stack_to_external_value[i] = -1;

		ASSERT(one_of(C(i - 1).command, PUSHR, PUSHV, PUSHARGSR, PUSHARGSV, NOP));
		stack_to_external_value[0] = C(i - 1).arg1;

		std::vector<ffscript> inlined_zasm;
		ffscript inline_instr = data.inline_instr;
		for_every_command_arg(inline_instr, [&](bool read, bool write, int& reg, int argn){
			if (read || write)
			{
				if (data.internal_reg_to_type[reg] == 0)
					reg = stack_to_external_value[data.internal_reg_to_value[reg]];
				else if (read)
				{
					inlined_zasm.emplace_back(SETR, reg, data.internal_reg_to_value[reg]);
				}
			}
		});
		inlined_zasm.push_back(inline_instr);

		pc_t store_stack_pc = store_stack_pcs.back();
		bool must_keep_store_stack = false;
		if (C(i + 1).command == PEEK)
			must_keep_store_stack = true;
		else
			store_stack_pcs.pop_back();

		pc_t hole_start_pc = i;
		if (C(i - 1).command != PUSHARGSR)
			hole_start_pc -= 1;
		pc_t hole_final_pc = i + 1;
		bool store_stack_part_of_hole =
			(store_stack_pc == hole_start_pc - 1 || store_stack_pc == hole_start_pc) && C(store_stack_pc).command != PUSHARGSR && !must_keep_store_stack;
		if (store_stack_part_of_hole)
			hole_start_pc = store_stack_pc;
		size_t hole_length = hole_final_pc - hole_start_pc + 1;
		if (inlined_zasm.size() > hole_length)
		{
			data.all_uses_inlined = false;
			continue;
		}

		std::copy(inlined_zasm.begin(), inlined_zasm.end(), &C(hole_start_pc));
		remove(ctx, hole_start_pc + inlined_zasm.size(), hole_final_pc);
		if (ctx.debug)
		{
			fmt::println("rewrite {}: {} -> {} commands", hole_start_pc, hole_length, inlined_zasm.size());
			for (int i = hole_start_pc; i <= hole_final_pc; i++)
				fmt::println("{}: {}", i, zasm_op_to_string(C(i)));
		}

		if (!must_keep_store_stack)
		{
			if (C(store_stack_pc).command == PUSHARGSR)
			{
				if (C(store_stack_pc).arg1 > 2)
					C(store_stack_pc).arg2 -= 1;
				else if (C(store_stack_pc).arg1 == 2)
					C(store_stack_pc) = {PUSHR, rSFRAME};
				else
					remove(ctx, store_stack_pc);
			}
			else if (!store_stack_part_of_hole)
				remove(ctx, store_stack_pc);
		}

		if (command_is_wait(inline_instr.command))
			ctx.cfg_stale = true;
	}

	for (const auto& [fn_id, data] : functions_to_inline)
	{
		if (data.all_uses_inlined)
			remove(ctx, data.fn.start_pc, data.fn.final_pc);
	}
}

// https://en.wikipedia.org/wiki/Data-flow_analysis
// https://www.cs.cornell.edu/courses/cs4120/2022sp/notes.html?id=livevar
// https://www.cs.cmu.edu/afs/cs/academic/class/15745-s19/www/lectures/L5-Intro-to-Dataflow.pdf
static void optimize_dead_code(OptContext& ctx)
{
	if (!should_run_experimental_passes())
		return;

	add_context_cfg(ctx);

	std::map<pc_t, std::vector<pc_t>> precede;
	for (pc_t i = 0; i < ctx.block_starts.size(); i++)
		precede[i] = {};
	for (pc_t i = 0; i < ctx.block_starts.size(); i++)
	{
		for (pc_t e : E(i))
		{
			precede.at(e).push_back(i);
		}
	}

	struct block_vars {
		uint8_t in, out, gen, kill;
		bool returns;
	};
	std::vector<block_vars> vars(ctx.block_starts.size());

	std::set<pc_t> worklist;
	for (pc_t block_index = 0; block_index < ctx.block_starts.size(); block_index++)
	{
		worklist.insert(block_index);

		uint8_t gen = 0;
		uint8_t kill = 0;
		bool returns = false;
		auto [start_pc, final_pc] = get_block_bounds(ctx, block_index);
		for (pc_t i = start_pc; i <= final_pc; i++)
		{
			int command = C(i).command;
			if (command == CALLFUNC)
			{
				kill = 0xFF;
				continue;
			}

			if (command == RETURNFUNC)
				returns = true;

			for_every_command_register_arg_d(C(i), [&](bool read, bool write, int reg){
				if (read)
				{
					if (!(kill & (1 << reg)))
						gen |= 1 << reg;
				}
				if (write)
					kill |= 1 << reg;
			});
		}

		vars.at(block_index) = {0, 0, gen, kill, returns};
	}

	while (!worklist.empty())
	{
		pc_t block_index = *worklist.begin();
		worklist.erase(worklist.begin());

		auto& [in, out, gen, kill, returns] = vars.at(block_index);

		out = 0;
		for (pc_t e : E(block_index))
			out |= vars.at(e).in;
		if (returns)
			out |= 1 << D(2);

		uint8_t old_in = in;
		in = gen | (out & ~kill);

		if (in != old_in)
		{
			for (pc_t e : precede.at(block_index))
				worklist.insert(e);
		}
	}

	optimize_by_block(ctx, [&](pc_t block_index, pc_t start_pc, pc_t final_pc){
		uint8_t out = vars.at(block_index).out;

		// fmt::print("{} in: ", block_index);
		// for (int i = 0; i < 8; i++) if (in & (1 << i)) fmt::print("D{} ", i);
		// fmt::print("out: ");
		// for (int i = 0; i < 8; i++) if (out & (1 << i)) fmt::print("D{} ", i);
		// fmt::print("\n");

		int live = out;
		pc_t i = final_pc;
		while (true)
		{
			for_every_command_register_arg_d(C(i), [&](bool read, bool write, int reg){
				if (write)
				{
					if (!(live & (1 << reg)))
					{
						// Don't remove writes that have other side effects (like modifying the stack).
						if (command_is_pure(C(i).command) && !bisect_tool_should_skip())
							remove(ctx, i);
					}
					live &= ~(1 << reg);
				}
				if (read)
					live |= 1 << reg;
			});

			if (i == start_pc)
				break;
			i--;
		}
	});
}

static std::vector<std::pair<std::string, std::function<void(OptContext&)>>> script_passes = {
	// Convert to modern function calls before anything else, so all
	// passes may assume that.
	{"calling_mode", optimize_calling_mode},
	{"goto_next_instruction", optimize_goto_next_instruction},
	{"inline_functions", optimize_inline_functions},
};

static std::vector<std::pair<std::string, std::function<void(OptContext&)>>> function_passes = {
	{"unreachable_blocks", optimize_unreachable_blocks},
	{"conseq_additive", optimize_conseq_additive},
	{"load_store", optimize_load_store},
	{"setv_pushr", optimize_setv_pushr},
	{"setr_pushr", optimize_setr_pushr},
	{"stack", optimize_stack},
	{"spurious_branches", optimize_spurious_branches},
	{"reduce_comparisons", optimize_reduce_comparisons},
	{"propagate_values", optimize_propagate_values},
	{"unreachable_blocks_2", optimize_unreachable_blocks},
	{"dead_code", optimize_dead_code},
};

static void run_pass(OptimizeResults& results, int i, OptContext& ctx, std::pair<std::string, std::function<void(OptContext&)>> pass)
{
	auto start_time = std::chrono::steady_clock::now();

	auto [name, fn] = pass;
	ctx.saved = 0;
	fn(ctx);

	auto end_time = std::chrono::steady_clock::now();
	results.passes[i].instructions_saved += ctx.saved;
	results.passes[i].elapsed += std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
}

static void optimize_function(OptimizeResults& results, StructuredZasm& structured_zasm, zasm_script* script, const ZasmFunction& fn)
{
	OptContext ctx = create_context_no_cfg(structured_zasm, script, fn);
	for (int i = 0; i < function_passes.size(); i++)
	{
		run_pass(results, script_passes.size() + i, ctx, function_passes[i]);
	}
}

static OptimizeResults create_opt_results()
{
	OptimizeResults results{};
	for (auto [pass_name, _] : script_passes)
	{
		results.passes.push_back({pass_name, 0, 0});
	}
	for (auto [pass_name, _] : function_passes)
	{
		results.passes.push_back({pass_name, 0, 0});
	}
	return results;
}

OptimizeResults zasm_optimize(zasm_script* script)
{
	OptimizeResults results = create_opt_results();

	auto start_time = std::chrono::steady_clock::now();
	auto structured_zasm = zasm_construct_structured(script);

	OptContext ctx = create_context_no_cfg(structured_zasm, script, structured_zasm.functions.front());
	for (int i = 0; i < script_passes.size(); i++)
	{
		run_pass(results, i, ctx, script_passes[i]);
	}

	// Fix bugged reads of SDDDD.
	// SDDDD reads D2, but when first introduced the compiler did not set D2. It still just happened to work,
	// but only by blind luck (see https://www.purezc.net/forums/index.php?showtopic=78604#entry1076373).
	// setv_pushr optimization 100% breaks these old scripts quests, since it doesn't bother to check
	// if the register it elides is used beyond the PUSH (or in this case, within a function call).
	// Let's find those bad POPs and fix them.
	// TODO: skip this if qst is known to be from non-bugged version.
	for (pc_t i = 4; i < script->size; i++)
	{
		if (!(script->zasm[i].command == SETR && script->zasm[i].arg1 == D(2) && script->zasm[i].arg2 == SDDDD))
			continue;

		// While this bug existed, some internal functions were made inline, which places the bad POP
		// in a different position relative to the SETR.
		bool is_func = structured_zasm.start_pc_to_function.contains(i - 4);
		pc_t pop_pc = is_func ? i - 4 : i - 3;
		if (!(script->zasm[pop_pc].command == POP && script->zasm[pop_pc].arg1 == D(6)))
			continue;

		script->zasm[pop_pc] = {POP, D(2)};
	}

	for (const auto& fn : structured_zasm.functions)
	{
		optimize_function(results, structured_zasm, script, fn);
	}

	// TODO: remove NOPs and update GOTOs.
	auto end_time = std::chrono::steady_clock::now();
	results.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	for (const auto& pass : results.passes)
	{
		results.instructions_saved += pass.instructions_saved;
	}

	script->optimized = true;

	return results;
}

void zasm_optimize_and_log(zasm_script* script)
{
	auto r = zasm_optimize(script);
	double pct = 100.0 * r.instructions_saved / script->size;
	std::string str = fmt::format("[{}] optimized script. saved {} instr ({:.1f}%), took {} ms", script->name, r.instructions_saved, pct, r.elapsed / 1000);
	al_trace("%s\n", str.c_str());
}

OptimizeResults zasm_optimize()
{
	int log_level = get_flag_int("-zasm-optimize-log").value_or(1);

	int size = 0;
	auto start_time = std::chrono::steady_clock::now();
	OptimizeResults results = create_opt_results();
	
	if (log_level >= 1)
		fmt::println("Optimizing scripts...");

	bool parallel = !get_flag_bool("-test-bisect").has_value();
	zasm_for_every_script(parallel, [&](auto script){
		if (script->optimized)
			return;

		auto r = zasm_optimize(script);

		static std::mutex mutex;
		{
			std::lock_guard<std::mutex> lock(mutex);

			size += script->size;
			double pct = 100.0 * r.instructions_saved / script->size;
			if (log_level >= 2)
				fmt::println("\t[{}] saved {} instr ({:.1f}%), took {} ms", script->name, r.instructions_saved, pct, r.elapsed / 1000);

			for (int i = 0; i < results.passes.size(); i++)
			{
				results.passes[i].instructions_saved += r.passes[i].instructions_saved;
				results.passes[i].elapsed += r.passes[i].elapsed;
			}
			results.instructions_saved += r.instructions_saved;
		}
	});

	if (size == 0)
	{
		if (log_level >= 1)
			fmt::println("\nNo scripts found.");
		return results;
	}

	auto end_time = std::chrono::steady_clock::now();
	results.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	if (log_level >= 1)
	{
		fmt::println("\nFinished optimizing scripts:");

		for (const auto& pass : results.passes)
		{
			double pct = 100.0 * pass.instructions_saved / size;
			fmt::println("\t[{}] saved {} instr ({:.1f}%), took {} ms", pass.name, pass.instructions_saved, pct, pass.elapsed / 1000);
		}

		double pct = 100.0 * results.instructions_saved / size;
		fmt::println("\t[{}] saved {} instr ({:.1f}%), took {} ms\n", "total", results.instructions_saved, pct, results.elapsed / 1000);
	}

	return results;
}

static bool _TEST(std::string& name_var, std::string name)
{
	static auto filter = get_flag_string("-test-filter");
	if (filter.has_value() && name.find(filter.value()) == name.npos)
		return false;

	name_var = name;
	return true;
}

#define TEST(name_val) if (_TEST(name, name_val))

// Simplified version of parsing ZASM.
static zasm_script zasm_from_string(std::string text)
{
	normalize_whitespace(text);

	std::vector<ffscript> instructions;
	std::vector<std::string> lines;
	util::split(text, lines, '\n');
	for (auto& line : lines)
	{
		util::trimstr(line);
		if (line.empty() || line.starts_with("Function"))
			continue;

		instructions.push_back(parse_zasm_op(line));
	}

	instructions.emplace_back(0xFFFF);
	return {std::move(instructions)};
}

// Used by test_optimize_zasm_test.py
void zasm_optimize_run_for_file(std::string path)
{
	std::string text = util::read_text_file(path);

	std::vector<std::string> lines;
	util::split(text, lines, '\n');
	int last_comment_line = 0;
	for (auto& line : lines)
	{
		if (line.starts_with((";")))
		{
			last_comment_line++;
		}
		else break;
	}

	std::string zasm;
	for (int i = last_comment_line; i < lines.size(); i++)
		zasm += lines[i] + "\n";

	auto script = zasm_from_string(zasm);

	// Just in case there is a minor roundtrip difference, resave the input.
	{
		std::ofstream out(path, std::ios::binary);
		for (int i = 0; i < last_comment_line; i++)
			out << lines[i] + "\n";
		out << zasm_to_string_clean(&script);
		out.close();
	}

	fmt::println("\noutput:");
	verbose = true;
	zasm_optimize(&script);
	fmt::println("{}", zasm_to_string_clean(&script));
}

bool zasm_optimize_test()
{
	tests_passed = true;
	zasm_script script;
	std::string name;

	TEST("evaluate_binary_op")
	{
		// Basics
		// number op number -> number
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_EQ, num_one, num_one));
		EXPECT(name, num(10000),
			evaluate_binary_op(CMP_EQ | CMP_SETI, num_one, num_one));
		EXPECT(name, num(10000),
			evaluate_binary_op(CMP_EQ | CMP_SETI, num_one, num_one));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_EQ, num_one, num(1337)));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_LT, num_one, num(1337)));
		
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_EQ, num_one, num_one));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_NE, num_one, num_one));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_LT, num_one, num_one));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_LE, num_one, num_one));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_LE, num_zero, num_one));
		
		// CMP_BOOL
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_EQ | CMP_BOOL, num_one, num_one));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_EQ | CMP_BOOL, num(100), num_one));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_EQ | CMP_BOOL, num_zero, num_one));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_EQ | CMP_BOOL, num_zero, num_zero));

		// Comparing a number with a register results in an expression
		// register op number -> expression
		// number op register -> expression
		EXPECT(name, expr(reg(1), CMP_LT, num(1337)),
			evaluate_binary_op(CMP_LT, reg(1), num(1337)));
		EXPECT(name, expr(reg(1), CMP_GT, num(1337)),
			evaluate_binary_op(CMP_LT, num(1337), reg(1)));
		EXPECT(name, expr(reg(1), CMP_GE, num(1337)),
			evaluate_binary_op(CMP_LE, num(1337), reg(1)));

		// Comparing with "Never"
		// any Never op -> num_zero
		EXPECT(name, num_zero,
			evaluate_binary_op(0, num_one, num(1337)));
		EXPECT(name, num_zero,
			evaluate_binary_op(0, reg(1), num(1337)));
		EXPECT(name, num_zero,
			evaluate_binary_op(0, {ValueType::Expression}, {ValueType::Expression}));
		EXPECT(name, num_zero,
			evaluate_binary_op(0, {ValueType::Unknown}, {ValueType::Unknown}));

		// Comparing with "Always"
		// any Always op -> num_one
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_FLAGS, num_one, num(1337)));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_FLAGS, reg(1), num(1337)));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_FLAGS, {ValueType::Expression}, {ValueType::Expression}));
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_FLAGS, {ValueType::Unknown}, {ValueType::Unknown}));
		EXPECT(name, num(10000),
			evaluate_binary_op(CMP_FLAGS | CMP_SETI, {ValueType::Unknown}, {ValueType::Unknown}));

		// Comparison with equal operands.
		auto do_identity_test_cases = [&](const SimulationValue& expected_result, int cmp) {
			auto T = [&](const SimulationValue& x){
				EXPECT(name, expected_result,
					evaluate_binary_op(cmp, x, x));
			};
			T(num(0));
			T(num(1));
			T(num(10000));
			T(reg(1));
			T(expr(reg(1), CMP_EQ, num(2)));
			auto compound_expr = SimulationValue{ValueType::Expression, CMP_EQ};
			compound_expr.op1 = std::make_shared<SimulationValue>(expr(reg(2), CMP_EQ, num(1337)));
			compound_expr.op2 = std::make_shared<SimulationValue>(expr(reg(2), CMP_EQ, num(1337)));
			T(compound_expr);
		};
		do_identity_test_cases(num_one, CMP_FLAGS);
		do_identity_test_cases(num_one, CMP_EQ);
		do_identity_test_cases(num_one, CMP_GE);
		do_identity_test_cases(num_one, CMP_LE);
		do_identity_test_cases(num_zero, CMP_NE);
		do_identity_test_cases(num_zero, CMP_GT);
		do_identity_test_cases(num_zero, CMP_LT);
		do_identity_test_cases(num_zero, 0);

		EXPECT(name, {ValueType::Unknown},
			evaluate_binary_op(CMP_EQ, {ValueType::Unknown}, {ValueType::Unknown}));
		EXPECT(name, {ValueType::Unknown},
			evaluate_binary_op(CMP_NE, {ValueType::Unknown}, {ValueType::Unknown}));

		// Comparing independent expressions
		// expr_a ? expr_b -> expr_c
		auto compound_expr = SimulationValue{ValueType::Expression, CMP_EQ};
		compound_expr.op1 = std::make_shared<SimulationValue>(expr(reg(1), CMP_EQ, num(1337)));
		compound_expr.op2 = std::make_shared<SimulationValue>(expr(reg(2), CMP_EQ, num(1337)));
		EXPECT(name, compound_expr,
			evaluate_binary_op(CMP_EQ, expr(reg(1), CMP_EQ, num(1337)), expr(reg(2), CMP_EQ, num(1337))));

		// Boolean cast.
		// int CMP_NE 0 -> boolean
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_NE, num(1337), num(0)));

		// Returns boolean when possible.
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_EQ, num_zero, num(0)));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_EQ, num_one, num(0)));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_EQ, num(10000), num(0)));

		// Negation.
		// x CMP_EQ 0 -> !x
		EXPECT(name, expr(reg(2), CMP_LT, num(1337)),
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_GE, num(1337)), num(0)));
		EXPECT(name, expr(reg(2), CMP_LT, num(1337)),
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_GE, num(1337)), num_zero));
		EXPECT(name, expr(reg(2), CMP_LT, num(1337)),
			evaluate_binary_op(CMP_NE, expr(reg(2), CMP_GE, num(1337)), num_one));

		// Basic reductions.
		// (x == y) == True -> x == y
		EXPECT(name, expr(reg(2), CMP_EQ, reg(3)),
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_EQ, reg(3)), num_one));
		EXPECT(name, expr(reg(2), CMP_EQ, reg(3)),
			evaluate_binary_op(CMP_EQ, num_one, expr(reg(2), CMP_EQ, reg(3))));
		// (x == y) == False -> x != y
		EXPECT(name, expr(reg(2), CMP_NE, reg(3)),
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_EQ, reg(3)), num_zero));
		EXPECT(name, expr(reg(2), CMP_NE, reg(3)),
			evaluate_binary_op(CMP_EQ, num_zero, expr(reg(2), CMP_EQ, reg(3))));

		// Comparing w/o CMP_SETI on expressions w/ CMP_SETI removes CMP_SETI.
		EXPECT(name, expr(reg(2), CMP_EQ, reg(3)),
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_EQ|CMP_SETI, reg(3)), num_one));

		// Weird stuff.
		// 0 <= (10 < D(2)) -> true
		EXPECT(name, num_one,
			evaluate_binary_op(CMP_LE, num(0), expr(num(10), CMP_LT, reg(2))));
		// 0 > (10 < D(2)) -> false
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_GT, num(0), expr(num(10), CMP_LT, reg(2))));
		// 0 >= (10 < D(2)) -> 10 < D(2)
		EXPECT(name, expr(num(10), CMP_LT, reg(2)),
			evaluate_binary_op(CMP_GE, num(0), expr(num(10), CMP_LT, reg(2))));
		// 0 < (10 < D(2)) -> 10 < D(2)
		EXPECT(name, expr(num(10), CMP_LT, reg(2)),
			evaluate_binary_op(CMP_LT, num(0), expr(num(10), CMP_LT, reg(2))));
		// 1 < (10 < D(2)) -> false
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_LT, num(1), expr(num(10), CMP_LT, reg(2))));
		// 10000 < (10 i< D(2)) -> 10 < D(2)
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_LT, num(10000), expr(num(10), CMP_LT | CMP_SETI, reg(2))));
		// 0 > (10 i< D(2)) -> 10 i< D(2)
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_GT, num(0), expr(num(10), CMP_LT, reg(2))));
		
		// Reduce comparison between mutually exclusive expressions.
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_GT, num(5)), expr(reg(2), CMP_LE, num(5))));
		EXPECT(name, num_zero,
			evaluate_binary_op(CMP_EQ, expr(reg(2), CMP_EQ, num(5)), expr(reg(2), CMP_NE, num(5))));
	}

	TEST("simulate")
	{
		std::vector<ffscript> s = {
			/*  0 */ {COMPAREV, D(2), 0},         // [Block 0 -> 1, 2]
			/*  1 */ {SETFALSE, D(2)},
			/*  2 */ {COMPAREV, D(3), 0},
			/*  3 */ {SETFALSE, D(3)},
			/*  4 */ {COMPARER, D(2), D(3)},
			/*  5 */ {SETFALSE, D(2)},
			/*  6 */ {COMPAREV, D(2), 0},
			/*  7 */ {GOTOTRUE, 9},

			/*  8 */ {TRACEV, 1},                 // [Block 1 -> 2]

			/*  9 */ {QUIT},                      // [Block 2 ->  ]
			/* 10 */ {0xFFFF},
		};
		script = zasm_script{0, "", std::move(s)};
		script.zasm = s;

		StructuredZasm structured_zasm = zasm_construct_structured(&script);
		OptContext ctx = create_context(structured_zasm, &script, structured_zasm.functions.at(0));
		SimulationState state{};
		state.pc = 0;
		state.final_pc = 7;

		simulate_and_advance(ctx, state);
		EXPECT(name, 1, state.pc);
		EXPECT(name, reg(2), state.operand_1);
		EXPECT(name, num(0), state.operand_2);

		simulate_and_advance(ctx, state);
		EXPECT(name, 2, state.pc);
		EXPECT(name, "Bool(D2)", state.d[2].to_string());

		simulate_and_advance(ctx, state);
		simulate_and_advance(ctx, state);
		EXPECT(name, 4, state.pc);
		EXPECT(name, "Bool(D3)", state.d[3].to_string());

		simulate_and_advance(ctx, state);
		simulate_and_advance(ctx, state);
		EXPECT(name, 6, state.pc);
		EXPECT(name, "(Bool(D2)) != (Bool(D3))", state.d[2].to_string());

		simulate_and_advance(ctx, state);
		simulate_and_advance(ctx, state);
		EXPECT(name, 7, state.pc);
		int cmp = command_to_cmp(C(state.pc).command, C(state.pc).arg1);
		EXPECT(name, "(Bool(D2)) == (Bool(D3))", evaluate_binary_op(cmp, state.operand_1, state.operand_2).to_string());
	}

	TEST("compile_conditional")
	{
		{
			auto r = compile_conditional({GOTOCMP, 0, CMP_EQ},
				reg(2), num_one);
			auto script = zasm_script(std::move(r));
			EXPECT(name, &script, {
				{COMPAREV, D(2), 1},
				{GOTOCMP, 0, CMP_EQ},
			});
		}

		{
			auto e = expr(reg(2), CMP_NE, num(0));
			auto r = compile_conditional({GOTOCMP, 0, CMP_EQ},
				e, num(1));
			auto script = zasm_script(std::move(r));
			EXPECT(name, &script, {
				{COMPAREV, D(2), 0},
				{GOTOCMP, 0, CMP_NE},
			});
		}

		{
			auto e = expr(reg(2), CMP_GT, num(10));
			auto r = compile_conditional({GOTOCMP, 0, CMP_NE},
				e, num(0));
			auto script = zasm_script(std::move(r));
			EXPECT(name, &script, {
				{COMPAREV, D(2), 10},
				{GOTOCMP, 0, CMP_GT},
			});
		}

		{
			auto e = expr(reg(2), CMP_NE, num(0));
			auto r = compile_conditional({GOTOCMP, 0, CMP_NE},
				e, num(0));
			auto script = zasm_script(std::move(r));
			EXPECT(name, &script, {
				{COMPAREV, D(2), 0},
				{GOTOCMP, 0, CMP_NE},
			});
		}

		{
			auto r = compile_conditional({GOTOCMP, 0, CMP_NE},
				boolean_cast(reg(2)), boolean_cast(reg(3)));
			auto script = zasm_script(std::move(r));
			EXPECT(name, &script, {
				{COMPARER, D(2), D(3)},
				{GOTOCMP, 0, CMP_NE|CMP_BOOL},
			});
		}
	}

	return tests_passed;
}
