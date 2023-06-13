#include <sstream>
#include <iostream>
#include <optional>
#include "pddl_parser/pddl_parser.hpp"
#include <fmt/core.h>
#include <functional>


enum SECTION {
    INIT, PREDICATES, ACTIONS
};

//void increment_line(std::string_view line, int &line_num) {
//    for (auto symbol: line) {
//        line_num += symbol == '\n';
//    }
//}

//char peek_next_token(std::stringstream &ss, const std::vector<char> & tokens) {
//    std::string line;
//    std::pair<char, int> min_tok = {'\0', 99999999};
//    for (auto token : tokens) {
//        std::getline(ss, line, token);
//        if (min_tok.second > line.size()){
//            min_tok.first = token;
//        }
//        ss << line << token;
//    }
//    std::getline(ss, line, min_tok.first);
//    return min_tok.first;
//}


std::vector<Parameter> parse_params(std::vector<std::string_view> str);


std::tuple<std::string_view, std::string_view, std::string>
getnext(std::string_view content_view, const std::vector<std::string> &tokens) {
    auto ind = 0ul;
    auto len = content_view.size();
    std::string matched_token;

    auto matched = [content_view, tokens, &matched_token](unsigned long ind) {
        for (auto token: tokens) {
            if (content_view[ind] == token[0]) {
                bool ret = true;
                auto i = 0ul;
                while (i < token.size() && i + ind < content_view.size()) {
                    ret &= token[i] == content_view[ind + i];
                    i++;
                }
                if (ret) {
                    matched_token = token;
                    return ret;
                }
            }
        }
        return false;
    };


    while (!matched(ind) && ind < len) {
        ind++;
    }
    for (auto i = 0ul; i < matched_token.size(); i++) {
        ind++;
    }
    if (ind == len) {
        return {content_view.substr(0, ind), {}, matched_token};
    }
    return {content_view.substr(0, ind), content_view.substr(ind, len - ind), matched_token};
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

//void parseRequirements(Domain &domain, const std::string_view &section) {
//    int ind = 0;
//    while (ind < section.size()) {
//        while (section[ind] == ' ' || section[ind] == '\n' || section[ind] == '\t') {
//            ind++;
//        }
//        int ind1 = ind;
//        while (section[ind] != ' ' && section[ind] != '\n' && section[ind] != '\t' && ind < section.size()) {
//            ind++;
//        }
//        if (ind - ind1 > 0) {
//            domain.requirements.insert(std::string(section.substr(ind1, (ind - ind1))));
//        }
//    }
//}

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
                action.precondtions.conditions.emplace_back(pred.value());
            }
            ind++;
        } else if (strings[ind] == ":effect") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]))) {
                action.effect = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]))) {
                action.effect.conditions.emplace_back(pred.value());
            }
            ind++;
        } else if (strings[ind] == ":observe") {
            ind++;
            if (auto cond = parse_condition(std::string(strings[ind]))) {
                action.observe = cond.value();
            } else if (auto pred = parse_predicate(std::string(strings[ind]))) {
                action.observe.conditions.emplace_back(pred.value());
            }
            ind++;
        }
    }


//    while (!remaining.empty()) {
//        std::tie(current, remaining, matched_token) = getnext(remaining, {":action"});
////        if (current != ":action") {
////            return fmt::format("ERROR line {}: missing ':action' keyword", get_line_num(content, current));
////        }
//        //parseAction
//        auto strings = parseVector(remaining, {'\t', '\n', ' '});
//        int ind = 0;
//        action.name = strings[ind];
//        ind++;
//        while (ind < strings.size()) {
//            if (strings[ind] == ":parameters") {
//                ind++;
//                auto [section2, remaining2] = getNextParen(strings[ind]);
//                strings = parseVector(section2, {'\t', '\n', ' '});
////                action.parameters = parse_params(strings); TODO broken
//            } else if (strings[ind] == ":precondition") {
//                ind++;
//                auto [section2, remaining2] = getNextParen(strings[ind]);
//                if (auto cond = parse_condition(std::string(section2))) {
//                    action.precondtions = cond.value();
//                } else {
//                    return {};
//                }
//            } else if (strings[ind] == ":effect") {
//
//            } else if (strings[ind] == ":observe") {
//
//            }
//
//            ind++;
//        }
//
////    strings = parseVector(section, {'\t', '\n', ' '});
////    for (const auto &str: strings) {
////        Predicate pred = parse_predicate(str);
////        domain.predicates.insert(pred);
////    }
//        std::tie(remaining, remaining) = getNextParen(remaining);
//        int o = 0;
//    }

    return action;
}


std::optional<Domain> parse_domain(const std::string &content) {
    std::stringstream ss;
    ss << content;

    std::string line;
//    while (!ss.eof()){
//        ss >> line;
//        std::cout << line << "\n";
//    }
    Domain domain;
    auto sec = SECTION::INIT;
    int line_num = 1;


    if (auto error = checkParens(content)) {
        std::cout << error.value() << std::endl;
    }

    if (auto error = parseInit(content, domain)) {
        std::cout << error.value() << std::endl;
    }


    while (!ss.eof()) {


        std::getline(ss, line, '(');
        std::cout << line << std::endl;
        std::getline(ss, line, ')');
        std::cout << line << std::endl;


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


    if (strings[0] == "and") {
        cond.operation = OPERATION::AND;
    } else if (strings[0] == "or") {
        cond.operation = OPERATION::OR;
    } else if (strings[0] == "not") {
        cond.operation = OPERATION::NOT;
    } else if (strings[0] == "forall") {
        cond.operation = OPERATION::FORALL;
    } else {
        return {};
    }
    for (int i = 1; i < strings.size(); i++) {
        if (auto cond2 = parse_condition(std::string(strings[i]))) {
            cond.conditions.emplace_back(cond2.value());
        }
    }

    return cond;
}
