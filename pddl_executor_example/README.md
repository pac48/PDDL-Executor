## CMake config

The first step to use the PDDL-Executor is to configure the cmake properly for your executable.
We need to specify the .pddl domain files to be used when generating the behavior tree action interface.
In this example, we will use the `domain_blocks.pddl` blocks world domain. 

```cmake
find_package(pddl2cpp REQUIRED) # find code generation package
generate_bt_header(bt_blocks_actions pddl/domain_blocks.pddl) # generate the interface header file
```
The `generate_bt_header` CMake function take two arguments. First you must specify the name of the target
to be created by CMake. This tells CMake to run the code generation step prior to building any code that 
depends on it. The second argument is a PDDL domain file to be used in the interface. 

Next, create your executable target in CMake and use `target_link_libraries` to specify the dependency. 
```cmake
add_executable(planning_controller_node src/planning_controller_node.cpp)
target_link_libraries(planning_controller_node bt_blocks_actions)
```

If the CMake is configured correctly, then you should be able to include the generated header in you executable file
using the following code.
```c++
#include "bt_blocks_actions.hpp"
```

## PDDL Concept
Implementations of PDDL often use the of a Knowledge Base (KB) to represent information. 
The KB contains a set of facts representing that describe the current state of the modeled world.
Any fact that is not contained in the KB is therefore false. A PDDL problem file is essentially 
an initialization of the KB. You get an instance of the KB using the following code. 
```c++
auto &kb = KnowledgeBase::getInstance();
```
The KB is implemented as a singleton class.
```c++
class KnowledgeBase {
public:
    static KnowledgeBase &getInstance();
    //...
}
```
The class provides a variety of functionality for working with PDDL, but the simplest way to initialize
the KB is to use the `load_kb` method.
```c++
kb.clear();
pddl_lib::Problem prob = parse_problem(problem_content).value();
kb.load_kb(prob);
```
Note that in the code snippet above, a `pddl_lib::Problem` object is created by parsing a std::string `problem_content`.  

With the KB initialized, we can run the planner. The example contains a `getPlan` helper function that runs
the planner give a domain and problem represented as a std::string.
```c++
pddl_lib::Domain domain = parse_domain(domain_content).value();
std::optional<std::string> config = getPlan(domain.str(), kb.convert_to_problem(domain));        
```
The planner output is optional because it may fail to find a solution. 

If a plan is found, we can then instantiate a behaviour tree from it.
```c++
auto factory = create_tree_factory<BlockWorldActions>();
auto tree = factory.createTreeFromText(config.value());
tree.tickRoot();
```
Note that the `create_tree_factory` take a template argument `BlockWorldActions`. 
This is a class that implements the generated interface `pddl_lib::ActionInterface`. 
The following class definition shows a toy example of `BlockWorldActions`. 
The class contains one method for each action defined in the PDDL domain file.  
Note that the sensing action simply return random observations. 

```c++
class BlockWorldActions : public pddl_lib::ActionInterface {
public:
    BT::NodeStatus blocksworld_senseON(const InstantiatedAction &action) override {
        return (rand() % 2) == 0 ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

    BT::NodeStatus blocksworld_senseCLEAR(const InstantiatedAction &action) override {
        return (rand() % 2) == 0 ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

    BT::NodeStatus blocksworld_senseONTABLE(const InstantiatedAction &action) override {
        return (rand() % 2) == 0 ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

    BT::NodeStatus blocksworld_move_b_to_b(const InstantiatedAction &action) override {
        return BT::NodeStatus::SUCCESS;
    }

    BT::NodeStatus blocksworld_move_to_t(const InstantiatedAction &action) override {
        return BT::NodeStatus::SUCCESS;
    }

    BT::NodeStatus blocksworld_move_t_to_b(const InstantiatedAction &action) override {
        return BT::NodeStatus::SUCCESS;
    }
};
```

Lastly, the `UpdatePredicates` interface is generated for you to implement. 
This time, the class defines one method for each predicate in the PDDL domain.
The return value for the function is a `pddl_lib::TRUTH_VALUE`, which has three states, `TRUE`, `FALSE`, and `UNKNOWN`.
The methods have one input argument for each parameter of the predicate in the domain.
Also, the current value `val` of the predicate is passed to the function.  

```c++
class UpdatePredicatesImpl : public UpdatePredicates {
public:

    TRUTH_VALUE clear(TRUTH_VALUE val, None b) const override {
        return val;
    }

    TRUTH_VALUE on(TRUTH_VALUE val, None b1, None b2) const override {
        return val;
    }

    TRUTH_VALUE on_table(TRUTH_VALUE val, None b) const override {
        return val;
    }

};
```

A predicate updater can be created and triggered with the following code. 
```c++
UpdatePredicatesImpl updater;
updater.update();
```

## Run
First, make sure you compile the repository in a ros2 workspace and source it.
Then, you can run the example node with the following command.

`ros2 run pddl_executor_example planning_controller_node`


