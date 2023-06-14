#pragma once

#include <string>
#include <vector>
#include <variant>

// types
enum OPERATION {
    AND, OR, FORALL, NOT
};

struct Parameter {
    std::string name;
    std::string type;

    bool operator==(const Parameter &other) const {
        return name == other.name && type == other.type;
    }
};


struct Predicate {
    std::string name;
    std::vector<Parameter> parameters;

    bool operator==(const Predicate &other) const {
        return name == other.name && parameters == other.parameters;
    }
};

struct Condition {
    OPERATION op;
    std::vector<Condition> conditions;
    std::vector<Predicate> predicates;
    std::vector<Parameter> parameters;

};

struct Action {
    std::string name;
    std::vector<Parameter> parameters;
    Condition precondtions;
    Condition effect;
    Condition observe;
};

//// Hash function specialization for MyStruct
//namespace std {
//    template<>
//    struct hash<Predicate> {
//        std::size_t operator()(const Predicate &obj) const {
//            std::size_t seed = 0;
//            // Combine the hash values of the struct members
//            // using the std::hash function for each type
//            hash<int> intHash;
//            hash<std::string> stringHash;
//
//            seed ^= intHash(obj.parameters.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= stringHash(obj.name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//
//            return seed;
//        }
//    };
//}
//// Hash function specialization for MyStruct
//namespace std {
//    template<>
//    struct hash<Action> {
//        std::size_t operator()(const Action &obj) const {
//            std::size_t seed = 0;
//            // Combine the hash values of the struct members
//            // using the std::hash function for each type
//            hash<int> intHash;
//            hash<std::string> stringHash;
//
//            seed ^= intHash(obj.precondtions.conditions.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= intHash(obj.precondtions.op) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= intHash(obj.effect.conditions.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= intHash(obj.effect.op) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= intHash(obj.observe.conditions.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= intHash(obj.observe.op) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            seed ^= stringHash(obj.name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//
//            return seed;
//        }
//    };
//}

struct Domain {
    std::string name;
    std::vector<std::string> requirements;
    std::vector<std::string> types;
    std::vector<Predicate> predicates;
    std::vector<Action> actions;

};

// parsing functions
std::optional<Domain> parse_domain(const std::string &content);

std::optional<Predicate> parse_predicate(const std::string &content);

std::optional<Action> parse_action(const std::string &content);

std::optional<Condition> parse_condition(const std::string &content);


// printing functions
std::ostream &operator<<(std::ostream &os, const Domain &domain);

std::ostream &operator<<(std::ostream &os, const Predicate &pred);

std::ostream &operator<<(std::ostream &os, const Condition &cond);

std::ostream &operator<<(std::ostream &os, const Action &action);

std::ostream &operator<<(std::ostream &os, const Parameter &param);
