#include "pddl_parser_py/pddl_parser_py.hpp"
#include "pddl_parser/pddl_parser.hpp"

void init_moveit_py(py::module &m) {
    py::class_<Predicate, std::shared_ptr<Predicate>>(m, "Predicate", R"(
  The Predicate class from parser.
									     )")

            .def(py::init([](const std::string &node_name) {

                     return std::make_shared<Predicate>();
                 }),
                 py::arg("node_name") = "default_name",
                 py::return_value_policy::take_ownership,
                 R"(
           Init stuff.
           )");
}