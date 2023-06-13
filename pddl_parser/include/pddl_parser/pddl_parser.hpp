#pragma once

#include <string>
#include <vector>
#include <variant>
#include <unordered_set>

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
    OPERATION operation;
    std::vector<std::variant<Condition, Predicate>> conditions;
};


struct Action {
    std::string name;
    std::vector<Parameter> parameters;
    Condition precondtions;
    Condition effect;
    Condition observe;
};

// Hash function specialization for MyStruct
namespace std {
    template<>
    struct hash<Predicate> {
        std::size_t operator()(const Predicate &obj) const {
            std::size_t seed = 0;
            // Combine the hash values of the struct members
            // using the std::hash function for each type
            hash<int> intHash;
            hash<std::string> stringHash;

            seed ^= intHash(obj.parameters.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= stringHash(obj.name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}
// Hash function specialization for MyStruct
namespace std {
    template<>
    struct hash<Action> {
        std::size_t operator()(const Action &obj) const {
            std::size_t seed = 0;
            // Combine the hash values of the struct members
            // using the std::hash function for each type
            hash<int> intHash;
            hash<std::string> stringHash;

            seed ^= intHash(obj.precondtions.conditions.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= intHash(obj.precondtions.operation) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= intHash(obj.effect.conditions.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= intHash(obj.effect.operation) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= intHash(obj.observe.conditions.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= intHash(obj.observe.operation) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= stringHash(obj.name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

struct Domain {
    std::string name;
    std::unordered_set<std::string> requirements;
    std::unordered_set<std::string> types;
    std::unordered_set<Predicate, std::hash<Predicate>> predicates;
    std::vector<Action> actions;

};

// parsing functions
std::optional<Domain> parse_domain(const std::string &content);

std::optional<Predicate> parse_predicate(const std::string &content);

std::optional<Action> parse_action(const std::string &content);

std::optional<Condition> parse_condition(const std::string &content);