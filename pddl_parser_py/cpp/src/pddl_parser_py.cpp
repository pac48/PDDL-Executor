#include "pddl_parser_py/pddl_parser_py.hpp"
#include "pddl_parser/pddl_parser.hpp"

namespace pddl_lib {
    std::optional<Domain> parse_domain_py(const std::string &content) {
        if (auto domain = parse_domain(content)) {
            return {domain.value()};
        }
        return {};
    }

    std::optional<Problem> parse_problem_py(const std::string &content) {
        if (auto problem = parse_problem(content)) {
            return {problem.value()};
        }
        return {};
    }

    std::optional<Predicate> parse_predicate_py(const std::string &content) {
        if (auto pred = parse_predicate(content)) {
            return {pred.value()};
        }
        return {};
    }

    std::optional<InstantiatedPredicate> parse_instantiated_predicate_py(const std::string &content) {
        if (auto pred = parse_instantiated_predicate(content)) {
            return {pred.value()};
        }
        return {};
    }

    std::optional<Condition> parse_condition_py(const std::string &content) {
        if (auto cond = parse_condition(content)) {
            return {cond.value()};
        }
        return {};
    }


    std::optional<Action> parse_action_py(const std::string &content) {
        if (auto action = parse_action(content)) {
            return {action.value()};
        }
        return {};
    }


    std::optional<InstantiatedPredicate>
    instantiate_predicate_py(const Predicate &pred, const std::unordered_map<std::string, std::string> &param_subs) {
        return instantiate_predicate(pred, param_subs);
    }

    std::optional<InstantiatedAction>
    instantiate_action_py(const Action &action, const std::unordered_map<std::string, std::string> &param_subs,
                          std::unordered_set<InstantiatedParameter> &objects) {
        return instantiate_action(action, param_subs, objects);
    }

