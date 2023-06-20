#include "pddl_parser_py/pddl_parser_py.hpp"
#include "pddl_parser/pddl_parser.hpp"

void init_Domain(py::module &m) {
    py::class_<pddl_lib::Domain>(m, "Domain", R"(
  The Domain class from parser.
									     )")

            .def(py::init([]() {
                     auto domain = pddl_lib::Domain();
                     return domain;
                 }),
                 R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Domain &pred) {
                std::stringstream ss;
                ss << pred;
                return ss.str();
            }).def_readwrite("name", &pddl_lib::Domain::name)
            .def_readwrite("predicates", &pddl_lib::Domain::predicates)
            .def_readwrite("actions", &pddl_lib::Domain::actions)
            .def_readwrite("requirements", &pddl_lib::Domain::requirements)
            .def_readwrite("types", &pddl_lib::Domain::types);
    m.def("parse_domain", &pddl_lib::parse_domain, "parse domain from string",
          py::arg("content"));
}

void init_Predicate(py::module &m) {
    py::class_<pddl_lib::Predicate>(m, "Predicate", R"(
  The Predicate class from parser.
									     )")

            .def(py::init([](const std::string &name, const std::vector<pddl_lib::Parameter> &parameters) {
                     auto pred = pddl_lib::Predicate();
                     pred.name = name;
                     pred.parameters = parameters;
                     return pred;
                 }),
                 py::arg("name") = "default_name",
                 py::arg("parameters") = std::vector<pddl_lib::Parameter>(),
                 R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Predicate &pred) {
                std::stringstream ss;
                ss << pred;
                return ss.str();
            }).def_readwrite("name", &pddl_lib::Predicate::name)
            .def_readwrite("parameters", &pddl_lib::Predicate::parameters);
    m.def("parse_predicate", &pddl_lib::parse_predicate, "parse predicate from string",
          py::arg("content"),
          py::arg("param_to_type_map") = std::unordered_map<std::string, std::string>());
}

void init_Parameter(py::module &m) {
    py::class_<pddl_lib::Parameter>(m, "Parameter", R"(
  The Parameter class from parser.
									     )")

            .def(py::init([](const std::string &name, const std::string &type) {
                     auto param = pddl_lib::Parameter();
                     param.name = name;
                     param.type = type;
                     return param;
                 }),
                 py::arg("name"),
                 py::arg("type") = "",
                 R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Parameter &param) {
                std::stringstream ss;
                ss << param;
                return ss.str();
            }).def_readwrite("name", &pddl_lib::Parameter::name)
            .def_readwrite("type", &pddl_lib::Parameter::type);
}

void init_Condition(py::module &m) {
    py::class_<pddl_lib::Condition>(m, "Condition", R"(
  The Condition class from parser.
									     )")

            .def(py::init([](const std::vector<pddl_lib::Parameter> &parameters,
                             const std::vector<pddl_lib::Predicate> &predicates,
                             const std::vector<pddl_lib::Condition> &conditions,
                             const pddl_lib::OPERATION op) {
                     auto cond = pddl_lib::Condition();
                     cond.parameters = parameters;
                     cond.predicates = predicates;
                     cond.conditions = conditions;
                     cond.op = op;
                     return cond;
                 }),
                 py::arg("parameters") = std::vector<pddl_lib::Parameter>(),
                 py::arg("predicates") = std::vector<pddl_lib::Predicate>(),
                 py::arg("conditions") = std::vector<pddl_lib::Condition>(),
                 py::arg("op") = pddl_lib::OPERATION(),
                 R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Condition &param) {
                std::stringstream ss;
                ss << param;
                return ss.str();
            }).def_readwrite("parameters", &pddl_lib::Condition::parameters)
            .def_readwrite("predicates", &pddl_lib::Condition::predicates)
            .def_readwrite("conditions", &pddl_lib::Condition::conditions)
            .def_readwrite("op", &pddl_lib::Condition::op);
    m.def("parse_condition", &pddl_lib::parse_condition, "parse condition from string",
          py::arg("content"),
          py::arg("param_to_type_map") = std::unordered_map<std::string, std::string>());
}


void init_Action(py::module &m) {
    py::class_<pddl_lib::Action>(m, "Action", R"(
  The Action class from parser.
									     )")

            .def(py::init([](const std::string &name, const std::vector<pddl_lib::Parameter> &parameters,
                             const pddl_lib::Condition &precondtions, const pddl_lib::Condition &effect, const pddl_lib::Condition &observe) {
                     auto action = pddl_lib::Action();
                     action.name = name;
                     action.parameters = parameters;
                     action.precondtions = precondtions;
                     action.effect = effect;
                     action.observe = observe;
                     return action;
                 }),
                 py::arg("name"),
                 py::arg("parameters") = std::vector<pddl_lib::Parameter>(),
                 py::arg("precondtions") = pddl_lib::Condition(),
                 py::arg("effect") = pddl_lib::Condition(),
                 py::arg("observe") = pddl_lib::Condition(),
                 R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Action &action) {
                std::stringstream ss;
                ss << action;
                return ss.str();
            }).def_readwrite("name", &pddl_lib::Action::name)
            .def_readwrite("parameters", &pddl_lib::Action::parameters)
            .def_readwrite("precondtions", &pddl_lib::Action::precondtions)
            .def_readwrite("effect", &pddl_lib::Action::effect)
            .def_readwrite("observe", &pddl_lib::Action::observe);
    m.def("parse_action", &pddl_lib::parse_action, "parse action from string",
          py::arg("content"));
}

void init_OPERATION(py::module &m) {
    py::enum_<pddl_lib::OPERATION>(m, "OPERATION", R"(
  The OPERATION class from parser.
									     )")
            .value("AND", pddl_lib::AND)
            .value("OR", pddl_lib::OR)
            .value("NOT", pddl_lib::NOT)
            .value("FORALL", pddl_lib::FORALL)
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