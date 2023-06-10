#include <string>
#include <vector>
#include "cff_plan_solver/cff_plan_solver.hpp"
#include "ff.h"


void plan(int argc, char *const *argv) {
    char *argv_copy[argc];
    std::vector<std::string> args_string(argc);
    for (int i =0 ; i < argc; i++){
        args_string[i] = argv[i];
        argv_copy[i] = args_string[i].data();
    }
    run( argc, argv_copy);
}

int main(int argc, char *argv[] ){
    plan(argc, argv);
    int o = 0;
    plan( argc, argv);

}


