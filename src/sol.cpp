#include "sol.hpp"

// TODO: enable support for jp-characters and other non supported scripts...
// this would be cool and needed to some RPG games.

// sol_parse: make the tokens more evident as the interpreter
// needs very simplified data.
#ifdef _SOL_ENABLE_WINDOWS_SUPPORT
  #warning "SOL has enabled Windows compatibility."
#endif
void sol_parse(sol_string s, sol_tokens *t){
  // NOTE: this parsing thing is very simple, in fact, it is so simple it doesn't
  // have any error.
  int index=0,length=s.size();
  bool outside_string=true;
  char string_begun=0;
  sol_string acc;
  #define _o(str,list) \
    if(str.size()>0) list->push_back(str); \
    str="";
  while(index<length){
    char ch=s.at(index);
    // HACK: if you want to program on Windows(R) Notepad(TM), you can by
    // enabling the compiler flag: _SOL_ENABLE_WINDOWS_SUPPORT, this obviously
    // is a very ugly hack, but it works.
    #ifdef _SOL_ENABLE_WINDOWS_SUPPORT
    if((ch==' '||ch==0x09||ch==',')&&outside_string){
      _o(acc,t);
    }
    #else
    if((ch=='\t'||ch==' '||ch==',')&&outside_string){
      _o(acc,t);
    }
    #endif
    else if((ch==';')&&outside_string){
      _o(acc,t);
      break;
    }
    else if((ch=='"'||ch=='\'')&&outside_string){
      _o(acc,t); 
      acc.push_back(ch); string_begun=ch;
      outside_string=false;
    }
    else if(string_begun==ch&&!outside_string){
      acc.push_back(ch); _o(acc,t);
      outside_string=true; string_begun=0;
    }
    else
      acc.push_back(ch);
    index++;
  }
  _o(acc,t);
  #undef _o
}
void sol_debug_parsed(sol_tokens *t){
  for(int i=0; i<t->size(); i++)
    std::cout<<"["<<i<<"]: "<<t->at(i)<<"\n";
}
void sol_loadfile(const char *fname, sol_tokens *t){
  std::ifstream fp; fp.open(fname, std::ifstream::in);
  sol_string line;
  while(std::getline(fp, line))
    sol_parse(line, t);
  fp.close();
}
// sol_type: blah blah, "dynamic" type system.
void sol_type::set(sol_type *t){
  #ifdef _DEBUG
    std::cout<<"sol_type::set(): type = "<<(int)t->type<<"\n";
  #endif
  switch(t->type){
    case sol_types::integer:
      number_v=t->number_v;
      break;
    case sol_types::decimal:
      decimal_v=t->decimal_v;
      break;
    case sol_types::string:
      string_v=t->string_v;
      string_v.shrink_to_fit();
      break;
    default:
      break;
  };
  type=t->type;
}
// sol_core: where the code is executed and stored.
void sol_core::make_sure(bool condition, const char *we){
  if(!condition)
    sol_core::error(we);
}
void sol_core::error(const char *we){
  err=we; status=sol_status::died;
}
void sol_core::show_labels(){
  for(auto &p : labels){
    std::cout<<"(label = "<<p.first<<")\n";
    for(int i=0; i<p.second.code_stack.size(); i++)
      std::cout<<" ["<<i<<"]: "<<p.second.code_stack.at(i)<<"\n";
  }
}
void sol_core::show_registers(){
  for(int i=0; i<10; i++){
    std::cout <<"Register ["<<i<<"]: TYPE: "<<(int)registers[i].type
              <<", NUM: "<<registers[i].number_v
              <<", DEC: "<<registers[i].decimal_v
              <<", STR: "<<registers[i].string_v
              << "\n";
  }
}
void sol_core::show_variable(){
  std::cout<<"(There are "<<current_state.local_vars.size()
    <<" local variables loaded.)\n";
  #define show_vars(from) \
    for(auto &p : from) \
      std::cout<<" "<<p.first<<": "\
        <<"INT: "<<p.second.number_v \
        <<" DEC: "<<p.second.decimal_v \
        <<" STR: "<<p.second.string_v \
        <<"\n";
  show_vars(current_state.local_vars);
  std::cout<<"(There are "<<global_vars.size()<<" global variables loaded.)\n";
  show_vars(global_vars);
  #undef show_vars
}
void sol_core::add_instr(
  sol_string key, 
  std::function<void(sol_core *c, sol_tokens *t)> function, 
  uint8_t nargs
){
  instr[key]=std::make_pair(nargs, function);
}
void sol_core::init(sol_tokens *t){
  // split the code in labels, the labels holds the points and other stuff,
  // which helps the code to run.
  sol_string current_label;
  for(int i=0; i<t->size(); i++){
    sol_string s=t->at(i);
    if(s.at(s.size()-1)==':') {
      s.erase(s.size()-1, 1);
      current_label=s;
    }
    else if(s=="point") {
      make_sure(i+1<t->size(),"point expects <name>");
      sol_string point_name=t->at(i+1);
      int point_at=labels[current_label].code_stack.size();
      labels[current_label].points_table[point_name]=point_at;
      i++;
    }
    else
      labels[current_label].code_stack.push_back(s);
  }
  // begin to build the system.
  // NOTE: did it even pass the token parsing?
  if(status!=sol_status::died)
    status=sol_status::running;
  else
    return;
  // initialize the registers.
  for(int i=0; i<10; i++)
    registers[i].type=sol_types::integer, registers[i].number_v=0;
  // set the machine to the default state.
  current_state.at_label="main";
  current_state.PC=0;
  // make the table of instructions, this are all remote functions, there are
  // two main instructions table: wrapper and it's number of arguments table.
  #define define_instruction(key, f, nargs) \
    instr[key]=std::make_pair(nargs, std::function<void(sol_core *c, sol_tokens *t)>(&f));
  // begin to add the instructions...
  define_instruction("data",    sol_i::sol_data, 2);
  define_instruction("move",    sol_i::sol_move, 2);
  define_instruction("sysc",    sol_i::sol_sysc, 1);
  define_instruction("halt",    sol_i::sol_halt, 0);
  define_instruction("jump",    sol_i::sol_jump, 1);
  define_instruction("call",    sol_i::sol_call, 1);
  define_instruction("retn",    sol_i::sol_retn, 0);
  define_instruction("cmpr",    sol_i::sol_cmpr, 2);
  define_instruction("jeq",     sol_i::sol_jeq,  1);
  define_instruction("jneq",    sol_i::sol_jneq, 1);
  define_instruction("jgt",     sol_i::sol_jgt,  1);
  define_instruction("jle",     sol_i::sol_jle,  1);
  define_instruction("ceq",     sol_i::sol_ceq,  1);
  define_instruction("cneq",    sol_i::sol_cneq, 1);
  define_instruction("cgt",     sol_i::sol_cgt,  1);
  define_instruction("cle",     sol_i::sol_cle,  1);
  define_instruction("incr",    sol_i::sol_incr, 1);
  define_instruction("decr",    sol_i::sol_decr, 1);
  define_instruction("add",     sol_i::sol_add,  2);
  define_instruction("sub",     sol_i::sol_sub,  2);
  define_instruction("mul",     sol_i::sol_mul,  2);
  define_instruction("div",     sol_i::sol_div,  2);
  define_instruction("push",    sol_i::sol_push, 1);
  define_instruction("pop",     sol_i::sol_pop,  1);
  define_instruction("sat",     sol_i::sol_sat,  2);
  define_instruction("slen",    sol_i::sol_slen, 2);
  define_instruction("stoi",    sol_i::sol_stoi, 2);
  define_instruction("stof",    sol_i::sol_stof, 2);
  #undef define_instruction
  // TODO: implement a way to disable the default system calls.
  add_system_call("print", std::function<void(sol_core *c)>(&sol_default_system_calls::sol_sysc_print));
  add_system_call("input", std::function<void(sol_core *c)>(&sol_default_system_calls::sol_sysc_input));
}
void sol_core::add_system_call(sol_string key, std::function<void(sol_core *c)> function){
  system_calls[key]=function;
}
void sol_core::trigger_system_call(sol_string key){
  if(system_calls.find(key)!=system_calls.end())
    system_calls[key](this);
}
int sol_core::step(){
  if(status!=sol_status::running)
    return status;
  // get the label size and decide if is time to shutdown.
  sol_string c_label=current_state.at_label;
  size_t label_length=labels[c_label].code_stack.size();
  if(current_state.PC>=label_length){
    status=sol_status::finished;
    return status;
  }
  // get the current instruction and begin to execute.
  sol_string c_instr=labels[c_label].code_stack.at(current_state.PC);
  #ifdef _DEBUG
    std::cout<<"\n";
    for(int i=0; i<80; i++) std::cout<<"-";
    std::cout<<"\nPC="<<current_state.PC<<", Label="<<current_state.at_label<<
      ", FLAG (EQ="<<eq_flag<<", GT="<<gt_flag
      <<"), Call Stack Size: "<<call_stack.size()<<"\n";
    show_registers();     show_variable();
  #endif
  if(instr.find(c_instr)!=instr.end()){
    int nargs=instr[c_instr].first;
    make_sure(current_state.PC+(nargs+1)<=label_length, "malformed instruction");
    sol_tokens args;
    if(nargs > 0)
      for(int i=1; i<(nargs+1); i++)
        args.push_back(labels[c_label].code_stack.at(current_state.PC+i));
    #ifdef _DEBUG
      // YOUR DEBUG INTERFACE?
      std::cout<<c_instr<<"   ";
      for(int i=0; i<args.size(); i++)
        std::cout<<args.at(i)<<(i+1>=args.size() ?"":", ");
      std::cout<<"\n";    std::cin.ignore();
    #endif
    instr[c_instr].second(this, &args);
    // NOTE: should we increment the PC? this register exists to prevent the
    // PC of walking in new set labels.
    if(inc_pc)  current_state.PC+=(1+nargs);
    else        inc_pc=true;
  }
  else{
    // NOTE: instruction wasn't found.
    sol_string _e="invalid instruction: "+c_instr;
    error(_e.c_str());
  }
  return status;
}
// get_data() & set_data() functions.
bool test4number(sol_string token){
  for(int i=0; i<token.size(); i++){
    char ch=token.at(i);
    if( '0' <= ch && '9' >= ch )
      continue;
    else if ('-' == ch && i == 0 )
      continue;
    else if ('-' == ch && i != 0 )
      return false;
    else
      return false;
  }
  return true;
}
bool test4decimal(sol_string token){
  bool found_dot = false;
  for(int i=0; i<token.size(); i++){
    char ch=token.at(i);
    if( '0' <= ch && '9' >= ch )
      continue;
    else if ( '.' == ch && ! found_dot )
      found_dot = true; 
    else if ( '.' == ch && found_dot )
      return false;
    else
      return false;
  }
  // it's important to have a dot in the decimal numbers, right?
  // else, why use a decimal in first place?
  return found_dot;
}
void sol_core::get_data(sol_string token, sol_type *v){
  // NOTE: $ = local variables, @ = global variables.
  // the variables are a convenient way to store data.
  char _ft=token.at(0), _lt=token.at(token.size()-1);
  if(_ft=='$'||_ft=='@'){
    token.erase(0, 1); // remove the first token '$' or '@'
    if(_ft=='$') v->set(&current_state.local_vars[token]);
    else v->set(&global_vars[token]);
  }
  else if( _ft=='r' ){
    token.erase(0, 1); int reg=std::atoi(token.c_str());
    if(reg > 10 || reg < 0) {
      sol_string _e="invalid register: r"+token;
      return error(_e.c_str());
    }
    v->set(&registers[reg]);
  }
  else if( ((_ft=='"'||_ft=='\'')&&(_lt=='"'||_lt=='\'')) ){
    token.erase(0, 1);
    token.erase(token.size()-1, 1);
    v->type=sol_types::string; v->string_v=token;
  }
  else if (test4number(token)) {
    // HACK: for some reason, std::atoi is faster than std::stoi... And it's
    // safe to use std::atoi here cuz' we don't have to worry the 0 return 
    // as error as the token can be tested to be a number independently of the
    // return.
    v->type=sol_types::integer; v->number_v=std::atoi(token.c_str());
  }
  else if (test4decimal(token)) {
    // TODO: make own function.
    v->type=sol_types::decimal; v->decimal_v=std::stof(token);
  }
  else {
    sol_string _e="unable to get data: "+token;
    error(_e.c_str());
  }
}
void sol_core::set_data(sol_string token, sol_type  v){
  char _ft=token.at(0), _lt=token.at(token.size()-1);
  if(_ft=='$'||_ft=='@'){
    token.erase(0, 1);
    if(_ft=='$') current_state.local_vars[token].set(&v);
    else global_vars[token].set(&v);
  }
  else if( _ft == 'r' ) {
    token.erase(0, 1); int reg=std::atoi(token.c_str());
    if(reg > 10 || reg < 0) {
      sol_string _e="invalid register: r"+token;
      return error(_e.c_str());
    }
    registers[reg].set(&v);
  }
  else if( ((_ft=='"'||_ft=='\'')&&(_lt=='"'||_lt=='\'')) ){
    return;
  }
  else if(test4number(token)||test4decimal(token)){
    return;
  }
  else {
    sol_string _e="unable to set data: "+token;
    error(_e.c_str());
  }
}
// point_goto(), save_call(), retn() movement on the code function.
void sol_core::point_goto(sol_string token){
  sol_string cl=current_state.at_label;
  if(labels[cl].points_table.find(token)!=labels[cl].points_table.end()){
    current_state.PC=labels[cl].points_table[token];
    inc_pc=false;
  }
  else{
    sol_string _e="unable to jump to point: "+token;
    error(_e.c_str());
  }
}
void sol_core::save_call(sol_string token, int incr_pc){
  if(labels.find(token)!=labels.end()){
    current_state.PC+=incr_pc; 
    call_stack.push_back(current_state);
    current_state.PC=0; current_state.at_label=token;
    current_state.local_vars.erase(
      current_state.local_vars.begin(),
      current_state.local_vars.end()
    );
    inc_pc=false;
  }
  else{
    sol_string _e="unknown label: "+token;
    error(_e.c_str());
  }
}
void sol_core::retn(){
  sol_state past_state;
  if(call_stack.size()<=0)
    return error("invalid retn.");
  past_state=call_stack[call_stack.size()-1];
  call_stack.pop_back();
  current_state.PC=past_state.PC;
  current_state.at_label=past_state.at_label;
  current_state.local_vars=past_state.local_vars;
  inc_pc=false;
}
// sol_instructions: functions that perform sol instructions.
// NORMAL instruction is: instruction source, target .
void sol_i::sol_data(sol_core *c, sol_tokens *t){
  // data x, y -> defines x with value y.
  sol_type source_value;
  c->get_data(t->at(1), &source_value);
  c->set_data(t->at(0), source_value);
}
void sol_i::sol_move(sol_core *c, sol_tokens *t){
  // move x, y : y = x
  sol_type source_value; 
  c->get_data(t->at(0), &source_value);
  c->set_data(t->at(1), source_value);
}
void sol_i::sol_sysc(sol_core *c, sol_tokens *t){
  // sysc "<system call>"
  // NOTE: system calls always require a string.
  sol_type source_value; 
  c->get_data(t->at(0), &source_value);
  c->make_sure(source_value.type==sol_types::string,"sysc expects <string>");
  c->system_calls[source_value.string_v](c);
}
void sol_i::sol_halt(sol_core *c, sol_tokens *t){
  // halt
  #ifdef _DEBUG
    std::cout<<"halt\n";
  #endif
  c->status=sol_status::finished;
}
void sol_i::sol_jump(sol_core *c, sol_tokens *t){
  // jump <code point>
  // NOTE: jump instruction doesn't use get_value().
  c->point_goto(t->at(0));
}
void sol_i::sol_call(sol_core *c, sol_tokens *t){
  // call <label>
  c->save_call(t->at(0), 2);
}
void sol_i::sol_retn(sol_core *c, sol_tokens *t){
  // retn
  c->retn();
}
// conditional and branching
void sol_i::sol_cmpr(sol_core *c, sol_tokens *t){
  // cmpr source, target : source == target && source < target
  sol_type sv; c->get_data(t->at(0), &sv);
  sol_type tt; c->get_data(t->at(1), &tt);
  // TODO: optimize this stuff by making operator functions!
  if(sv.type == tt.type){
    switch(sv.type){
      case sol_types::integer:
        c->eq_flag=sv.number_v==tt.number_v;
        c->gt_flag=sv.number_v <tt.number_v;
        break;
      case sol_types::decimal:
        c->eq_flag=sv.decimal_v==tt.decimal_v;
        c->gt_flag=sv.decimal_v <tt.decimal_v;
        break;
      case sol_types::string:
        c->eq_flag=sv.string_v==tt.string_v;
        c->gt_flag=sv.string_v.size() <tt.string_v.size();
        break;
      default:
        break;
    }
  }
  else
    c->eq_flag=false, c->gt_flag=false;
}
// TODO: this functions are the same except for the '!'
// so make some code optimizations here LOL.
#define build_jump_function(function, instr, condition) \
    void sol_i::function(sol_core *c, sol_tokens *t){ \
      if(condition) \
        c->point_goto(t->at(0)); \
    }
