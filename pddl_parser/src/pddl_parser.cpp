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

std::vector<std::string>
parseVector(const std::string_view &section, std::unordered_set<char> skip_symbols) {
    std::vector<std::string> out;
    int ind = 0;
    while (ind < section.size()) {
        while (skip_symbols.find(section[ind]) != skip_symbols.end()) {
            ind++;
        }
        int ind1 = ind;
        int num_open = 0;
        while (num_open != 0 || (skip_symbols.find(section[ind]) == skip_symbols.end() && ind < section.size())) {
            num_open += section[ind] == '(';
            num_open -= section[ind] == ')';
            ind++;
        }
        if (ind - ind1 > 0) {
            out.emplace_back(section.substr(ind1, (ind - ind1)));
        }
    }
    return out;
}


std::optional<std::string> parseInit(const std::string &content, Domain &domain) {
    std::string_view remaining(content);
    std::string_view current;
    std::string_view section;
    std::string matched_token;
    int line_num = 0;

    std::tie(current, remaining, matched_token) = getnext(remaining, {"(define"});
    if (current != "(define") {
        return fmt::format("ERROR line {}: start of file does not begin with '(define'", line_num);
    }

    std::tie(section, remaining) = getNextParen(remaining);
    std::tie(current, section, matched_token) = getnext(section, {" "});
    domain.name = section;

    std::tie(section, remaining) = getNextParen(remaining);
    std::tie(current, section, matched_token) = getnext(section, {":requirements"});
    if (current != ":requirements") {
        return fmt::format("ERROR line {}: missing ':requirements' keyword", line_num);
    }
    auto strings = parseVector(section, {'\t', '\n', ' '});
    for (const auto &str: strings) {
        domain.requirements.insert(str);
    }

    std::tie(section, remaining) = getNextParen(remaining);
    std::tie(current, section, matched_token) = getnext(section, {":types"});
    if (current != ":types") {
        return fmt::format("ERROR line {}: missing ':requirements' keyword", line_num);
    }
    strings = parseVector(section, {'\t', '\n', ' '});
    for (const auto &str: strings) {
        domain.types.insert(str);
    }

    std::tie(section, remaining) = getNextParen(remaining);
    std::tie(current, section, matched_token) = getnext(section, {":predicates"});
    if (current != ":predicates") {
        return fmt::format("ERROR line {}: missing ':requirements' keyword", line_num);
    }
    strings = parseVector(section, {'\t', '\n', ' '});
    for (const auto &str: strings) {
        Predicate pred = parse_predicate(str);
        domain.predicates.insert(pred);
    }
    int o = 0;




//    std::getline(ss, line, '(');
//    if (!line.empty()) {
//        return fmt::format("ERROR line {}: missing '('", line_num);
//    }
//    increment_line(line_num, line);
//
//    std::getline(ss, line, ' ');
//    if (line != "domain"){
//        return fmt::format("ERROR line {}: keyword 'domain' missing", line_num);
//    }
//    increment_line(line_num, line);
//
//    std::getline(ss, line, ')');
//    if (line.empty()){
//        return fmt::format("ERROR line {}: domain name cannot be empty", line_num);
//    }
//    increment_line(line_num, line);
//    domain.name = line;
//
//    std::getline(ss, line, '(');
//    increment_line(line_num, line);
//
//    std::getline(ss, line, ' ');
//    if (line != ":requirements"){
//        return fmt::format("ERROR line {}: expected '(:requirements ...)' ", line_num);
//    }
//    increment_line(line_num, line);
//
//    while (peek_next_token(ss, {')', ' '}) == ' '){
//        // TODO check requirements is  pddl
//        std::getline(ss, line, ' ');
//        increment_line(line_num, line);
//        domain.requirements.insert(line);
//    }
//    std::getline(ss, line, ')');
//    increment_line(line_num, line);
//    domain.requirements.insert(line);
//
//
//    std::getline(ss, line, ')');
//    if (line != ":requirements"){
//        return fmt::format("ERROR line {}: expected '(:requirements ...)' ", line_num);
//    }
//    increment_line(line_num, line);
//
//
//
//    while (!ss.eof()) {
//        std::getline(ss, line, '(');
//        std::cout << line << std::endl;
//        std::getline(ss, line, ')');
//        std::cout << line << std::endl;
//    }
//
//
//    ss << ss2.str();

    return {};
}


Domain parse_domain(const std::string &content) {
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

Predicate parse_predicate(const std::string &content) {
    auto [section, remaining] = getNextParen(content);
    auto str = parseVector(section, {'\t', '\n', ' '});
    auto pred = Predicate();
    pred.name = str[0];

    int ind = 1;
    while (ind < str.size()){
        std::vector<std::string> names;
        while(str[ind][0]=='?' && ind < str.size()){
            names.push_back(str[ind]);
            ind++;
        }
        if (str[ind][0] == '-'){
            ind++;
            auto type = str[ind];
            for (const auto& name : names){
                auto param = Parameter();
                param.name = name;
                param.type = type;
                pred.parameters.push_back(param);
            }
        }
        ind++;
    }


    return pred;
}

Action parse_action(const std::string &content) {
    return {};
}

Condition parse_precondition(const std::string &content) {
    return {};
}

Condition parse_parameters(const std::string &content) {
    return {};
}

Condition parse_observe(const std::string &content) {
    return {};
}

Condition parse_effect(const std::string &content) {
    return {};
}

