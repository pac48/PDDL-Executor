#include <sstream>
#include <iostream>
#include <optional>
#include "pddl_parser/pddl_parser.hpp"
#include <fmt/core.h>
#include <functional>
#include <unordered_set>
#include <cassert>


std::vector<Parameter> parse_params(std::vector<std::string_view> str);


std::optional<std::string> checkParens(const std::string &content) {
    auto ind = 0ul;
    auto len = content.size();
    int num_open = 0;

    while (ind < len) {
        num_open += content[ind] == '(';
        num_open -= content[ind] == ')';
        ind++;
    }

    if (num_open == 0) {
        return {};
    } else {
        return "mismatched parentheses";
    }
}

std::tuple<std::string_view, std::string_view> getNextParen(std::string_view content_view) {
    auto ind1 = 0ul;
    auto len = content_view.size();

    while (content_view[ind1] != '(' && ind1 < len) {
        ind1++;
    }

    auto ind2 = ind1;
    int num_open = 1;
    while (num_open > 0 && ind2 < len) {
        ind2++;
        num_open += content_view[ind2] == '(';
        num_open -= content_view[ind2] == ')';
    }
    if (ind2 == len) {
        return {content_view.substr(ind1, ind2), {}};
    }
    return {content_view.substr(ind1 + 1, (ind2 - ind1) - 1), content_view.substr(ind2 + 1, len - ind2)};
}

std::vector<std::string_view>
parseVector(const std::string_view &section, std::unordered_set<char> skip_symbols) {
    std::vector<std::string_view> out;
    int ind = 0;
    while (ind < section.size()) {
        while (ind < section.size() && skip_symbols.find(section[ind]) != skip_symbols.end()) {
            ind++;
        }

        int ind1 = ind;
        int num_open = 0;
        while (ind < section.size() && (num_open != 0 || (skip_symbols.find(section[ind]) == skip_symbols.end()))) {
            num_open += section[ind] == '(';
            num_open -= section[ind] == ')';
            ind++;
        }
        if (ind - ind1 > 0) {
            if (section.substr(ind1, (ind - ind1)) == ";;") {
                while (ind < section.size() && section[ind] != '\n') {
                    ind++;
                }
            } else {
                out.emplace_back(section.substr(ind1, (ind - ind1)));
            }
        }
    }
    return out;
}

int get_line_num(const std::string &content, std::string_view current) {
    char const *data = content.data();
    char const *data2 = current.data();
    int line_num = 1;
    while (data != data2) {
        line_num += *data == '\n';
        data++;
    }
    return line_num;

}


std::optional<std::string> parseInit(const std::string &content, Domain &domain) {
    std::string_view remaining;
    std::string_view current;
    std::string_view section;
    std::string matched_token;

    std::tie(section, remaining) = getNextParen(content);
    const auto strings = parseVector(section, {'\t', '\n', ' '});

    int ind = 0;

    if (strings[ind] != "define") {
        return fmt::format("ERROR line {}: start of file does not begin with '(define'",
                           get_line_num(content, strings[ind]));
    }
    ind++;


    std::tie(section, remaining) = getNextParen(strings[ind]);
    auto substrings = parseVector(section, {'\t', '\n', ' '});
    if (substrings[0] != "domain") {
        return fmt::format("ERROR line {}: missing 'domain' keyword",
                           get_line_num(content, substrings[0]));
    }
    domain.name = substrings[1];
    ind++;


    std::tie(section, remaining) = getNextParen(strings[ind]);
    substrings = parseVector(section, {'\t', '\n', ' '});
    if (substrings[0] != ":requirements") {
        return fmt::format("ERROR line {}: missing ':requirements' keyword", get_line_num(content, substrings[0]));
    }
    for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
        domain.requirements.emplace_back(str);
    }
    ind++;


    std::tie(section, remaining) = getNextParen(strings[ind]);
    substrings = parseVector(section, {'\t', '\n', ' '});
    if (substrings[0] != ":types") {
        return fmt::format("ERROR line {}: missing ':types' keyword", get_line_num(content, substrings[0]));
    }
    for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
        domain.types.emplace_back(std::string(str));
    }
    ind++;

    std::tie(section, remaining) = getNextParen(strings[ind]);
    substrings = parseVector(section, {'\t', '\n', ' '});
    if (substrings[0] != ":predicates") {
        return fmt::format("ERROR line {}: missing ':predicates' keyword", get_line_num(content, substrings[0]));
    }
    for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
        if (auto pred = parse_predicate(std::string(str))) {
            domain.predicates.emplace_back(pred.value());
        } else {
            return fmt::format("ERROR line {}: failed to parse predicate", get_line_num(content, str));
        }
    }
    ind++;

    while (ind < strings.size()) {
        if (auto action = parse_action(std::string(strings[ind]))) {
            domain.actions.push_back(action.value());
        } else {
            return fmt::format("ERROR line {}: failed to parse action", get_line_num(content, strings[ind]));
        }
        ind++;
    }

    return {
    };
}

