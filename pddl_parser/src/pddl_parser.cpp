#include <sstream>
#include <iostream>
#include <optional>
#include "pddl_parser/pddl_parser.hpp"
#include <fmt/core.h>
#include <functional>
#include <unordered_set>
#include <cassert>

namespace pddl_lib {

    std::vector<Parameter> parse_params(std::vector<std::string_view> str);

    std::vector<InstantiatedParameter> parse_instantiated_params(std::vector<std::string_view> str);


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
    parseVector(const std::string_view &section, std::unordered_set<char> skip_symbols,
                std::vector<std::string_view> &comments) {
        std::vector<std::string_view> out;
        int ind = 0;
        while (ind < section.size()) {
            while (ind < section.size() && skip_symbols.find(section[ind]) != skip_symbols.end()) {
                ind++;
            }

            int ind1 = ind;
            int num_open = 0;
            int num_close = 0;
            while (ind < section.size() && (num_open > 0 || skip_symbols.find(section[ind]) == skip_symbols.end())) {
                num_open += section[ind] == '(';
                num_close += section[ind] == ')';
                ind++;
                if (num_open > 0 && num_open == num_close) {
                    break;
                }
            }
            if (ind - ind1 > 0) {
                if (ind - ind1 >= 2 && section.substr(ind1, 2) == ";;") {
                    while (ind < section.size() && section[ind] != '\n') {
                        ind++;
                    }
                    comments.push_back(section.substr(ind1, (ind - ind1)));
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


    std::optional<std::string> parse_problem_internal(const std::string &content, Problem &problem) {
        std::string_view remaining;
        std::string_view current;
        std::string_view section;
        std::string matched_token;
        std::vector<std::string_view> all_comments;

        std::tie(section, remaining) = getNextParen(content);
        const auto strings = parseVector(section, {'\t', '\n', ' '}, all_comments);

        int ind = 0;

        if (strings[ind] != "define") {
            return fmt::format("ERROR line {}: start of file does not begin with '(define'",
                               get_line_num(content, strings[ind]));
        }
        ind++;


        std::tie(section, remaining) = getNextParen(strings[ind]);
        auto substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        if (substrings[0] != "problem") {
            return fmt::format("ERROR line {}: missing 'domain' keyword",
                               get_line_num(content, substrings[0]));
        }
        problem.name = substrings[1];
        ind++;


        std::tie(section, remaining) = getNextParen(strings[ind]);
        substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        if (substrings[0] != ":domain") {
            return fmt::format("ERROR line {}: missing ':domain' keyword", get_line_num(content, substrings[0]));
        }
        problem.domain = substrings[1];
        ind++;

        std::tie(section, remaining) = getNextParen(strings[ind]);
        substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        if (substrings[0] == ":objects") {
//            return fmt::format("ERROR line {}: missing ':objects' keyword", get_line_num(content, substrings[0]));
            problem.objects = parse_instantiated_params(
                    std::vector<std::string_view>(substrings.begin() + 1, substrings.end()));
            ind++;
            std::tie(section, remaining) = getNextParen(strings[ind]);
            substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        }

        if (substrings[0] != ":init") {
            return fmt::format("ERROR line {}: missing ':init' keyword", get_line_num(content, substrings[0]));
        }
        std::unordered_map<std::string, std::string> param_to_type_map;
        for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
            std::tie(section, remaining) = getNextParen(str);
            auto subsubstrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
            if (subsubstrings[0] == "unknown") {
                if (auto pred = parse_instantiated_predicate(std::string(subsubstrings[1]))) {
                    problem.unknowns.insert({pred.value()});
                } else {
                    return fmt::format("ERROR line {}: failed to parse unknown predicate \n{}",
                                       get_line_num(content, str),
                                       pred.error());
                }
            } else if (subsubstrings[0] == "oneof") {
                Constraint con;
                con.constraint = CONSTRAINTS::ONEOF;
                InstantiatedCondition condition;
                for (const auto &tmp: std::vector<std::string_view>(subsubstrings.begin() + 1, subsubstrings.end())) {
                    if (auto pred = parse_instantiated_predicate(std::string(tmp))) {
                        condition.conditions.push_back(pred.value());
                        problem.unknowns.insert({pred.value()});
                    } else {
                        return fmt::format("ERROR line {}: failed to parse oneof predicate \n{}",
                                           get_line_num(content, str),
                                           pred.error());
                    }
                }
                con.condition = condition;
                problem.constraints.push_back(con);

            } else if (subsubstrings[0] == "or") {
                Constraint con;
                con.constraint = CONSTRAINTS::OR;
                InstantiatedCondition condition;
                for (const auto &tmp: std::vector<std::string_view>(subsubstrings.begin() + 1, subsubstrings.end())) {
                    if (auto pred = parse_instantiated_predicate(std::string(tmp))) {
                        condition.conditions.push_back(pred.value());
                        continue;
                    }
                    if (auto cond = parse_instantiated_condition(std::string(tmp))) {
                        condition.conditions.push_back(cond.value());
                        continue;
                    }
                    return fmt::format("ERROR line {}: failed to parse or condition \n",
                                       get_line_num(content, str));
                }
                con.condition = condition;
                problem.constraints.push_back(con);

            } else {
                if (auto pred = parse_instantiated_predicate(std::string(str), param_to_type_map)) {
                    problem.init.insert(pred.value());
                } else {
                    return fmt::format("ERROR line {}: failed to parse predicate \n{}", get_line_num(content, str),
                                       pred.error());
                }
            }


        }
        ind++;

        std::tie(section, remaining) = getNextParen(strings[ind]);
        substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        if (substrings[0] != ":goal") {
            return fmt::format("ERROR line {}: missing ':goal' keyword", get_line_num(content, substrings[0]));
        }
        if (auto cond = parse_instantiated_condition(std::string(substrings[1]))) {
            problem.goal = cond.value();
        } else {
            problem.goal.op = OPERATION::AND;
            for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
                if (auto pred = parse_instantiated_predicate(std::string(str))) {
                    problem.goal.conditions.push_back(pred.value());
                    continue;
                } else {
                    return fmt::format("ERROR line {}: failed to parse goal \n", get_line_num(content, str));
                }
            }
        }
        ind++;

        problem.comments.insert(problem.comments.end(), all_comments.begin(), all_comments.end());

        return {};
    }


    std::optional<std::string> parse_domain_internal(const std::string &content, Domain &domain) {
        std::string_view remaining;
        std::string_view current;
        std::string_view section;
        std::string matched_token;
        std::vector<std::string_view> all_comments;

        std::tie(section, remaining) = getNextParen(content);
        const auto strings = parseVector(section, {'\t', '\n', ' '}, all_comments);

        int ind = 0;

        if (strings[ind] != "define") {
            return fmt::format("ERROR line {}: start of file does not begin with '(define'",
                               get_line_num(content, strings[ind]));
        }
        ind++;


        std::tie(section, remaining) = getNextParen(strings[ind]);
        auto substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        if (substrings[0] != "domain") {
            return fmt::format("ERROR line {}: missing 'domain' keyword",
                               get_line_num(content, substrings[0]));
        }
        domain.name = substrings[1];
        ind++;
        std::tie(section, remaining) = getNextParen(strings[ind]);
        substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);

        if (substrings[0] == ":requirements") {
            for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
                domain.requirements.emplace_back(str);
            }
            ind++;

            std::tie(section, remaining) = getNextParen(strings[ind]);
            substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        }

        if (substrings[0] == ":types") {
            for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
                domain.types.insert(std::string(str));
            }
            ind++;

            std::tie(section, remaining) = getNextParen(strings[ind]);
            substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        }

