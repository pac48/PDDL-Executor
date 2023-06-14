#include <sstream>
#include <iostream>
#include <optional>
#include "pddl_parser/pddl_parser.hpp"
#include <fmt/core.h>
#include <functional>


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
        domain.requirements.insert(std::string(str));
    }
    ind++;


    std::tie(section, remaining) = getNextParen(strings[ind]);
    substrings = parseVector(section, {'\t', '\n', ' '});
    if (substrings[0] != ":types") {
        return fmt::format("ERROR line {}: missing ':types' keyword", get_line_num(content, substrings[0]));
    }
    for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
        domain.types.insert(std::string(str));
    }
    ind++;

    std::tie(section, remaining) = getNextParen(strings[ind]);
    substrings = parseVector(section, {'\t', '\n', ' '});
    if (substrings[0] != ":predicates") {
        return fmt::format("ERROR line {}: missing ':predicates' keyword", get_line_num(content, substrings[0]));
    }
    for (const auto &str: std::vector<std::string_view>(substrings.begin() + 1, substrings.end())) {
        if (auto pred = parse_predicate(std::string(str))) {
            domain.predicates.insert(pred.value());
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
            ind++;
        } else if (strings[ind] == ":precondition") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]))) {
                action.precondtions = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]))) {
                action.precondtions.predicates.emplace_back(pred.value());
            }
            ind++;
        } else if (strings[ind] == ":effect") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]))) {
                action.effect = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]))) {
                action.effect.predicates.emplace_back(pred.value());
            }
            ind++;
        } else if (strings[ind] == ":observe") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]))) {
                action.observe = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]))) {
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

std::optional<Predicate> parse_predicate(const std::string &content) {
    auto [section, remaining] = getNextParen(content);
    auto str = parseVector(section, {'\t', '\n', ' '});
    auto pred = Predicate();
    pred.name = str[0];

    pred.parameters = parse_params(std::vector<std::string_view>(str.begin() + 1, str.end()));


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


std::optional<Condition> parse_condition(const std::string &content) {
    std::string_view section;
    std::string_view remaining;
    Condition cond;

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
        ind++;
    } else {
        return {};
    }
    for (int i = ind; i < strings.size(); i++) {
        if (auto cond2 = parse_condition(std::string(strings[i]))) {
            cond.conditions.emplace_back(cond2.value());
        } else if (auto pred = parse_predicate(std::string(strings[i]))) {
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
        os << param;
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