#define build_call_function(function, instr, condition) \
    void sol_i::function(sol_core *c, sol_tokens *t){ \
      if(condition) \
        c->save_call(t->at(0), 2); \
    }
build_jump_function(sol_jeq,   "jeq",  c->eq_flag);
build_jump_function(sol_jneq,  "jneq", !c->eq_flag);
build_jump_function(sol_jgt,   "jgt",  c->gt_flag);
build_jump_function(sol_jle,   "jle",  !c->gt_flag);
build_call_function(sol_ceq,   "ceq",  c->eq_flag);
build_call_function(sol_cneq,  "cneq", !c->eq_flag);
build_call_function(sol_cgt,   "cgt",  c->gt_flag);
build_call_function(sol_cle,   "cle",  !c->gt_flag);
// basic operations
void sol_i::sol_incr(sol_core *c, sol_tokens *t){
  // incr <value>
  // TODO: this is not efficient at ALL!
  sol_type source_value; c->get_data(t->at(0), &source_value);
  if(source_value.type==sol_types::integer)
    source_value.number_v++;
  c->set_data(t->at(0), source_value);
}
void sol_i::sol_decr(sol_core *c, sol_tokens *t){
  // decr <value>
  // TODO: this is ALSO not efficient at ALL!
  sol_type source_value; c->get_data(t->at(0), &source_value);
  if(source_value.type==sol_types::integer)
    source_value.number_v--;
  c->set_data(t->at(0), source_value);
}
void sol_i::sol_add (sol_core *c, sol_tokens *t){
  sol_type source_value; c->get_data(t->at(0), &source_value);
  sol_type target_value; c->get_data(t->at(1), &target_value);
  if(target_value.type==sol_types::integer)
    target_value.number_v = target_value.number_v + source_value.number_v;
  else if(target_value.type==sol_types::decimal)
    target_value.decimal_v = target_value.decimal_v + source_value.decimal_v;
  else if(target_value.type==sol_types::string)
    target_value.string_v += source_value.string_v;
  c->set_data(t->at(1), target_value);
}
#define pattern_op(operation) \
  sol_type source_value; c->get_data(t->at(0), &source_value);\
  sol_type target_value; c->get_data(t->at(1), &target_value);\
  if(target_value.type==sol_types::integer)\
    target_value.number_v = target_value.number_v operation source_value.number_v;\
  else if(target_value.type==sol_types::decimal)\
    target_value.decimal_v = target_value.decimal_v operation source_value.decimal_v;\
  c->set_data(t->at(1), target_value);