        if (substrings[0] == ":constants") {
            domain.constants = parse_instantiated_params(
                    std::vector<std::string_view>(substrings.begin() + 1, substrings.end()));
            for (const auto &con: domain.constants) {
                domain.types.insert(con.type);
            }
            ind++;

            std::tie(section, remaining) = getNextParen(strings[ind]);
            substrings = parseVector(section, {'\t', '\n', ' '}, all_comments);
        }


        if (substrings[0] != ":predicates") {
            return fmt::format("ERROR line {}: missing ':predicates' keyword", get_line_num(content, substrings[0]));
        }
        for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
            if (auto pred = parse_predicate(std::string(str))) {
                domain.predicates.insert(pred.value());
            } else {
                return fmt::format("ERROR line {}: failed to parse predicate \n{}", get_line_num(content, str),
                                   pred.error());
            }
        }
        ind++;

        while (ind < strings.size()) {
            if (auto action = parse_action(std::string(strings[ind]))) {
                domain.actions.push_back(action.value());
            } else {
                return fmt::format("ERROR line {}: failed to parse action \n{}", get_line_num(content, strings[ind]),
                                   action.error());
            }
            ind++;
        }

        domain.comments.insert(domain.comments.end(), all_comments.begin(), all_comments.end());

        return {};
    }

    tl::expected<Action, std::string> parse_action(const std::string &content) {
        try {
            auto action = Action();

            int ind = 0;
            std::string_view section;
            std::string_view remaining;
            std::vector<std::string_view> comments;
            std::unordered_map<std::string, std::string> param_to_type_map;

            std::tie(section, remaining) = getNextParen(content);
            const auto strings = parseVector(section, {'\t', '\n', ' '}, comments);

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
                    auto substrings = parseVector(section, {'\t', '\n', ' '}, comments);
                    action.parameters = parse_params(substrings);
                    for (const auto &param: action.parameters) {
                        param_to_type_map[param.name] = param.type;
                    }
                } else if (strings[ind] == ":precondition") {
                    ind++;
                    if (auto cond = parse_condition(std::string(strings[ind]), param_to_type_map)) {
                        action.precondtions = cond.value();
                        continue;
                    }
                    if (auto pred = parse_predicate(std::string(strings[ind]), param_to_type_map)) {
                        action.precondtions.conditions.emplace_back(pred.value());
                        continue;
                    }
                    return tl::unexpected(fmt::format("ERROR: failed to parse precondition of action {}", action.name));

                } else if (strings[ind] == ":effect") {
                    ind++;
                    if (auto cond = parse_condition(std::string(strings[ind]), param_to_type_map)) {
                        action.effect = cond.value();
                        continue;
                    }
                    if (auto pred = parse_predicate(std::string(strings[ind]), param_to_type_map)) {
                        action.effect.conditions.emplace_back(pred.value());
                        continue;
                    }
                    return tl::unexpected(fmt::format("ERROR: failed to parse effect of action {}", action.name));

                } else if (strings[ind] == ":observe") {
                    ind++;
                    if (auto cond = parse_condition(std::string(strings[ind]), param_to_type_map)) {
                        action.observe = cond.value();
                        continue;
                    }
                    if (auto pred = parse_predicate(std::string(strings[ind]), param_to_type_map)) {
                        action.observe.conditions.emplace_back(pred.value());
                        continue;
                    }
                    return tl::unexpected(fmt::format("ERROR: failed to parse observe of action {}", action.name));

                }
                ind++;
            }

            return action;

        } catch (std::exception e) {
            return tl::unexpected(std::string("ERROR: failed to parse action"));
        }
    }


    tl::expected<Domain, std::string> parse_domain(const std::string &content) {
        Domain domain;
        if (auto error = checkParens(content)) {
            return tl::unexpected(fmt::format("ERROR: failed to parse domain\n{}", error.value()));
        }

        if (auto error = parse_domain_internal(content, domain)) {
            return tl::unexpected(fmt::format("ERROR: failed to parse domain\n{}", error.value()));
        }

        return domain;
    }

    tl::expected<Problem, std::string> parse_problem(const std::string &content, const std::string &domain_content) {
        Problem problem;
        if (auto error = checkParens(content)) {
            return tl::unexpected(fmt::format("ERROR: failed to parse problem\n{}", error.value()));
        }

        if (auto error = parse_problem_internal(content, problem)) {
            return tl::unexpected(fmt::format("ERROR: failed to parse problem\n{}", error.value()));
        }

        if (!domain_content.empty()) {
            Domain domain;
            if (auto error = parse_domain_internal(domain_content, domain)) {
                return tl::unexpected(
                        fmt::format("ERROR: failed to parse optional domain argument\n{}", error.value()));
            }
            std::unordered_map<std::string, std::vector<Parameter>> type_map;
            for (const auto &pred: domain.predicates) {
                type_map[pred.name] = pred.parameters;
            }
            auto update_set = [type_map](std::unordered_set<InstantiatedPredicate> &pred_set) {
                auto init_copy = pred_set;
                for (auto pred: init_copy) {
                    auto domain_pred = type_map.at(pred.name);
                    assert(pred.parameters.size() == domain_pred.size());
                    pred_set.erase(pred);
                    for (auto i = 0ul; i < pred.parameters.size(); i++) {
                        pred.parameters[i].type = domain_pred[i].type;
                    }
                    pred_set.insert(pred);
                }
            };
            std::function<void(InstantiatedCondition &cond)> update_condition;
            update_condition = [&type_map, &update_condition](InstantiatedCondition &cond) {
                for (auto &c: cond.conditions) {
                    if (auto inst = std::get_if<InstantiatedCondition>(&c)) {
                        update_condition(*inst);
                    } else {
                        auto &pred = std::get<InstantiatedPredicate>(c);
                        auto domain_pred = type_map.at(pred.name);
                        assert(pred.parameters.size() == domain_pred.size());
                        for (auto i = 0ul; i < pred.parameters.size(); i++) {
                            pred.parameters[i].type = domain_pred[i].type;
                        }
                    }
                }
            };

            update_set(problem.init);
            update_set(problem.unknowns);
            for (auto &constraint: problem.constraints) {
                update_condition(constraint.condition);
            }
        }

        return problem;
    }

    tl::expected<Predicate, std::string>
    parse_predicate(const std::string &content, const std::unordered_map<std::string, std::string> &param_to_type_map) {
        try {
            std::vector<std::string_view> comments;
            auto pred = Predicate();
            auto [section, remaining] = getNextParen(content);
            auto str = parseVector(section, {'\t', '\n', ' '}, comments);
            pred.name = str[0];

            pred.parameters = parse_params(std::vector<std::string_view>(str.begin() + 1, str.end()));
            for (auto &param: pred.parameters) {
                if (param_to_type_map.find(param.name) != param_to_type_map.end()) {
                    param.type = param_to_type_map.at(param.name);
                }
            }

            return pred;

        } catch (std::exception e) {
            return tl::unexpected(std::string("ERROR: failed to parse predicate"));
        }
    }

    tl::expected<InstantiatedPredicate, std::string>
    parse_instantiated_predicate(const std::string &content,
                                 const std::unordered_map<std::string, std::string> &param_to_type_map) {
        try {
            std::vector<std::string_view> comments;
            auto pred = InstantiatedPredicate();
            auto [section, remaining] = getNextParen(content);
            auto str = parseVector(section, {'\t', '\n', ' '}, comments);
            pred.name = str[0];

            pred.parameters = parse_instantiated_params(std::vector<std::string_view>(str.begin() + 1, str.end()));
            for (auto &param: pred.parameters) {
                if (param_to_type_map.find(param.name) != param_to_type_map.end()) {
                    param.type = param_to_type_map.at(param.name);
                }
            }

            return pred;

        } catch (std::exception e) {
            return tl::unexpected(std::string("ERROR: failed to parse predicate"));
        }
    }

    std::vector<Parameter> parse_params(std::vector<std::string_view> str) {
        std::vector<Parameter> parameters;
        int ind = 0;
        while (ind < str.size()) {
            std::vector<std::string> names;
//            while (ind < str.size() && str[ind][0] == '?') {
//                names.push_back(std::string(str[ind]));
//                ind++;
//            }
            while (ind < str.size() && str[ind][0] != '-') {
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
            }
            if (ind == str.size()) {
                for (const auto &name: names) {
                    auto param = Parameter();
                    param.name = name;
                    parameters.push_back(param);
                }
            }
//            else {
//                throw std::runtime_error("failed to parse parameters");
//            }
            ind++;
        }
        return parameters;
    }

    std::vector<InstantiatedParameter> parse_instantiated_params(std::vector<std::string_view> str) {
        std::vector<InstantiatedParameter> parameters;
        int ind = 0;
        while (ind < str.size()) {
            std::vector<std::string> names;
            while (ind < str.size() && isalnum(str[ind][0])) {
                names.emplace_back(str[ind]);
                ind++;
            }
            if (ind < str.size() && str[ind][0] == '-') {
                ind++;
                auto type = str[ind];
                for (const auto &name: names) {
                    auto param = InstantiatedParameter();
                    param.name = name;
                    param.type = type;
                    parameters.push_back(param);
                }
            } else if (ind == str.size()) {
                for (const auto &name: names) {
                    auto param = InstantiatedParameter();
                    param.name = name;
                    parameters.push_back(param);
                }
            } else {
                throw std::runtime_error("failed to parse parameters");
            }
            ind++;
        }
        std::unordered_set<std::string> duplicate_checker;
        for (const auto &param: parameters) {
            if (duplicate_checker.find(param.name) == duplicate_checker.end()) {
                duplicate_checker.insert(param.name);
            } else {
                throw std::runtime_error("duplicate object in parameter list");
            }
        }
        return parameters;
    }


    tl::expected<Condition, std::string>
    parse_condition(const std::string &content,
                    const std::unordered_map<std::string, std::string> &param_to_type_map_const) {
        std::string_view section;
        std::string_view remaining;
        std::vector<std::string_view> comments;
        Condition cond;
        auto param_to_type_map = param_to_type_map_const;

        std::tie(section, remaining) = getNextParen(content);
        auto strings = parseVector(section, {'\t', '\n', ' ', ')'}, comments);
        if (strings.empty()) {
            cond.op = OPERATION::AND;
            return cond;
        }

        int ind = 1;
        if (strings[0] == "and") {
            cond.op = OPERATION::AND;
        } else if (strings[0] == "or") {
            cond.op = OPERATION::OR;
        } else if (strings[0] == "not") {
            cond.op = OPERATION::NOT;
        } else if (strings[0] == "when") {
            cond.op = OPERATION::WHEN;
        } else if (strings[0] == "forall") {
            cond.op = OPERATION::FORALL;
            std::tie(section, remaining) = getNextParen(strings[ind]);
            auto tmp = parseVector(section, {'\t', '\n', ' '}, comments);
            cond.parameters = parse_params(tmp);
            for (const auto &param: cond.parameters) {
                param_to_type_map[param.name] = param.type;
            }
            ind++;
        } else {
            return tl::unexpected(std::string("ERROR: failed to parse condition"));
        }

        for (int i = ind; i < strings.size(); i++) {
            if (auto cond2 = parse_condition(std::string(strings[i]), param_to_type_map)) {
                cond.conditions.emplace_back(cond2.value());
                continue;
            }
            if (auto pred = parse_predicate(std::string(strings[i]), param_to_type_map)) {
                cond.conditions.emplace_back(pred.value());
                continue;
            }

            return tl::unexpected(std::string("ERROR: failed to parse condition"));
        }

        return cond;

    }

    tl::expected<InstantiatedCondition, std::string>
    parse_instantiated_condition(const std::string &content,
                                 const std::unordered_map<std::string, std::string> &param_to_type_map) {
        std::string_view section;
        std::string_view remaining;
        InstantiatedCondition cond;
        std::vector<std::string_view> comments;

        std::tie(section, remaining) = getNextParen(content);
        auto strings = parseVector(section, {'\t', '\n', ' '}, comments);
        if (strings.empty()) {
            cond.op = OPERATION::AND;
            return cond;
        }

        int ind = 1;
        if (strings[0] == "and") {
            cond.op = OPERATION::AND;
        } else if (strings[0] == "not") {
            cond.op = OPERATION::NOT;
        } else {
            return tl::unexpected(std::string("ERROR: failed to parse instantiated condition"));
        }

        for (int i = ind; i < strings.size(); i++) {
            if (auto cond2 = parse_instantiated_condition(std::string(strings[i]), param_to_type_map)) {
                cond.conditions.emplace_back(cond2.value());
                continue;
            }
            if (auto pred = parse_instantiated_predicate(std::string(strings[i]), param_to_type_map)) {
                cond.conditions.emplace_back(pred.value());
                continue;
            }

            return tl::unexpected(std::string("ERROR: failed to parse condition"));
        }

        return cond;

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
                       const std::unordered_set<InstantiatedParameter> &objects) {
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
    instantiate_condition(const Condition &conditionConst,
                          const std::unordered_map<std::string, std::string> &param_subs,
                          const std::unordered_set<InstantiatedParameter> &objects) {
        Condition condition = conditionConst;
        auto inst_cond = InstantiatedCondition();

        if (condition.op == OPERATION::FORALL) {
            inst_cond.op = OPERATION::AND;

            auto param_subs_mod = param_subs;
            for (const auto &obj: objects) {
                if (obj.type == condition.parameters[0].type) {
                    int len = condition.parameters[0].name.size() - 1;
                    auto var_name = condition.parameters[0].name.substr(1, len);
                    param_subs_mod[var_name] = obj.name;


                    for (const auto &c: condition.conditions) {
                        if (auto inst = std::get_if<Condition>(&c)) {
                            inst_cond.conditions.emplace_back(instantiate_condition(*inst, param_subs_mod, objects));
                        } else {
                            inst_cond.conditions.emplace_back(
                                    instantiate_predicate(std::get<Predicate>(c), param_subs_mod));
                        }
                    }
                }
            }
        } else {
            inst_cond.op = condition.op;
            for (const auto &c: condition.conditions) {
                if (auto inst = std::get_if<Condition>(&c)) {
                    auto potential_cond = instantiate_condition(*inst, param_subs, objects);
                    if (!potential_cond.conditions.empty()) {
                        inst_cond.conditions.emplace_back(potential_cond);
                    }
                } else {
                    inst_cond.conditions.emplace_back(instantiate_predicate(std::get<Predicate>(c), param_subs));
                }
            }
        }

        return inst_cond;
    }


    KnowledgeBase &KnowledgeBase::getInstance() {
        static KnowledgeBase instance; // Created only once
        return instance;
    }

    bool check_pred_in_domain(const InstantiatedPredicate &pred,
                              std::unordered_map<std::string, const Predicate *> name_pred) {
        auto it = name_pred.find(pred.name);
        if (it == name_pred.end()) {
            return false;
        }
        auto potential_match = it->second;
        if (potential_match->parameters.size() != pred.parameters.size()) {
            return false;
        }
        for (int i = 0; i < pred.parameters.size(); i++) {
            if (pred.parameters[i].type != potential_match->parameters[i].type) {
                return false;
            }
        }
        return true;

    };

    std::string KnowledgeBase::convert_to_problem(const Domain &domain) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::stringstream ss;
        ss << fmt::format("(define (problem {}_problem)\n", domain.name);
        ss << fmt::format("(:domain {})\n", domain.name);

        ss << "(:objects\n";
        for (auto &object: objects) {
            if (domain.types.find(object.type) != domain.types.end() || object.type.empty()) {
                ss << "\t" << object << "\n";
            }
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


        std::unordered_map<std::string, const Predicate *> name_pred;
        for (const auto &pred: domain.predicates) {
            name_pred[pred.name] = &pred;
        }


        ss << "(:init\n";
        for (const auto &pred: knownPredicates) {
            if (check_pred_in_domain(pred, name_pred)) {
                ss << "\t" << pred_to_str_no_type(pred) << "\n";
            }
        }
        for (auto &pred: unknownPredicates) {
            if (check_pred_in_domain(pred, name_pred)) {
                ss << "\t" << "(unknown " << pred_to_str_no_type(pred) << ")" << "\n";
            }
        }
        for (auto &constraint: constraints) {
            // remove entire constraint if any of the predicates are missing
            if (constraint.constraint == CONSTRAINTS::ONEOF) {
                bool none_missing = true;
                for (auto &c: constraint.condition.conditions) {
                    none_missing &= check_pred_in_domain(std::get<InstantiatedPredicate>(c), name_pred);
                }
                if (!none_missing) {
                    continue;
                }
                ss << "\t" << "(oneof";
                for (auto &c: constraint.condition.conditions) {
                    ss << " " << pred_to_str_no_type(std::get<InstantiatedPredicate>(c));
                }
                ss << ")\n";
            } else if (constraint.constraint == CONSTRAINTS::OR) {
                bool none_missing = true;
                std::function<void(
                        const InstantiatedCondition &condition)> tmp_check = [&none_missing, &tmp_check, name_pred](
                        const InstantiatedCondition &condition) {
                    for (auto &c: condition.conditions) {
                        if (auto pred = std::get_if<InstantiatedPredicate>(&c)) {
                            none_missing &= check_pred_in_domain(std::get<InstantiatedPredicate>(c), name_pred);
                        } else {
                            tmp_check(std::get<InstantiatedCondition>(c));
                        }
                    }
                };
                tmp_check(constraint.condition);
                if (!none_missing) {
                    continue;
                }
                assert(constraint.condition.conditions.size() == 2);
                ss << "\t" << "(or";
                ss << " " << constraint.condition.conditions[0];
                ss << " " << constraint.condition.conditions[1];
                ss << ")\n";
            }

        }
        ss << ")\n";

        ss << "(:goal\n";
        ss << goal;
        ss << ")\n";

        ss << ")\n";

        return ss.str();
    }

    pddl_lib::TRUTH_VALUE KnowledgeBase::check_conditions_internal(const InstantiatedCondition &condition) {
        pddl_lib::TRUTH_VALUE ret;
        if (condition.op == OPERATION::AND) {
            ret = pddl_lib::TRUTH_VALUE::TRUE;
            for (const auto &sub_cond: condition.conditions) {
                ret &= check_variant_internal(sub_cond);
            }
            return ret;
        }
        if (condition.op == OPERATION::OR) {
            ret = pddl_lib::TRUTH_VALUE::FALSE;
            for (const auto &sub_cond: condition.conditions) {
                ret |= check_variant_internal(sub_cond);
            }
            return ret;
        }
        if (condition.op == OPERATION::NOT) {
            assert(condition.conditions.size() == 1);
            ret = !check_variant_internal(condition.conditions[0]);
            return ret;
        }

        throw std::runtime_error("Only AND, OR, and NOT operations are  allowed in a InstantiatedCondition");
    }

    pddl_lib::TRUTH_VALUE
    KnowledgeBase::check_variant_internal(const std::variant<InstantiatedCondition, InstantiatedPredicate> &condition) {
        if (auto pred = std::get_if<InstantiatedPredicate>(&condition)) {
            if (knownPredicates.find(*pred) != knownPredicates.end()) {
                return pddl_lib::TRUTH_VALUE::TRUE;
            } else if (unknownPredicates.find(*pred) != unknownPredicates.end()) {
                return pddl_lib::TRUTH_VALUE::UNKNOWN;
            } else {
                return pddl_lib::TRUTH_VALUE::FALSE;
            }
        } else {
            return check_conditions_internal(std::get<InstantiatedCondition>(condition));
        }

    }

    void
    KnowledgeBase::apply_variant_internal(const std::variant<InstantiatedCondition, InstantiatedPredicate> &condition,
                                          bool negated) {
        if (auto pred = std::get_if<InstantiatedPredicate>(&condition)) {
            if (!negated) {
                knownPredicates.insert(*pred);
            } else {
                knownPredicates.erase(*pred);
            }
            // whether or not the predicate becomes true or false, it must be removed from the unknown set
            unknownPredicates.erase({*pred});
        } else {
            apply_conditions_internal(std::get<InstantiatedCondition>(condition), negated);
        }
    }

    void KnowledgeBase::apply_conditions_internal(const InstantiatedCondition &condition, bool negated) {
        if (condition.op == OPERATION::AND) {
            for (const auto &sub_cond: condition.conditions) {
                apply_variant_internal(sub_cond, negated);
            }
        } else if (condition.op == OPERATION::NOT) {
            negated = !negated;
            assert(condition.conditions.size() == 1);
            apply_variant_internal(condition.conditions[0], negated);
        } else if (condition.op == OPERATION::WHEN) {
            assert(condition.conditions.size() == 2);
            if (check_variant_internal(condition.conditions[0]) == TRUTH_VALUE::TRUE) {
                apply_variant_internal(condition.conditions[1], negated);
            }
        } else {
            throw std::runtime_error("Only AND and NOT operations can be applied");
        }
    }

    TRUTH_VALUE KnowledgeBase::check_conditions(const InstantiatedCondition &condition) {
        std::lock_guard<std::mutex> lock(mutex_);
        return check_conditions_internal(condition);
    }

    void KnowledgeBase::apply_conditions(const InstantiatedCondition &condition, bool negated) {
        std::lock_guard<std::mutex> lock(mutex_);
        apply_conditions_internal(condition, negated);
    }

    void KnowledgeBase::apply_constraints() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (unknownPredicates.empty()){
            return;
        }
        for (const auto &constraint: constraints) {
            if (constraint.constraint == CONSTRAINTS::ONEOF) {
                std::vector<InstantiatedPredicate> unknown_preds;
                std::optional<InstantiatedPredicate> known_pred;
                for (const auto &c: constraint.condition.conditions) {
                    if (auto pred_con = std::get_if<InstantiatedPredicate>(&c)) {
                        if (unknownPredicates.find(*pred_con) != unknownPredicates.end()) {
                            unknown_preds.push_back(*pred_con);
                        } else if (knownPredicates.find(*pred_con) != knownPredicates.end()) {
                            known_pred = *pred_con;
                        }
                    } else {
                        throw std::runtime_error("ONEOF constraint acts on predicates not conditions");
                    }
                }
//                assert(!unknown_preds.empty() || known_pred.has_value());
                if (unknown_preds.size() == 1) {
                    if (known_pred.has_value()) {
                        knownPredicates.erase(unknown_preds[0]);
                    } else {
                        knownPredicates.insert(unknown_preds[0]);
                    }
                    for (const auto &value: unknown_preds) {
                        unknownPredicates.erase(value);
                    }
                }
            } else if (constraint.constraint == CONSTRAINTS::OR) {
                assert(constraint.condition.conditions.size() == 2);
                auto cond1 = check_variant_internal(constraint.condition.conditions[0]);
                auto cond2 = check_variant_internal(constraint.condition.conditions[1]);
//                assert(!(cond2 == TRUTH_VALUE::TRUE && cond1 ==
//                                                       TRUTH_VALUE::TRUE)); // TODO only one condition can be true, but need to check unknowns
                if (cond2 == TRUTH_VALUE::FALSE) {
                    apply_variant_internal(constraint.condition.conditions[0], false);
                } else if (cond2 == TRUTH_VALUE::TRUE) {
                    apply_variant_internal(constraint.condition.conditions[0], true);
                } else if (cond1 == TRUTH_VALUE::FALSE) {
                    apply_variant_internal(constraint.condition.conditions[1], false);
                } else if (cond1 == TRUTH_VALUE::TRUE) {
                    apply_variant_internal(constraint.condition.conditions[1], true);
                }
            } else {
                throw std::runtime_error("Only CONSTRAINTS::ONEOF constraint is supported");
            }
        }
    }

    void KnowledgeBase::insert_object(const InstantiatedParameter &obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        objects.insert(obj);
    }

    void KnowledgeBase::erase_object(const InstantiatedParameter &obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        objects.erase(obj);
    }

    bool KnowledgeBase::find_object(const InstantiatedParameter &obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        return objects.find(obj) != objects.end();
    }

    void KnowledgeBase::insert_predicate(const InstantiatedPredicate &pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        knownPredicates.insert(pred);
    }

    void KnowledgeBase::erase_predicate(const InstantiatedPredicate &pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        knownPredicates.erase(pred);
    }

    bool KnowledgeBase::find_predicate(const InstantiatedPredicate &pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        return knownPredicates.find(pred) != knownPredicates.end();
    }

    void KnowledgeBase::insert_unknown_predicate(const InstantiatedPredicate &pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        unknownPredicates.insert(pred);
    }

    void KnowledgeBase::erase_unknown_predicate(const InstantiatedPredicate &pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        unknownPredicates.erase(pred);
    }

    bool KnowledgeBase::find_unknown_predicate(const InstantiatedPredicate &pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        return unknownPredicates.find(pred) != unknownPredicates.end();
    }

    void KnowledgeBase::insert_constraint(const Constraint &constraint) {
        std::lock_guard<std::mutex> lock(mutex_);
        constraints.push_back(constraint);
    }

    void KnowledgeBase::set_goal(const InstantiatedCondition &new_goal) {
        std::lock_guard<std::mutex> lock(mutex_);
        goal = new_goal;
    }

    std::unordered_set<InstantiatedParameter> KnowledgeBase::get_objects() {
        std::lock_guard<std::mutex> lock(mutex_);
        return objects;
    }

    std::unordered_set<InstantiatedPredicate> KnowledgeBase::get_known_predicates() {
        std::lock_guard<std::mutex> lock(mutex_);
        return knownPredicates;
    }

    void KnowledgeBase::clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        objects.clear();
        knownPredicates.clear();
        unknownPredicates.clear();
        constraints.clear();
    }

    void KnowledgeBase::load_kb(const Problem &problem) {
        // does not need mutex becuase all of the methods are thread safe
        for (const auto &obj: problem.objects) {
            insert_object(obj);
        }
        for (const auto &pred: problem.init) {
            insert_predicate(pred);
        }
        for (const auto &pred: problem.unknowns) {
            erase_predicate(pred);
            insert_unknown_predicate(pred);
        }
        for (const auto &constraint: problem.constraints) {
            insert_constraint(constraint);
        }
        set_goal(problem.goal);

    }

    void KnowledgeBase::clear_unknowns() {
        std::lock_guard<std::mutex> lock(mutex_);
        unknownPredicates.clear();
        constraints.clear();
    }

    void KnowledgeBase::print_predicate() {
//        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "objects :\n";
        for (const auto &obj: objects) {
            std::cout << obj << "\n";
        }
        std::cout << "predicates :\n";
        for (const auto &pred: knownPredicates) {
            std::cout << pred << "\n";
        }
        std::cout << "unknowns :\n";
        for (const auto &pred: unknownPredicates) {
            std::cout << pred << "\n";
        }
        for (const auto &constraint: constraints) {
            if (constraint.constraint == CONSTRAINTS::ONEOF) {
                std::cout << "oneof : " << constraint.condition << "\n";
            }
            if (constraint.constraint == CONSTRAINTS::OR) {
                std::cout << "or : " << constraint.condition << "\n";
            }
        }
        std::cout << "goal : " << goal << "\n";
    }


    std::string Domain::str() {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }


    std::string Problem::str() {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

} // pddl_lib