std::optional<Action> parse_action(const std::string &content) {
    auto action = Action();

    int ind = 0;
    std::string_view section;
    std::string_view remaining;
    std::unordered_map<std::string, std::string> param_to_type_map;

    std::tie(section, remaining) = getNextParen(content);
    const auto strings = parseVector(section, {'\t', '\n', ' '});

    if (strings[ind] != ":action") {
        return {};
    }
    ind++;

    action.name = strings[ind];
    ind++;

    while (ind < strings.size()) {
        if (strings[ind] == ":parameters") {
            ind++;
            std::tie(section, remaining) = getNextParen(strings[ind]);
            auto substrings = parseVector(section, {'\t', '\n', ' '});
            action.parameters = parse_params(substrings);
            for (const auto &param: action.parameters) {
                param_to_type_map[param.name] = param.type;
            }
            ind++;
        } else if (strings[ind] == ":precondition") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]), param_to_type_map)) {
                action.precondtions = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]), param_to_type_map)) {
                action.precondtions.predicates.emplace_back(pred.value());
            }
            ind++;
        } else if (strings[ind] == ":effect") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]), param_to_type_map)) {
                action.effect = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]), param_to_type_map)) {
                action.effect.predicates.emplace_back(pred.value());
            }
            ind++;
        } else if (strings[ind] == ":observe") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]), param_to_type_map)) {
                action.observe = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]), param_to_type_map)) {
                action.observe.predicates.emplace_back(pred.value());
            }
            ind++;
        }
    }

    return action;
}


std::optional<Domain> parse_domain(const std::string &content) {
    Domain domain;
    if (auto error = checkParens(content)) {
        std::cout << error.value() << std::endl;
        return {};
    }

    if (auto error = parseInit(content, domain)) {
        std::cout << error.value() << std::endl;
        return {};
    }

    return domain;
}

std::optional<Predicate>
parse_predicate(const std::string &content, const std::unordered_map<std::string, std::string> &param_to_type_map) {
    auto [section, remaining] = getNextParen(content);
    auto str = parseVector(section, {'\t', '\n', ' '});
    auto pred = Predicate();
    pred.name = str[0];

    pred.parameters = parse_params(std::vector<std::string_view>(str.begin() + 1, str.end()));
    for (auto &param: pred.parameters) {
        if (param_to_type_map.find(param.name) != param_to_type_map.end()) {
            param.type = param_to_type_map.at(param.name);
        }
    }

    return pred;
}

std::vector<Parameter> parse_params(std::vector<std::string_view> str) {
    std::vector<Parameter> parameters;
    int ind = 0;
    while (ind < str.size()) {
        std::vector<std::string> names;
        while (ind < str.size() && str[ind][0] == '?') {
            names.push_back(std::string(str[ind]));
            ind++;
        }
        if (ind < str.size() && str[ind][0] == '-') {
            ind++;
            auto type = str[ind];
            for (const auto &name: names) {
                auto param = Parameter();
                param.name = name;
                param.type = type;
                parameters.push_back(param);
            }
        } else if (ind == str.size()) {
            for (const auto &name: names) {
                auto param = Parameter();
                param.name = name;
                parameters.push_back(param);
            }
        }
        ind++;
    }
    return parameters;
}


