#include "sol.hpp"
int main(int argc, char* argv[]){
  if(argc < 2){
    std::cout<<"nothing to do.\n";
    return 1;
  }
  // begin loading the file here.
  sol_tokens loaded_file;
  sol_loadfile(argv[1], &loaded_file);
  #ifdef _DEBUG
    sol_debug_parsed(&loaded_file);
  #endif
  // init the thread
  sol_core main_core;
  main_core.init(&loaded_file);
  int status = sol_status::running;
  while(status==sol_status::running)
    status=main_core.step();
  if(status==sol_status::died){
    std::cout<<"DIED: "<<main_core.err<<"\n";
    return 1;
  }
  return 0;
}