std::ostream &operator<<(std::ostream &os, const pddl_lib::Parameter &param) {
    os << " " << param.name;
    if (!param.type.empty()) {
        os << " - " << param.type;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedParameter &param) {
    os << " " << param.name;
    if (!param.type.empty()) {
        os << " - " << param.type;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::Action &action) {
    os << "(:action " << action.name << "\n";
    os << "\t:parameters (";
    for (auto &param: action.parameters) {
        os << param;
    }
    os << ")\n";

    os << "\t:precondition " << action.precondtions << "\n";
    if (!action.effect.conditions.empty() || !action.effect.conditions.empty()) {
        os << "\t:effect " << action.effect << "\n";
    }
    if (!action.observe.conditions.empty() || !action.observe.conditions.empty()) {
        os << "\t:observe " << action.observe << "\n";
    }

    os << ")\n";

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedAction &action) {
    os << "(:action " << action.name << "\n";
    os << "\t:parameters (";
    for (auto &param: action.parameters) {
        os << param;
    }
    os << ")\n";

    os << "\t:precondition " << action.precondtions << "\n";
    if (!action.effect.conditions.empty() || !action.effect.conditions.empty()) {
        os << "\t:effect " << action.effect << "\n";
    }
    if (!action.observe.conditions.empty() || !action.observe.conditions.empty()) {
        os << "\t:observe " << action.observe << "\n";
    }

    os << ")\n";

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::Condition &condConst) {
    pddl_lib::Condition cond = condConst;
    if (cond.op == pddl_lib::OPERATION::AND && cond.conditions.size() == 1) {
        if (auto pred = std::get_if<pddl_lib::Predicate>(&cond.conditions[0])) {
            os << *pred;
            return os;
        }
    }

    std::string op;
    std::string term;
    auto conditions = cond.conditions;
    if (cond.op == pddl_lib::OPERATION::AND) {
        os << "(and\n";
        term = "\n";
    } else if (cond.op == pddl_lib::OPERATION::OR) {
        os << "(or\n";
        term = "\n";
    } else if (cond.op == pddl_lib::OPERATION::NOT) {
        os << "(not ";
        term = " ";
    } else if (cond.op == pddl_lib::OPERATION::WHEN) {
        os << "(when ";
        term = " ";
    } else if (cond.op == pddl_lib::OPERATION::FORALL) {
        assert(cond.parameters.size() == 1);
        assert(cond.conditions.size() == 1);
        os << "(forall (" << cond.parameters[0].name << " - " << cond.parameters[0].type << ")\n";
        term = "\n";
    }
    for (const auto &c: conditions) {
        if (auto pred = std::get_if<pddl_lib::Predicate>(&c)) {
            os << *pred << term;
        } else {
            os << std::get<pddl_lib::Condition>(c) << term;
        }
    }
    os << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedCondition &condConst) {
    pddl_lib::InstantiatedCondition cond = condConst;
    if (cond.op == pddl_lib::OPERATION::AND && cond.conditions.size() == 1) {
        if (auto inst = std::get_if<pddl_lib::InstantiatedPredicate>(&cond.conditions[0])) {
            os << *inst;
            return os;
        }
    }

    std::string op;
    std::string term;
    auto conditions = cond.conditions;
    if (cond.op == pddl_lib::OPERATION::AND) {
        os << "(and\n";
        term = "\n";
    } else if (cond.op == pddl_lib::OPERATION::OR) {
        os << "(or\n";
        term = "\n";
    } else if (cond.op == pddl_lib::OPERATION::NOT) {
        os << "(not ";
        term = " ";
    } else if (cond.op == pddl_lib::OPERATION::WHEN) {
        os << "(when ";
        term = " ";
    } else if (cond.op == pddl_lib::OPERATION::FORALL) {
        os << "(forall (" << cond.parameters[0].name << " - " << cond.parameters[0].type << ")\n";
        term = "\n";
        //TODO refactor this
        std::function<void(pddl_lib::InstantiatedCondition &cond,
                           std::vector<pddl_lib::InstantiatedParameter>)> sub_pred;
        sub_pred = [&sub_pred](pddl_lib::InstantiatedCondition &cond,
                               std::vector<pddl_lib::InstantiatedParameter> params) {
            for (auto &c: cond.conditions) {
                if (auto pred = std::get_if<pddl_lib::InstantiatedPredicate>(&c)) {
                    for (auto &pred_param: pred->parameters) {
                        for (auto &param: params) {
                            if (pred_param.name == param.name) {
                                pred_param.type = param.type;
                            }
                        }
                    }
                } else {
                    sub_pred(std::get<pddl_lib::InstantiatedCondition>(c), params);
                }
            }
            if (cond.op == pddl_lib::OPERATION::FORALL) {
                assert(0); //TODO what is this
                params.push_back(cond.parameters[0]);
            }

        };
        sub_pred(cond, {cond.parameters[0]});

    }

    for (const auto &c: conditions) {
        if (auto inst = std::get_if<pddl_lib::InstantiatedPredicate>(&c)) {
            os << *inst << term;
        } else {
            os << std::get<pddl_lib::InstantiatedCondition>(c) << term;
        }
    }
    os << ")";

    return os;
}


std::ostream &operator<<(std::ostream &os, const pddl_lib::Predicate &pred) {
    os << "(" << pred.name;
    for (auto &param: pred.parameters) {
        os << " " << param.name;
    }
    os << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::InstantiatedPredicate &pred) {
    os << "(" << pred.name;
    for (auto &param: pred.parameters) {
        os << " " << param.name;
    }
    os << ")";

    return os;
}


std::ostream &operator<<(std::ostream &os,
                         const std::variant<pddl_lib::InstantiatedCondition, pddl_lib::InstantiatedPredicate> &cond) {
    if (auto inst = std::get_if<pddl_lib::InstantiatedCondition>(&cond)) {
        os << *inst;
    } else {
        os << std::get<pddl_lib::InstantiatedPredicate>(cond);
    }
    return os;
}


std::ostream &operator<<(std::ostream &os, const pddl_lib::Domain &domain) {
    os << fmt::format("(define (domain {})\n", domain.name);

    if (!domain.requirements.empty()) {
        os << "(:requirements\n";
        for (auto &req: domain.requirements) {
            os << fmt::format("\t{}\n", req);
        }
        os << ")\n";
    }


    if (!domain.types.empty()) {
        os << "(:types\n";
        for (auto &type: domain.types) {
            os << fmt::format("\t{}\n", type);
        }
        os << ")\n";
    }

    os << "(:predicates\n";
    for (auto &pred: domain.predicates) {
        os << "\t";
        os << "(" << pred.name;
        for (auto &param: pred.parameters) {
            os << param;
        }
        os << ")";
        os << "\n";
    }
    os << ")\n";

    for (auto &action: domain.actions) {
        os << action;// << "\n";
    }

    os << ")\n";

    return os;
}

std::ostream &operator<<(std::ostream &os, const pddl_lib::Problem &problem) {
    os << fmt::format("(define (problem {})\n", problem.name);
    os << fmt::format("(:domain {})\n", problem.domain);

    os << "(:objects\n";
    std::unordered_map<std::string, std::vector<std::string>> type_to_name_map;
    for (auto &obj: problem.objects) {
        type_to_name_map[obj.type].push_back(obj.name);
    }
    for (auto [type, objs]: type_to_name_map) {
        for (const auto &obj: objs) {
            os << obj << " ";
        }
        os << "- " << type << "\n";
    }
    os << ")\n";

    os << "(:init\n";
    for (auto &pred: problem.init) {
        os << "\t";
        os << "(" << pred.name;
        for (auto &param: pred.parameters) {
            os << param;
        }
        os << ")";
        os << "\n";
    }

    for (auto &pred: problem.unknowns) {
        os << "\t";
        os << "(unknown ";
        os << "(" << pred.name;
        for (auto &param: pred.parameters) {
            os << param;
        }
        os << ")";
        os << ")";
        os << "\n";
    }

    for (auto &cond: problem.constraints) {
        os << "\t";
        if (cond.constraint == pddl_lib::CONSTRAINTS::ONEOF) {
            os << "(oneof";
        } else if (cond.constraint == pddl_lib::CONSTRAINTS::OR) {
            os << "(or";
        }
        for (const auto &c: cond.condition.conditions) {
            if (auto inst = std::get_if<pddl_lib::InstantiatedPredicate>(&c)) {
                os << " " << *inst;
            } else {
                os << " " << std::get<pddl_lib::InstantiatedCondition>(c);
            }

        }
        os << ")";
        os << "\n";
    }

    os << ")\n";

    os << "(:goal\n";
    os << problem.goal;
    os << ")\n";

    os << ")\n";

    return os;
}