std::optional<Condition>
parse_condition(const std::string &content,
                const std::unordered_map<std::string, std::string> &param_to_type_map_const) {
    std::string_view section;
    std::string_view remaining;
    Condition cond;
    auto param_to_type_map = param_to_type_map_const;

    std::tie(section, remaining) = getNextParen(content);
    auto strings = parseVector(section, {'\t', '\n', ' '});


    int ind = 1;
    if (strings[0] == "and") {
        cond.op = OPERATION::AND;
    } else if (strings[0] == "or") {
        cond.op = OPERATION::OR;
    } else if (strings[0] == "not") {
        cond.op = OPERATION::NOT;
    } else if (strings[0] == "forall") {
        cond.op = OPERATION::FORALL;
        std::tie(section, remaining) = getNextParen(strings[ind]);
        auto tmp = parseVector(section, {'\t', '\n', ' '});
        cond.parameters = parse_params(tmp);
        for (const auto &param: cond.parameters) {
            param_to_type_map[param.name] = param.type;
        }
        ind++;
    } else {
        return {};
    }
    for (int i = ind; i < strings.size(); i++) {
        if (auto cond2 = parse_condition(std::string(strings[i]), param_to_type_map)) {
            cond.conditions.emplace_back(cond2.value());
        } else if (auto pred = parse_predicate(std::string(strings[i]), param_to_type_map)) {
            cond.predicates.emplace_back(pred.value());
        }
    }

    return cond;
}

