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
#include <cassert>

#include "functional"

namespace pddl_lib {

    enum class TRUTH_VALUE {
        FALSE, TRUE, UNKNOWN
    };

    inline TRUTH_VALUE operator&=(TRUTH_VALUE lhs, TRUTH_VALUE rhs) {
        if (lhs == TRUTH_VALUE::FALSE || rhs == TRUTH_VALUE::FALSE) {
            return TRUTH_VALUE::FALSE;
        }
        if (lhs == TRUTH_VALUE::UNKNOWN || rhs == TRUTH_VALUE::UNKNOWN) {
            return TRUTH_VALUE::UNKNOWN;
        }
        return TRUTH_VALUE::TRUE;
    }

    inline TRUTH_VALUE operator|=(TRUTH_VALUE lhs, TRUTH_VALUE rhs) {
        if (lhs == TRUTH_VALUE::TRUE || rhs == TRUTH_VALUE::TRUE) {
            return TRUTH_VALUE::TRUE;
        }
        if (lhs == TRUTH_VALUE::UNKNOWN || rhs == TRUTH_VALUE::UNKNOWN) {
            return TRUTH_VALUE::UNKNOWN;
        }
        return TRUTH_VALUE::FALSE;
    }

    inline TRUTH_VALUE operator!(TRUTH_VALUE val) {
        if (val == TRUTH_VALUE::TRUE) {
            return TRUTH_VALUE::FALSE;
        }
        if (val == TRUTH_VALUE::FALSE) {
            return TRUTH_VALUE::TRUE;
        }
        return TRUTH_VALUE::UNKNOWN;
    }

    enum class OPERATION {
        AND = 0, OR, FORALL, NOT, WHEN
    };

    enum class CONSTRAINTS {
        ONEOF = 0, OR
    };

    struct Parameter {
        std::string name;
        std::string type;

        bool operator==(const Parameter &other) const {
            return name == other.name && type == other.type;
        }
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
        std::vector<std::variant<Condition, Predicate>> conditions;
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
        std::vector<std::string> comments;

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
        std::vector<std::string> comments;

        std::string str();

    };


//    class KnownPredicates : public std::unordered_set<InstantiatedPredicate> {
//    public:
//        void concurrent_insert(const InstantiatedPredicate &value);
//
//        void concurrent_erase(const InstantiatedPredicate &value);
//
//        bool concurrent_find(const InstantiatedPredicate &value);
//
//        void lock();
//
//        void unlock();
//
//    private:
//        std::mutex mutex_;
//
//    };
//
//
//    class UnknownPredicates : public std::unordered_set<InstantiatedPredicate> {
//    public:
//        void concurrent_insert(const InstantiatedPredicate &value);
//
//        void concurrent_erase(const InstantiatedPredicate &value);
//
//        bool concurrent_find(const InstantiatedPredicate &value);
//
//        void lock();
//
//        void unlock();
//
//        std::vector<Constraint> constraints;
//    private:
//        std::mutex mutex_;
//
//    };
//
//    class Objects : public std::unordered_set<InstantiatedParameter> {
//    public:
//        void concurrent_insert(const InstantiatedParameter &value);
//
//        void concurrent_erase(const InstantiatedParameter &value);
//
//        bool concurrent_find(const InstantiatedParameter &value);
//
//        void lock();
//
//        void unlock();
//
//    private:
//        std::mutex mutex_;
//
//    };

//    template<typename T>
//    class ConcurrentSet : public std::unordered_set<T> {
//    public:
//        void concurrent_insert(const T &value);
//
//        void concurrent_erase(const T &value);
//
//        bool concurrent_find(const T &value)  const;
//
//        void concurrent_clear();
//
//        void lock() {
//            mutex_.lock();
//        }
//
//        void unlock() {
//            mutex_.unlock();
//        }
//
//    private:
//        std::mutex mutex_;
//
//    };

    class KnowledgeBase {
    public:
        static KnowledgeBase &getInstance();

        std::string convert_to_problem(const Domain &domain);

        TRUTH_VALUE check_conditions(const InstantiatedCondition &condition);

        void load_kb(const Problem &problem);

        void clear();

        void clear_unknowns();

        void apply_conditions(const InstantiatedCondition &condition, bool negated = false);

        void apply_constraints();

        std::unordered_set<InstantiatedParameter> get_objects();

        std::unordered_set<InstantiatedPredicate> get_known_predicates();

        void insert_object(const InstantiatedParameter &obj);

        void erase_object(const InstantiatedParameter &obj);

        bool find_object(const InstantiatedParameter &obj);

        void insert_predicate(const InstantiatedPredicate &pred);

        void erase_predicate(const InstantiatedPredicate &pred);

        void set_goal(const InstantiatedCondition &goal);

        bool find_predicate(const InstantiatedPredicate &pred);

        void insert_unknown_predicate(const InstantiatedPredicate &pred);

        void erase_unknown_predicate(const InstantiatedPredicate &pred);

        bool find_unknown_predicate(const InstantiatedPredicate &pred);

        void insert_constraint(const Constraint &constraint);

        void print_predicate();


    private:
        KnowledgeBase() {} // Private constructor to prevent direct instantiation
        ~KnowledgeBase() {} // Private destructor to prevent deletion
        KnowledgeBase(const KnowledgeBase &) = delete; // Disable copy constructor
        KnowledgeBase &operator=(const KnowledgeBase &) = delete; // Disable assignment operator

        TRUTH_VALUE check_variant_internal(const std::variant<InstantiatedCondition, InstantiatedPredicate> &condition);

        void apply_conditions_internal(const InstantiatedCondition &condition, bool negated);

        void
        apply_variant_internal(const std::variant<InstantiatedCondition, InstantiatedPredicate> &var, bool negated);

        TRUTH_VALUE check_conditions_internal(const InstantiatedCondition &condition);


        std::unordered_set<InstantiatedPredicate> knownPredicates;
        std::unordered_set<InstantiatedPredicate> unknownPredicates;
        std::unordered_set<InstantiatedParameter> objects;
        std::vector<Constraint> constraints;
        InstantiatedCondition goal;
        std::mutex mutex_;
    };



// condition

// parsing functions
    tl::expected<Domain, std::string> parse_domain(const std::string &content);

    tl::expected<Problem, std::string>
    parse_problem(const std::string &content, const std::string &domain_content = "");

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

std::ostream &operator<<(std::ostream &os,
                         const std::variant<pddl_lib::InstantiatedCondition, pddl_lib::InstantiatedPredicate> &cond);