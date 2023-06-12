#include <sstream>
#include <iostream>
#include <optional>
#include "pddl_parser/pddl_parser.hpp"
#include <fmt/core.h>


enum SECTION {
    INIT, PREDICATES, ACTIONS
};

void increment_line(int &line_num, const std::string & line) {
    for (auto symbol: line) {
        line_num += symbol == '\n';
    }
}

char peek_next_token(std::stringstream &ss, const std::vector<char> & tokens) {
    std::string line;
    std::pair<char, int> min_tok = {'\0', 99999999};
    for (auto token : tokens) {
        std::getline(ss, line, token);
        if (min_tok.second > line.size()){
            min_tok.first = token;
        }
        ss << line << token;
    }
    std::getline(ss, line, min_tok.first);
    return min_tok.first;
}

std::optional<std::string> parseInit(std::stringstream &ss, Domain &domain, int &line_num) {
    std::stringstream ss2;
    // make a copy of file
//    ss2 << ss.str();
    std::string line;

    std::getline(ss, line, '(');
    if (!line.empty()) {
        return fmt::format("ERROR line {}: start of file does not begin with a '('", line_num);
    }
    increment_line(line_num, line);

    std::getline(ss, line, ' ');
    if (line != "define") {
        return fmt::format("ERROR line {}: keyword 'define' missing", line_num);
    }
    increment_line(line_num, line);

    std::getline(ss, line, '(');
    if (!line.empty()) {
        return fmt::format("ERROR line {}: missing '('", line_num);
    }
    increment_line(line_num, line);

    std::getline(ss, line, ' ');
    if (line != "domain"){
        return fmt::format("ERROR line {}: keyword 'domain' missing", line_num);
    }
    increment_line(line_num, line);

    std::getline(ss, line, ')');
    if (line.empty()){
        return fmt::format("ERROR line {}: domain name cannot be empty", line_num);
    }
    increment_line(line_num, line);
    domain.name = line;

    std::getline(ss, line, '(');
    increment_line(line_num, line);

    std::getline(ss, line, ' ');
    if (line != ":requirements"){
        return fmt::format("ERROR line {}: expected '(:requirements ...)' ", line_num);
    }
    increment_line(line_num, line);

    while (peek_next_token(ss, {')', ' '}) == ' '){
        // TODO check requirements is  pddl
        std::getline(ss, line, ' ');
        increment_line(line_num, line);
        domain.requirements.insert(line);
    }
    std::getline(ss, line, ')');
    increment_line(line_num, line);
    domain.requirements.insert(line);


    std::getline(ss, line, ')');
    if (line != ":requirements"){
        return fmt::format("ERROR line {}: expected '(:requirements ...)' ", line_num);
    }
    increment_line(line_num, line);



    while (!ss.eof()) {
        std::getline(ss, line, '(');
        std::cout << line << std::endl;
        std::getline(ss, line, ')');
        std::cout << line << std::endl;
    }


    ss << ss2.str();

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

    auto error = parseInit(ss, domain, line_num);


    while (!ss.eof()) {


        std::getline(ss, line, '(');
        std::cout << line << std::endl;
        std::getline(ss, line, ')');
        std::cout << line << std::endl;


    }

    return domain;
}

Predicate parse_predicate(const std::string &content) {
    return {};
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