void sol_i::sol_sub (sol_core *c, sol_tokens *t){
  pattern_op(-);
}
void sol_i::sol_mul (sol_core *c, sol_tokens *t){
  pattern_op(*);
}
#undef pattern_op
void sol_i::sol_div (sol_core *c, sol_tokens *t){
  // NOTE: division can be only with similar types and strings are ignored.
  sol_type source_value;  c->get_data(t->at(0), &source_value);
  sol_type target_value;  c->get_data(t->at(1), &target_value);
  if(source_value.type==sol_types::integer&&target_value.type==sol_types::integer){
    // HACK: prevent division with 0.
    if(source_value.number_v!=0&&target_value.number_v!=0){
      target_value.decimal_v=source_value.number_v/target_value.number_v;
      target_value.type=sol_types::decimal;
    }
    else{
      sol_string _e="attempt to divide with zero: "+t->at(0)+" / "+t->at(1);
      c->error(_e.c_str());
    }
  }
  else if(source_value.type==sol_types::decimal&&target_value.type==sol_types::decimal){
    target_value.decimal_v=source_value.decimal_v/target_value.decimal_v;
  }
  else{
    sol_string _e="attempt to division with: "+t->at(0)+" / "+t->at(1);
    c->error(_e.c_str());
  }
  c->set_data(t->at(1), target_value);
}
// stack operations (pop & push)
void sol_i::sol_push(sol_core *c, sol_tokens *t){
  sol_type source_value; c->get_data(t->at(0), &source_value);
  c->global_stack.push_back(source_value);
}
void sol_i::sol_pop (sol_core *c, sol_tokens *t){
  sol_type source_value;
  if(c->global_stack.size() > 0){
    source_value.set(&c->global_stack[c->global_stack.size()-1]);
    c->set_data(t->at(0), source_value);
    c->global_stack.pop_back();
  }
  else{
    c->error("stack reached 0.");
  }
}
// string operations (sat, stoi, stof)
void sol_i::sol_sat (sol_core *c, sol_tokens *t){
  // R0 = register indexing
  sol_type source_value; c->get_data(t->at(0), &source_value);
  sol_type target_value; 
  int __index=c->registers[0].number_v;
  if(source_value.type==sol_types::string){
    if(__index<source_value.string_v.size()){
      sol_string ch; ch = source_value.string_v.at(__index);
      target_value.type=sol_types::string; target_value.string_v=ch;
    }
    else{
      sol_string _e="invalid string position: ";_e+=__index;
      c->error(_e.c_str());
    }
  }
  c->set_data(t->at(1),target_value);
}
void sol_i::sol_slen(sol_core *c, sol_tokens *t){
  sol_type source_value; c->get_data(t->at(0), &source_value);
  if(source_value.type==sol_types::string){
    sol_type target_value;  target_value.type=sol_types::integer; 
    target_value.number_v=source_value.string_v.size();
    c->set_data(t->at(1), target_value);
  }
}
// TODO: make a builder for this instructions.
void sol_i::sol_stoi(sol_core *c, sol_tokens *t){
  sol_type source_value; c->get_data(t->at(0), &source_value);
  sol_type target_value;
  if(source_value.type==sol_types::string&&test4number(source_value.string_v)){
    target_value.type=sol_types::integer;
    target_value.number_v=std::atoi(source_value.string_v.c_str());
  } 
}
void sol_i::sol_stof(sol_core *c, sol_tokens *t){
  sol_type source_value; c->get_data(t->at(0), &source_value);
  sol_type target_value;
  if(source_value.type==sol_types::string&&test4decimal(source_value.string_v)){
    target_value.type=sol_types::decimal;
    target_value.decimal_v=std::stof(source_value.string_v);
  }
}
// sol_default_system_calls: set the default system calls
void sol_default_system_calls::sol_sysc_input(sol_core *c){
  c->registers[0].type=sol_types::string;
  std::cin>>c->registers[0].string_v;
}
void sol_default_system_calls::sol_sysc_print(sol_core *c){
  // prints whatever is on the R0 register, check if the R2 register
  // is 1 (this enables the '\n' on the end).
  switch(c->registers[0].type){
    case sol_types::integer:
      std::cout<<c->registers[0].number_v<<(c->registers[2].number_v==1?"\n":"");
      break;
    case sol_types::decimal:
      std::cout<<c->registers[0].decimal_v<<(c->registers[2].number_v==1?"\n":"");
      break;
    case sol_types::string:
      std::cout<<c->registers[0].string_v<<(c->registers[2].number_v==1?"\n":"");
      break;
    default:
      break;
  };
}