    void init_Constraint(py::module &m) {
        py::class_<pddl_lib::Constraint>(m, "Constraint", R"(
  The Constraint class from parser.
									     )")

                .def(py::init([]() {
                         auto constraint = pddl_lib::Constraint();
                         return constraint;
                     }),
                     R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Constraint &con) {
                    return "TODO";
                }).def_readwrite("constraint", &pddl_lib::Constraint::constraint)
                .def_readwrite("condition", &pddl_lib::Constraint::condition);
    }

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
        m.def("parse_domain", &pddl_lib::parse_domain_py, "parse domain from string",
              py::arg("content"));
    }

    void init_Problem(py::module &m) {
        py::class_<pddl_lib::Problem>(m, "Problem", R"(
  The Problem class from parser.
									     )")

                .def(py::init([]() {
                         auto problem = pddl_lib::Problem();
                         return problem;
                     }),
                     R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::Problem &prob) {
                    std::stringstream ss;
                    ss << prob;
                    return ss.str();
                }).def_readwrite("name", &pddl_lib::Problem::name)
                .def_readwrite("domain", &pddl_lib::Problem::domain)
                .def_readwrite("objects", &pddl_lib::Problem::objects)
                .def_readwrite("init", &pddl_lib::Problem::init)
                .def_readwrite("unknowns", &pddl_lib::Problem::unknowns)
                .def_readwrite("constraints", &pddl_lib::Problem::constraints)
                .def_readwrite("goal", &pddl_lib::Problem::goal);
        m.def("parse_problem", &pddl_lib::parse_problem_py, "parse problem from string",
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
        m.def("parse_predicate", &pddl_lib::parse_predicate_py, "parse predicate from string",
              py::arg("content"));
    }

    void init_InstantiatedPredicate(py::module &m) {
        py::class_<pddl_lib::InstantiatedPredicate>(m, "InstantiatedPredicate", R"(
  The InstantiatedPredicate class from parser.
									     )")

                .def(py::init(
                             [](const std::string &name, const std::vector<pddl_lib::InstantiatedParameter> &parameters) {
                                 auto pred = pddl_lib::InstantiatedPredicate();
                                 pred.name = name;
                                 pred.parameters = parameters;
                                 return pred;
                             }),
                     py::arg("name") = "default_name",
                     py::arg("parameters") = std::vector<pddl_lib::Parameter>(),
                     R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::InstantiatedPredicate &pred) {
                    std::stringstream ss;
                    ss << pred;
                    return ss.str();
                }).def_readwrite("name", &pddl_lib::InstantiatedPredicate::name)
                .def_readwrite("parameters", &pddl_lib::InstantiatedPredicate::parameters);
        m.def("parse_instantiated_predicate", &pddl_lib::parse_instantiated_predicate_py,
              "parse InstantiatedPredicate from string",
              py::arg("content"));
        m.def("instantiate_predicate", &pddl_lib::instantiate_predicate_py,
              "instantiate_predicate from predicate and map",
              py::arg("predicate"),
              py::arg("param_subs")
        );
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

    void init_InstantiatedParameter(py::module &m) {
        py::class_<pddl_lib::InstantiatedParameter>(m, "InstantiatedParameter", R"(
  The InstantiatedParameter class from parser.
									     )")

                .def(py::init([](const std::string &name, const std::string &type) {
                         auto param = pddl_lib::InstantiatedParameter();
                         param.name = name;
                         param.type = type;
                         return param;
                     }),
                     py::arg("name"),
                     py::arg("type") = "",
                     R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::InstantiatedParameter &param) {
                    std::stringstream ss;
                    ss << param;
                    return ss.str();
                }).def_readwrite("name", &pddl_lib::InstantiatedParameter::name)
                .def_readwrite("type", &pddl_lib::InstantiatedParameter::type);
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
        m.def("parse_condition", &pddl_lib::parse_condition_py, "parse condition from string",
              py::arg("content"));
    }

    void init_InstantiatedCondition(py::module &m) {
        py::class_<pddl_lib::InstantiatedCondition>(m, "InstantiatedCondition", R"(
  The InstantiatedCondition class from parser.
									     )")

                .def(py::init([](const std::vector<pddl_lib::InstantiatedParameter> &parameters,
                                 const std::vector<pddl_lib::InstantiatedPredicate> &predicates,
                                 const std::vector<pddl_lib::InstantiatedCondition> &conditions,
                                 const pddl_lib::OPERATION op) {
                         auto cond = pddl_lib::InstantiatedCondition();
                         cond.parameters = parameters;
                         cond.predicates = predicates;
                         cond.conditions = conditions;
                         cond.op = op;
                         return cond;
                     }),
                     py::arg("parameters") = std::vector<pddl_lib::InstantiatedParameter>(),
                     py::arg("predicates") = std::vector<pddl_lib::InstantiatedPredicate>(),
                     py::arg("conditions") = std::vector<pddl_lib::InstantiatedCondition>(),
                     py::arg("op") = pddl_lib::OPERATION(),
                     R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::InstantiatedCondition &param) {
                    std::stringstream ss;
                    ss << param;
                    return ss.str();
                }).def_readwrite("parameters", &pddl_lib::InstantiatedCondition::parameters)
                .def_readwrite("predicates", &pddl_lib::InstantiatedCondition::predicates)
                .def_readwrite("conditions", &pddl_lib::InstantiatedCondition::conditions)
                .def_readwrite("op", &pddl_lib::InstantiatedCondition::op);
    }


    void init_Action(py::module &m) {
        py::class_<pddl_lib::Action>(m, "Action", R"(
  The Action class from parser.
									     )")

                .def(py::init([](const std::string &name, const std::vector<pddl_lib::Parameter> &parameters,
                                 const pddl_lib::Condition &precondtions, const pddl_lib::Condition &effect,
                                 const pddl_lib::Condition &observe) {
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
        m.def("parse_action", &pddl_lib::parse_action_py, "parse action from string",
              py::arg("content"));
    }

    void init_InstantiatedAction(py::module &m) {
        py::class_<pddl_lib::InstantiatedAction>(m, "InstantiatedAction", R"(
  The InstantiatedAction class from parser.
									     )")

                .def(py::init(
                             [](const std::string &name, const std::vector<pddl_lib::InstantiatedParameter> &parameters,
                                const pddl_lib::InstantiatedCondition &precondtions,
                                const pddl_lib::InstantiatedCondition &effect,
                                const pddl_lib::InstantiatedCondition &observe) {
                                 auto action = pddl_lib::InstantiatedAction();
                                 action.name = name;
                                 action.parameters = parameters;
                                 action.precondtions = precondtions;
                                 action.effect = effect;
                                 action.observe = observe;
                                 return action;
                             }),
                     py::arg("name"),
                     py::arg("parameters") = std::vector<pddl_lib::InstantiatedParameter>(),
                     py::arg("precondtion") = pddl_lib::InstantiatedCondition(),
                     py::arg("effect") = pddl_lib::InstantiatedCondition(),
                     py::arg("observe") = pddl_lib::InstantiatedCondition(),
                     R"(
                 Init stuff.
           )").def("__str__", [](const pddl_lib::InstantiatedAction &action) {
                    std::stringstream ss;
                    ss << action;
                    return ss.str();
                }).def_readwrite("name", &pddl_lib::InstantiatedAction::name)
                .def_readwrite("parameters", &pddl_lib::InstantiatedAction::parameters)
                .def_readwrite("precondtion", &pddl_lib::InstantiatedAction::precondtions)
                .def_readwrite("effect", &pddl_lib::InstantiatedAction::effect)
                .def_readwrite("observe", &pddl_lib::InstantiatedAction::observe);
        m.def("instantiate_action", &pddl_lib::instantiate_action_py,
              "instantiated action",
              py::arg("action"),
              py::arg("param_subs"),
              py::arg("objects"));
    }

    void init_OPERATION(py::module &m) {
        py::enum_<pddl_lib::OPERATION>(m, "OPERATION", R"(
  The OPERATION class from parser.
									     )")
                .value("AND", pddl_lib::AND)
                .value("OR", pddl_lib::OR)
                .value("NOT", pddl_lib::NOT)
                .value("FORALL", pddl_lib::FORALL)
                .value("WHEN", pddl_lib::WHEN)
                .export_values();
    }

    void init_CONSTRAINTS(py::module &m) {
        py::enum_<pddl_lib::CONSTRAINTS>(m, "CONSTRAINTS", R"(
  The CONSTRAINTS class from parser.
									     )")
                .value("ONEOF", pddl_lib::ONEOF)
                .value("OR", pddl_lib::OR_CONSTRAINT)
                .export_values();
    }


    PYBIND11_MODULE(parser, m) {
        m.doc() = R"(
            PDDL parser functionalities.
            )";
        init_OPERATION(m);
        init_CONSTRAINTS(m);
        init_Condition(m);
        init_InstantiatedCondition(m);
        init_Domain(m);
        init_Constraint(m);
        init_Problem(m);
        init_Action(m);
        init_InstantiatedAction(m);
        init_Predicate(m);
        init_InstantiatedPredicate(m);
        init_Parameter(m);
        init_InstantiatedParameter(m);
    }
}