std::ostream &operator<<(std::ostream &os, const Parameter &param) {
    os << " " << param.name;
    if (!param.type.empty()) {
        os << " - " << param.type;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const InstantiatedParameter &param) {
    os << " " << param.name;
    if (!param.type.empty()) {
        os << " - " << param.type;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const Action &action) {
    os << "(:action " << action.name << "\n";
    os << "\t:parameters (";
    for (auto &param: action.parameters) {
        os << param;
    }
    os << ")\n";

    os << "\t:precondition " << action.precondtions << "\n";
    if (!action.effect.conditions.empty() || !action.effect.predicates.empty()) {
        os << "\t:effect " << action.effect << "\n";
    }
    if (!action.observe.conditions.empty() || !action.observe.predicates.empty()) {
        os << "\t:observe " << action.observe << "\n";
    }

    os << ")\n";

    return os;
}

std::ostream &operator<<(std::ostream &os, const Condition &condConst) {
    Condition cond = condConst;
    if (cond.op == OPERATION::AND && cond.predicates.size() == 1 && cond.conditions.empty()) {
        os << cond.predicates[0];
        return os;
    }

    std::string op;
    std::string term;
    auto predicates = cond.predicates;
    auto conditions = cond.conditions;
    if (cond.op == OPERATION::AND) {
        os << "(and\n";
        term = "\n";
    } else if (cond.op == OPERATION::OR) {
        os << "(or\n";
        term = "\n";
    } else if (cond.op == OPERATION::NOT) {
        os << "(not ";
        term = " ";
    } else if (cond.op == OPERATION::FORALL) {
        os << "(forall (" << cond.parameters[0].name << " - " << cond.parameters[0].type << ")\n";
        term = "\n";
        std::function<void(Condition &cond, std::vector<Parameter>)> sub_pred;
        sub_pred = [&sub_pred](Condition &cond, std::vector<Parameter> params) {
            for (auto &pred: cond.predicates) {
                for (auto &pred_param: pred.parameters) {
                    for (auto &param: params) {
                        if (pred_param.name == param.name) {
                            pred_param.type = param.type;
                        }
                    }
                }
            }
            if (cond.op == OPERATION::FORALL) {
                params.push_back(cond.parameters[0]);
            }
            for (auto &c: cond.conditions) {
                sub_pred(c, params);
            }

        };
        sub_pred(cond, {cond.parameters[0]});

    }
    for (const auto &p: predicates) {
        os << p << term;
    }
    for (const auto &c: conditions) {
        os << c << term;
    }
    os << ")";

    return os;
}


std::ostream &operator<<(std::ostream &os, const Predicate &pred) {
    os << "(" << pred.name;
    for (auto &param: pred.parameters) {
        os << " " << param.name;
    }
    os << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const InstantiatedPredicate &pred) {
    os << "(" << pred.name;
    for (auto &param: pred.parameters) {
        os << " " << param.name;
    }
    os << ")";

    return os;
}


std::ostream &operator<<(std::ostream &os, const Domain &domain) {
    os << fmt::format("(define (domain {})\n", domain.name);

    os << "(:requirements\n";
    for (auto &req: domain.requirements) {
        os << fmt::format("\t{}\n", req);
    }
    os << ")\n";

    os << "(:types\n";
    for (auto &type: domain.types) {
        os << fmt::format("\t{}\n", type);
    }
    os << ")\n";

    os << "(:predicates\n";
    for (auto &pred: domain.predicates) {
        os << "\t" << pred << "\n";
    }
    os << ")\n";

    for (auto &action: domain.actions) {
        os << action;// << "\n";
    }

    os << ")\n";

    return os;
}

InstantiatedPredicate
instantiate_predicate(const Predicate &predicate, const std::unordered_map<std::string, std::string> &param_subs) {
    auto inst_pred = InstantiatedPredicate();
    inst_pred.name = predicate.name;
    inst_pred.parameters.resize(predicate.parameters.size());

    for (int i = 0; i < predicate.parameters.size(); i++) {
        inst_pred.parameters[i].type = predicate.parameters[i].type;
        int len = predicate.parameters[i].name.size() - 1;
        auto param_name = predicate.parameters[i].name.substr(1, len);
        if (param_subs.find(param_name) != param_subs.end()) {
            inst_pred.parameters[i].name = param_subs.at(param_name);
        } else {
            inst_pred.parameters[i].name = predicate.parameters[i].name;
        }

    }

    return inst_pred;
}

InstantiatedAction
instantiate_action(const Action &action, const std::unordered_map<std::string, std::string> &param_subs,
                   const std::vector<InstantiatedParameter> &objects) {
    auto inst_action = InstantiatedAction();
    inst_action.name = action.name;

    inst_action.parameters.resize(action.parameters.size());
    for (int i = 0; i < action.parameters.size(); i++) {
        inst_action.parameters[i].type = action.parameters[i].type;
        int len = action.parameters[i].name.size() - 1;
        auto param_name = action.parameters[i].name.substr(1, len);
        if (param_subs.find(param_name) != param_subs.end()) {
            inst_action.parameters[i].name = param_subs.at(param_name);
        } else {
            inst_action.parameters[i].name = action.parameters[i].name;
        }

    }

    inst_action.precondtions = instantiate_condition(action.precondtions, param_subs, objects);
    inst_action.effect = instantiate_condition(action.effect, param_subs, objects);
    inst_action.observe = instantiate_condition(action.observe, param_subs, objects);

    return inst_action;
}

InstantiatedCondition
instantiate_condition(const Condition &conditionConst, const std::unordered_map<std::string, std::string> &param_subs,
                      const std::vector<InstantiatedParameter> &objects) {
    Condition condition = conditionConst;

    auto inst_cond = InstantiatedCondition();
    inst_cond.op = condition.op;
    inst_cond.predicates.resize(condition.predicates.size());
    for (int i = 0; i < condition.predicates.size(); i++) {
        inst_cond.predicates[i] = instantiate_predicate(condition.predicates[i], param_subs);
    }
    inst_cond.conditions.resize(condition.conditions.size());
    for (int i = 0; i < condition.conditions.size(); i++) {
        inst_cond.conditions[i] = instantiate_condition(condition.conditions[i], param_subs, objects);
    }

    if (condition.op == OPERATION::FORALL) {
        auto inst_cond_2 = InstantiatedCondition();
        inst_cond_2.op = OPERATION::AND;

        auto param_subs_mod = param_subs;
        for (const auto &obj: objects) {
            if (obj.type == condition.parameters[0].type) {
                int len = condition.parameters[0].name.size() - 1;
                auto var_name = condition.parameters[0].name.substr(1, len);
                param_subs_mod[var_name] = obj.name;

                for (int i = 0; i < condition.predicates.size(); i++) {
                    inst_cond_2.predicates.push_back(instantiate_predicate(condition.predicates[i], param_subs_mod));
                }
                for (int i = 0; i < condition.conditions.size(); i++) {
                    inst_cond_2.conditions.push_back(
                            instantiate_condition(condition.conditions[i], param_subs_mod, objects));
                }
            }
        }

        return inst_cond_2;
    }

    return inst_cond;
}


KnowledgeBase &KnowledgeBase::getInstance() {
    static KnowledgeBase instance; // Created only once
    return instance;
}

void KnownKnowledgeBase::concurrent_insert(const InstantiatedPredicate &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    insert(value);
}

void KnownKnowledgeBase::concurrent_erase(const InstantiatedPredicate &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    erase(value);
}

bool KnownKnowledgeBase::concurrent_find(const InstantiatedPredicate &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return find(value) != end();
}

void UnknownKnowledgeBase::concurrent_insert(const InstantiatedPredicate &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    insert(value);
}

void UnknownKnowledgeBase::concurrent_erase(const InstantiatedPredicate &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    erase(value);
}

bool UnknownKnowledgeBase::concurrent_find(const InstantiatedPredicate &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return find(value) != end();
}

std::string KnowledgeBase::convert_to_problem(const Domain &domain) {
    std::stringstream ss;
    ss << fmt::format("(define (problem {}_problem)\n", domain.name);
    ss << fmt::format("(:domain {})\n", domain.name);

    ss << "(:objects\n";
    for (auto &object: objects) {
        ss << "\t" << object << "\n";
    }
    ss << ")\n";

    auto pred_to_str_no_type = [](const InstantiatedPredicate &pred) {
        std::stringstream ss;
        ss << "(" << pred.name;
        for (auto &param: pred.parameters) {
            ss << " " << param.name;
        }
        ss << ")";
        return ss.str();
    };

    ss << "(:init\n";
    for (auto &pred: knownPredicates) {
        ss << "\t" << pred_to_str_no_type(pred) << "\n";
    }
    for (auto &pred: unknownPredicates) {
        ss << "\t" << "(unknown " << pred_to_str_no_type(pred) << ")" << "\n";
    }
    for (auto &constraint: unknownPredicates.constraints) {
        assert(constraint.constraint == CONSTRAINTS::ONEOF);
        ss << "\t" << "(oneof";
        for (auto &pred: constraint.predicates) {
            ss << " " << pred_to_str_no_type(pred);
        }
        ss << ")\n";
    }
    ss << ")\n";

    ss << "(:goal (success))\n";
    ss << ")\n";

    return ss.str();
}

bool KnowledgeBase::check_conditions(const InstantiatedCondition &condition) {
    bool ret;
    if (condition.op == OPERATION::AND) {
        ret = true;
        for (const auto &sub_cond: condition.conditions) {
            ret &= check_conditions(sub_cond);
        }
        for (const auto &pred: condition.predicates) {
            ret &= knownPredicates.concurrent_find(pred);
        }
    } else if (condition.op == OPERATION::OR) {
        ret = false;
        for (const auto &sub_cond: condition.conditions) {
            ret |= check_conditions(sub_cond);
        }
        for (const auto &pred: condition.predicates) {
            ret |= knownPredicates.concurrent_find(pred);
        }
    } else if (condition.op == OPERATION::NOT) {
        if (condition.predicates.size() == 1 && condition.conditions.empty()) {
            auto pred = condition.predicates[0];
            return !knownPredicates.concurrent_find(pred) && !unknownPredicates.concurrent_find(pred);
        } else if (condition.predicates.empty() && condition.conditions.size() == 1) {
            return !check_conditions(condition.conditions[0]);
        } else {
            throw std::runtime_error("OPERATION::NOT should applies to only a single condition or predicate");
        }

    } else {
        throw std::runtime_error("Only AND, OR, and NOT operations are  allowed in a InstantiatedCondition");
    }

    return ret;
}

void KnowledgeBase::apply_conditions(const InstantiatedCondition &condition, bool negated) {
    if (condition.op == OPERATION::AND) {
        for (const auto &sub_cond: condition.conditions) {
            apply_conditions(sub_cond, negated);
        }
    } else if (condition.op == OPERATION::NOT) {
        negated = !negated;
        for (const auto &sub_cond: condition.conditions) {
            apply_conditions(sub_cond, negated);
        }
    } else {
        throw std::runtime_error("Only AND and NOT operations can be applied");
    }
    for (const auto &pred: condition.predicates) {
        if (!negated) {
            knownPredicates.concurrent_insert(pred);
        } else {
            knownPredicates.concurrent_erase(pred);
        }
        // whether or not the predicate becomes true or false, it must be removed from the unknown set
        unknownPredicates.concurrent_erase(pred);
    }
}

void KnowledgeBase::apply_constraints() {
    int ind = 0;
    while (ind < unknownPredicates.constraints.size()) {
        auto constraint = unknownPredicates.constraints[ind];
        if (constraint.constraint == CONSTRAINTS::ONEOF) {
            std::vector<InstantiatedPredicate> preds;
            for (const auto &pred_con: constraint.predicates) {
                if (unknownPredicates.concurrent_find(pred_con)) {
                    preds.push_back(pred_con);
                }
            }
            if (preds.size() == 1) {
                unknownPredicates.concurrent_erase(preds[0]);
                knownPredicates.concurrent_insert(preds[0]);
                unknownPredicates.constraints.erase(unknownPredicates.constraints.begin() + ind);
                continue;
            }

        } else {
            throw std::runtime_error("Only CONSTRAINTS::ONEOF constraint is supported");
        }
        ind++;
    }
}
