#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_set>
#include <mutex>
#include <unordered_map>
#include <tl_expected/expected.hpp>
#include <tl_expected/expected.hpp>

namespace pddl_lib {

    enum class OPERATION {
        AND = 0, OR, FORALL, NOT, WHEN
    };

    enum class CONSTRAINTS {
        ONEOF = 0, OR
    };

    struct Parameter  {
        std::string name;
        std::string type;

        bool operator==(const Parameter &other) const {
            return name == other.name && type == other.type;
        }
    };

    struct InstantiatedParameter  {
        std::string name;
        std::string type;

        bool operator==(const InstantiatedParameter &other) const {
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
        std::vector<InstantiatedParameter> parameters;
        std::vector<std::variant<InstantiatedCondition, InstantiatedPredicate>> conditions;
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
} //pddl_lib

// Hash functions
namespace std {
    template<>
    struct hash<pddl_lib::Predicate> {
        std::size_t operator()(const pddl_lib::Predicate &obj) const {
            hash<int> intHash;
            hash<std::string> stringHash;
            return intHash(obj.parameters.size()) ^ stringHash(obj.name);
        }
    };
    template<>
    struct hash<pddl_lib::InstantiatedPredicate> {
        std::size_t operator()(const pddl_lib::InstantiatedPredicate &obj) const {
            hash<int> intHash;
            hash<std::string> stringHash;
            return intHash(obj.parameters.size()) ^ stringHash(obj.name);
        }
    };
    template<>
    struct hash<pddl_lib::InstantiatedParameter> {
        std::size_t operator()(const pddl_lib::InstantiatedParameter &obj) const {
            hash<std::string> stringHash;
            return stringHash(obj.type) ^ stringHash(obj.name);
        }
    };
    template<>
    struct hash<pddl_lib::Parameter> {
        std::size_t operator()(const pddl_lib::Parameter &obj) const {
            hash<std::string> stringHash;
            return stringHash(obj.type) ^ stringHash(obj.name);
        }
    };
}

namespace pddl_lib {

    struct Domain {
        std::string name;
        std::vector<std::string> requirements;
        std::vector<InstantiatedParameter> constants;
        std::unordered_set<std::string> types;
        std::unordered_set<Predicate> predicates;
        std::vector<Action> actions;

        std::string str();

    };

    struct Constraint {
        CONSTRAINTS constraint{};
        InstantiatedCondition condition;
    };

    struct Problem {
        std::string name;
        std::string domain;
        std::vector<InstantiatedParameter> objects;
        std::unordered_set<InstantiatedPredicate> init;
        std::unordered_set<InstantiatedPredicate> unknowns;
        std::vector<Constraint> constraints;
        InstantiatedCondition goal;

        std::string str();

    };


    class KnownPredicates : public std::unordered_set<InstantiatedPredicate> {
    public:
        void concurrent_insert(const InstantiatedPredicate &value);

        void concurrent_erase(const InstantiatedPredicate &value);

        bool concurrent_find(const InstantiatedPredicate &value);

        void lock();

        void unlock();

    private:
        std::mutex mutex_;

    };


    class UnknownPredicates : public std::unordered_set<InstantiatedPredicate> {
    public:
        void concurrent_insert(const InstantiatedPredicate &value);

        void concurrent_erase(const InstantiatedPredicate &value);

        bool concurrent_find(const InstantiatedPredicate &value);

        void lock();

        void unlock();

        std::vector<Constraint> constraints;
    private:
        std::mutex mutex_;

    };

    class Objects : public std::unordered_set<InstantiatedParameter> {
    public:
        void concurrent_insert(const InstantiatedParameter &value);

        void concurrent_erase(const InstantiatedParameter &value);

        bool concurrent_find(const InstantiatedParameter &value);

        void lock();

        void unlock();

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

        KnownPredicates knownPredicates;
        UnknownPredicates unknownPredicates;
        Objects objects;

    private:
        KnowledgeBase() {} // Private constructor to prevent direct instantiation
        ~KnowledgeBase() {} // Private destructor to prevent deletion
        KnowledgeBase(const KnowledgeBase &) = delete; // Disable copy constructor
        KnowledgeBase &operator=(const KnowledgeBase &) = delete; // Disable assignment operator
    };

// condition

// parsing functions
    tl::expected<Domain, std::string> parse_domain(const std::string &content);

    tl::expected<Problem, std::string> parse_problem(const std::string &content);

    tl::expected<Predicate, std::string>
    parse_predicate(const std::string &content,
                    const std::unordered_map<std::string, std::string> &param_to_type_map = {});

    tl::expected<InstantiatedPredicate, std::string>
    parse_instantiated_predicate(const std::string &content,
                                 const std::unordered_map<std::string, std::string> &param_to_type_map = {});

    tl::expected<Action, std::string> parse_action(const std::string &content);

    tl::expected<Condition, std::string>
    parse_condition(const std::string &content,
                    const std::unordered_map<std::string, std::string> &param_to_type_map = {});

    tl::expected<InstantiatedCondition, std::string>
    parse_instantiated_condition(const std::string &content,
                                 const std::unordered_map<std::string, std::string> &param_to_type_map = {});

    InstantiatedPredicate
    instantiate_predicate(const Predicate &predicate, const std::unordered_map<std::string, std::string> &param_subs);

    InstantiatedAction
    instantiate_action(const Action &action, const std::unordered_map<std::string, std::string> &param_subs,
                       const std::unordered_set<InstantiatedParameter> &objects = {});

    InstantiatedCondition
    instantiate_condition(const Condition &condition, const std::unordered_map<std::string, std::string> &param_subs,
                          const std::unordered_set<InstantiatedParameter> &objects = {});

} // pddl_lib

// printing functions
std::ostream &operator<<(std::ostream &os, const pddl_lib::Domain &domain);

std::ostream &operator<<(std::ostream &os, const pddl_lib::Problem &problem);

std::ostream &operator<<(std::ostream &os, const pddl_lib::Predicate &pred);

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedPredicate &pred);

std::ostream &operator<<(std::ostream &os, const pddl_lib::Condition &cond);

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedCondition &cond);

std::ostream &operator<<(std::ostream &os, const pddl_lib::Action &action);

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedAction &action);

std::ostream &operator<<(std::ostream &os, const pddl_lib::Parameter &param);

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedParameter &param);