#ifndef sol_included
#define sol_included
// include some modules.
#include <unordered_map>
#include <functional>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
// define the types here (basic types).
typedef std::string sol_string;
typedef std::vector<sol_string> sol_tokens;
// sol_parse: make the tokens more evident.
void sol_parse(sol_string s, sol_tokens *t);
void sol_debug_parsed(sol_tokens *t);
void sol_loadfile(const char *fname, sol_tokens *t);
// sol_types: enumerate the types.
enum sol_types {
  integer = 1,
  decimal,
  string
};
// sol_type: simple "dynamic" type system that can store multiple types.
struct sol_type {
  union {
    int	number_v;
    float decimal_v;
  };
  sol_string string_v;
  uint8_t type;
  void set(sol_type *t);
};
typedef struct sol_type sol_type;
// sol_label: stores the point and the code stack.
class sol_label {
  public:
  sol_tokens code_stack;
  std::unordered_map<sol_string, int> points_table;
};
// sol_state: keeps the IC (instruction counter) and local variables.
struct sol_state {
  std::unordered_map<sol_string, sol_type> local_vars;
  sol_string at_label;
  uint64_t PC = 0;
};
typedef struct sol_state sol_state;
// sol_status: enumerate the status here.
enum sol_status {
  running = 1,
  finished,
  died,
  sleeping
};
// sol_core: where the code is executed and stored.
class sol_core {
  public:
  std::unordered_map<sol_string, sol_type> global_vars;
  std::unordered_map<sol_string, sol_label> labels;
  sol_type registers[10];

  // keep the status.
  int status = 0;
  sol_string err;

  // code execution quality.
  void make_sure(bool condition, const char *we);
  void error(const char *we);

  // debugging...
  void show_labels();
  void show_registers();
  void show_variable();

  // instructions
  std::unordered_map<
    sol_string, 
    std::pair<uint8_t, std::function<void(sol_core *c, sol_tokens *t)>>
  > instr;
  void add_instr(sol_string key, std::function<void(sol_core *c, sol_tokens *t)> function, uint8_t nargs);

  // system calls.
  std::unordered_map<sol_string, std::function<void(sol_core *c)>> system_calls;
  void add_system_call(sol_string key, std::function<void(sol_core *c)> function);
  void trigger_system_call(sol_string key);

  // init function.
  void init(sol_tokens *t);

  // step: run a single instruction.
  bool eq_flag = false, gt_flag = false;
  std::vector<sol_state> call_stack;
  std::vector<sol_type> global_stack;
  sol_state current_state;
  bool inc_pc=true;
  int step();

  // get_data() & set_data() functions.
  void get_data(sol_string token, sol_type *v);
  void set_data(sol_string token, sol_type  v);

  // point_goto(), save_call(), retn() movement on the code function.
  void point_goto(sol_string token);
  void save_call(sol_string token, int incr_pc);
  void retn();
};
// sol_instructions: functions that perform sol instructions.
namespace sol_i {
  // information storage instructions.
  void sol_data(sol_core *c, sol_tokens *t);
  void sol_move(sol_core *c, sol_tokens *t);
  void sol_sysc(sol_core *c, sol_tokens *t);
  // code manipulation
  void sol_halt(sol_core *c, sol_tokens *t);
  void sol_jump(sol_core *c, sol_tokens *t);
  void sol_call(sol_core *c, sol_tokens *t);
  void sol_retn(sol_core *c, sol_tokens *t);
  // conditionals
  void sol_cmpr(sol_core *c, sol_tokens *t);
  void sol_jeq (sol_core *c, sol_tokens *t);
  void sol_jneq(sol_core *c, sol_tokens *t);
  void sol_jgt (sol_core *c, sol_tokens *t);
  void sol_jle (sol_core *c, sol_tokens *t);
  void sol_ceq (sol_core *c, sol_tokens *t);
  void sol_cneq(sol_core *c, sol_tokens *t);
  void sol_cgt (sol_core *c, sol_tokens *t);
  void sol_cle (sol_core *c, sol_tokens *t);
  // basic operations (add, incr and other)
  void sol_incr(sol_core *c, sol_tokens *t);
  void sol_decr(sol_core *c, sol_tokens *t);
  void sol_add (sol_core *c, sol_tokens *t);
  void sol_sub (sol_core *c, sol_tokens *t);
  void sol_mul (sol_core *c, sol_tokens *t);
  void sol_div (sol_core *c, sol_tokens *t);
  // stack operations (pop & push)
  void sol_push(sol_core *c, sol_tokens *t);
  void sol_pop (sol_core *c, sol_tokens *t);
  // string operations (sat, stoi, stof)
  void sol_sat (sol_core *c, sol_tokens *t);
  void sol_slen(sol_core *c, sol_tokens *t);
  void sol_stoi(sol_core *c, sol_tokens *t);
  void sol_stof(sol_core *c, sol_tokens *t);
};
// sol_default_system_calls: set the default system calls
namespace sol_default_system_calls {
  void sol_sysc_print(sol_core *c);
  void sol_sysc_input(sol_core *c);
};
#endif