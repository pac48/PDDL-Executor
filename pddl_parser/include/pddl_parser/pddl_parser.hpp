#pragma once

#include <string>
#include <vector>
#include <variant>

// parsing functions
void parse_domain(const std::string &content);

void parse_predicate(const std::string &content);

void parse_action(const std::string &content);

void parse_precondition(const std::string &content);

void parse_parameters(const std::string &content);

void parse_observe(const std::string &content);

void parse_effect(const std::string &content);

// types
enum OPERATION {
    AND, OR, FORALL, NOT
};

struct Parameter {
    std::string type;
//    std::string name;
};


struct Predicate {
    std::string name;
    std::vector<Parameter> parameters;
};

struct Condition {
    OPERATION operation;
    std::vector<std::variant<Condition, Predicate>> conditions;
};


struct Action {
    Condition precondtions;
    Condition effect;
    Condition observe;
};

struct Domain {
    std::string name;
    std::vector<std::string> requirements;
    std::vector<std::string> types;
    std::vector<Predicate> predicates;
    std::vector<Action> actions;

};
