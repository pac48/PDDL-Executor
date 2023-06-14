#include "pddl_parser_py/pddl_parser_py.hpp"
#include "pddl_parser/pddl_parser.hpp"

void init_Predicate(py::module &m) {
    py::class_<Predicate>(m, "Predicate", R"(
  The Predicate class from parser.
									     )")

            .def(py::init([](const std::string &name, const std::vector<Parameter> &parameters) {
                     auto pred = Predicate();
                     pred.name = name;
                     pred.parameters = parameters;
                     return pred;
                 }),
                 py::arg("name") = "default_name",
                 py::arg("parameters") = std::vector<Parameter>(),
                 R"(
                 Init stuff.
           )").def("__str__", [](const Predicate &pred) {
                std::stringstream ss;
                ss << pred;
                return ss.str();
            }).def_readwrite("name", &Predicate::name)
            .def_readwrite("parameters", &Predicate::parameters);
}

void init_Parameter(py::module &m) {
    py::class_<Parameter>(m, "Parameter", R"(
  The Predicate class from parser.
									     )")

            .def(py::init([](const std::string &name, const std::string &type) {
                     auto param = Parameter();
                     param.name = name;
                     param.type = type;
                     return param;
                 }),
                 py::arg("name"),
                 py::arg("type") = "",
                 R"(
                 Init stuff.
           )").def("__str__", [](const Parameter &param) {
        std::stringstream ss;
        ss << param;
        return ss.str();
    });
}

PYBIND11_MODULE(parser, m) {
    m.doc() = R"(
            PDDL parser functionalities.
            )";
    init_Predicate(m);
    init_Parameter(m);
}