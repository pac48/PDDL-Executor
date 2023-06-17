#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_set>
#include <mutex>
#include <unordered_map>


// types
enum OPERATION {
    AND, OR, FORALL, NOT
};

enum CONSTRAINTS {
    ONEOF
};

struct Parameter {
    std::string name;
    std::string type;

};

struct InstantiatedParameter {
    std::string name;
    std::string type;

    bool operator==(const InstantiatedParameter &other) const {
        return name == other.name && type == other.type;
    }
};

struct Predicate {
    std::string name;
    std::vector<Parameter> parameters;
};

struct InstantiatedPredicate {
    std::string name;
    std::vector<InstantiatedParameter> parameters;

    bool operator==(const InstantiatedPredicate &other) const {
        return name == other.name && parameters == other.parameters;
    }
};

struct Condition {
    OPERATION op;
    std::vector<Condition> conditions;
    std::vector<Predicate> predicates;
    std::vector<Parameter> parameters;

};

struct InstantiatedCondition {
    OPERATION op;
    std::vector<InstantiatedCondition> conditions;
    std::vector<InstantiatedPredicate> predicates;
    std::vector<InstantiatedParameter> parameters;

};

struct InstantiatedAction {
    std::string name;
    std::vector<InstantiatedParameter> parameters;
    InstantiatedCondition precondtions;
    InstantiatedCondition effect;
    InstantiatedCondition observe;
};

struct Action {
    std::string name;
    std::vector<Parameter> parameters;
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

// Hash functions
namespace std {
    template<>
    struct hash<InstantiatedPredicate> {
        std::size_t operator()(const InstantiatedPredicate &obj) const {
            std::size_t seed = 0;
            hash<int> intHash;
            hash<std::string> stringHash;

            seed ^= intHash(obj.parameters.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= stringHash(obj.name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}


class KnownKnowledgeBase : public std::unordered_set<InstantiatedPredicate> {
public:
    void concurrent_insert(const InstantiatedPredicate &value);

    void concurrent_erase(const InstantiatedPredicate &value);

    bool concurrent_find(const InstantiatedPredicate &value);

private:
    std::mutex mutex_;

};

struct Constraint {
    CONSTRAINTS constraint;
    std::unordered_set<InstantiatedPredicate> predicates;

};

class UnknownKnowledgeBase : public std::unordered_set<InstantiatedPredicate> {
public:
    void concurrent_insert(const InstantiatedPredicate &value);

    void concurrent_erase(const InstantiatedPredicate &value);

    bool concurrent_find(const InstantiatedPredicate &value);

    std::vector<Constraint> constraints;
private:
    std::mutex mutex_;

};

class KnowledgeBase {
public:
    static KnowledgeBase &getInstance();

    std::string convert_to_problem(const Domain &domain);

    bool check_conditions(const InstantiatedCondition &condition);

    void apply_conditions(const InstantiatedCondition &condition, bool negated = false);

    void apply_constraints();

    KnownKnowledgeBase knownPredicates;
    UnknownKnowledgeBase unknownPredicates;
    std::vector<InstantiatedParameter> objects;

private:
    KnowledgeBase() {} // Private constructor to prevent direct instantiation
    ~KnowledgeBase() {} // Private destructor to prevent deletion
    KnowledgeBase(const KnowledgeBase &) = delete; // Disable copy constructor
    KnowledgeBase &operator=(const KnowledgeBase &) = delete; // Disable assignment operator
};

// condition

// parsing functions
std::optional<Domain> parse_domain(const std::string &content);

std::optional<Predicate>
parse_predicate(const std::string &content, const std::unordered_map<std::string, std::string> &param_to_type_map = {});

std::optional<Action> parse_action(const std::string &content);

std::optional<Condition>
parse_condition(const std::string &content, const std::unordered_map<std::string, std::string> &param_to_type_map = {});

InstantiatedPredicate
instantiate_predicate(const Predicate &predicate, const std::unordered_map<std::string, std::string> &param_subs);

InstantiatedAction
instantiate_action(const Action &action, const std::unordered_map<std::string, std::string> &param_subs,
                   const std::vector<InstantiatedParameter> &objects = {});

InstantiatedCondition
instantiate_condition(const Condition &condition, const std::unordered_map<std::string, std::string> &param_subs,
                      const std::vector<InstantiatedParameter> &objects = {});


// printing functions
std::ostream &operator<<(std::ostream &os, const Domain &domain);

std::ostream &operator<<(std::ostream &os, const Predicate &pred);

std::ostream &operator<<(std::ostream &os, const Condition &cond);

std::ostream &operator<<(std::ostream &os, const Action &action);

std::ostream &operator<<(std::ostream &os, const Parameter &param);

