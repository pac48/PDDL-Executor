#include "pddl_parser_py/pddl_parser_py.hpp"
#include "pddl_parser/pddl_parser.hpp"

void init_Domain(py::module &m) {
    py::class_<Domain>(m, "Domain", R"(
  The Domain class from parser.
									     )")

            .def(py::init([]() {
                     auto domain = Domain();
                     return domain;
                 }),
                 R"(
                 Init stuff.
           )").def("__str__", [](const Domain &pred) {
                std::stringstream ss;
                ss << pred;
                return ss.str();
            }).def_readwrite("name", &Domain::name)
            .def_readwrite("predicates", &Domain::predicates)
            .def_readwrite("actions", &Domain::actions)
            .def_readwrite("requirements", &Domain::requirements)
            .def_readwrite("types", &Domain::types);
    m.def("parse_domain", &parse_domain, "parse domain from string",
          py::arg("content"));
}

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
    m.def("parse_predicate", &parse_predicate, "parse predicate from string",
          py::arg("content"));
}

void init_Parameter(py::module &m) {
    py::class_<Parameter>(m, "Parameter", R"(
  The Parameter class from parser.
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
            }).def_readwrite("name", &Parameter::name)
            .def_readwrite("type", &Parameter::type);
}

void init_Condition(py::module &m) {
    py::class_<Condition>(m, "Condition", R"(
  The Condition class from parser.
									     )")

            .def(py::init([](const std::vector<Parameter> &parameters,
                             const std::vector<Predicate> &predicates,
                             const std::vector<Condition> &conditions,
                             const OPERATION op) {
                     auto cond = Condition();
                     cond.parameters = parameters;
                     cond.predicates = predicates;
                     cond.conditions = conditions;
                     cond.op = op;
                     return cond;
                 }),
                 py::arg("parameters") = std::vector<Parameter>(),
                 py::arg("predicates") = std::vector<Predicate>(),
                 py::arg("conditions") = std::vector<Condition>(),
                 py::arg("op") = OPERATION(),
                 R"(
                 Init stuff.
           )").def("__str__", [](const Condition &param) {
                std::stringstream ss;
                ss << param;
                return ss.str();
            }).def_readwrite("parameters", &Condition::parameters)
            .def_readwrite("predicates", &Condition::predicates)
            .def_readwrite("conditions", &Condition::conditions)
            .def_readwrite("op", &Condition::op);
    m.def("parse_condition", &parse_condition, "parse condition from string",
          py::arg("content"));
}


void init_Action(py::module &m) {
    py::class_<Action>(m, "Action", R"(
  The Action class from parser.
									     )")

            .def(py::init([](const std::string &name, const std::vector<Parameter> &parameters,
                             const Condition &precondtions, const Condition &effect, const Condition &observe) {
                     auto action = Action();
                     action.name = name;
                     action.parameters = parameters;
                     action.precondtions = precondtions;
                     action.effect = effect;
                     action.observe = observe;
                     return action;
                 }),
                 py::arg("name"),
                 py::arg("parameters") = std::vector<Parameter>(),
                 py::arg("precondtions") = Condition(),
                 py::arg("effect") = Condition(),
                 py::arg("observe") = Condition(),
                 R"(
                 Init stuff.
           )").def("__str__", [](const Action &action) {
                std::stringstream ss;
                ss << action;
                return ss.str();
            }).def_readwrite("name", &Action::name)
            .def_readwrite("parameters", &Action::parameters)
            .def_readwrite("precondtions", &Action::precondtions)
            .def_readwrite("effect", &Action::effect)
            .def_readwrite("observe", &Action::observe);
    m.def("parse_action", &parse_action, "parse action from string",
          py::arg("content"));
}

void init_OPERATION(py::module &m) {
    py::enum_<OPERATION>(m, "OPERATION", R"(
  The OPERATION class from parser.
									     )")
            .value("AND", AND)
            .value("OR", OR)
            .value("NOT", NOT)
            .value("FORALL", FORALL)
            .export_values();
}

PYBIND11_MODULE(parser, m) {
    m.doc() = R"(
            PDDL parser functionalities.
            )";
    init_OPERATION(m);
    init_Condition(m);
    init_Domain(m);
    init_Action(m);
    init_Predicate(m);
    init_Parameter(m);
}