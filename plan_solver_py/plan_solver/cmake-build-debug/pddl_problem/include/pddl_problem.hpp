#include <cstddef>
#include <unordered_set>
#include <array>
#include <tuple>
#include <cstring>
#include <stdexcept>
#include "pddl_parser/pddl_parser.hpp"

#pragma once

namespace pddl_lib {

    unsigned int subgraph_counter = 0;
    struct KBState {
        KBState(){
            memset(data, 0, sizeof(data));
        }
        unsigned char data[216];
        unsigned int depth = 0;
        unsigned int action = 0;
        int goal_dist = -1;
        char valid = 0;
        KBState* associated_state = {};
        std::vector<KBState*> children = {};
        std::vector<KBState*> parents = {};
//        std::vector<std::string> action_name_; //TODO remove after done debugging
//        unsigned int subgraph = 0;

        bool operator==(const KBState & other) const{
            return std::memcmp(data, other.data, 216) == 0; // && subgraph == other.subgraph;
        }
//        bool operator==(const KBState* & other) const{
//            return std::memcmp(data, other->data, 216) == 0; // && subgraph == other.subgraph;
//        }
    };

    std::function<std::pair<bool,bool>(KBState &)> create_constraint(const Constraint& constraint, std::unordered_map<std::string, unsigned int> func_map){
        if (constraint.constraint == CONSTRAINTS::ONEOF){
            std::vector<unsigned int> inds;
            for (const auto& c : constraint.condition.conditions){
                if (auto pred = std::get_if<InstantiatedPredicate>(&c)){
                    std::stringstream ss;
                    ss << *pred;
                    inds.push_back(func_map[ss.str()]);
                } else {
                    throw std::runtime_error("ONEOF constraint must only contain predicates");
                }
            }
            return [inds](KBState & state){
                unsigned int num_unknowns = 0;
                unsigned int num_true = 0;
                for (const auto& ind : inds){
                    num_true += state.data[ind]==1;
                    num_unknowns += state.data[ind]==2;
                }
                if (num_true > 1 || (num_true == 0 && num_unknowns == 0)) {
                    return std::make_pair(false, false);
                }

                bool modified = false;
                if (num_true==1 && num_unknowns > 0){
                    for (const auto& ind : inds){
                        state.data[ind] = state.data[ind]*(state.data[ind]==1);
                    }
                    modified = true;
                }
                if (num_true==0 && num_unknowns==1){
                    for (const auto& ind : inds){
                        state.data[ind] = state.data[ind]==1 || state.data[ind]==2;
                    }
                    modified = true;
                }
                return std::make_pair(true, modified);
            };

        } else if (constraint.constraint == CONSTRAINTS::OR){
            assert(constraint.condition.conditions.size() == 2);
            std::vector<std::pair<unsigned int, unsigned char>> conds;
            for (const auto & c  : constraint.condition.conditions) {
                if (auto pred = std::get_if<InstantiatedPredicate>(&c)){
                    std::stringstream ss;
                    ss << *pred;
                    conds.push_back({func_map[ss.str()], 1});
                } else {
                    auto cond = std::get<InstantiatedCondition>(c);
                    assert(cond.op == OPERATION::NOT);
                    assert(cond.conditions.size()==1);
                    std::stringstream ss;
                    ss << std::get<InstantiatedPredicate>(cond.conditions[0]);
                    conds.push_back({func_map[ss.str()], 0});
                }
            }
            assert(conds.size()==2);

            return [conds](KBState & state){
                const auto & cond_1 = conds[0];
                const auto & cond_2 = conds[1];
                if (state.data[cond_1.first] != cond_1.second && state.data[cond_1.first] != 2
                    && state.data[cond_2.first] != cond_2.second && state.data[cond_2.first] != 2){
                    assert(0);
//                    return std::make_pair(false,false);
                }
//                if (state.data[cond_1.first] == cond_1.second && state.data[cond_1.first] != 2
//                    && state.data[cond_2.first] == cond_2.second && state.data[cond_2.first] != 2){
//                    return std::make_pair(false,false);
//                }
                bool modified = false;
                if (state.data[cond_1.first] == 2 && state.data[cond_2.first] != 2 && state.data[cond_2.first] != cond_2.second){
                    state.data[cond_1.first] = cond_1.second;
                    modified = true;
                } else if (state.data[cond_2.first] == 2 && state.data[cond_1.first] != 2 && state.data[cond_1.first] != cond_1.second){
                    state.data[cond_2.first] = cond_2.second;
                    modified = true;
                }

                if (state.data[cond_1.first] == 2 && state.data[cond_2.first] != 2 && state.data[cond_2.first] == cond_2.second){
                    state.data[cond_1.first] = 1-cond_1.second;
                    modified = true;
                } else if (state.data[cond_2.first] == 2 && state.data[cond_1.first] != 2 && state.data[cond_1.first] == cond_1.second){
                    state.data[cond_2.first] = 1-cond_2.second;
                    modified = true;
                }

                return std::make_pair(true, modified);
            };
        } else{
            throw std::runtime_error("Constraint not supported");
        }
    }

    std::function<bool(const KBState &)> create_goal(const InstantiatedCondition& goal, std::unordered_map<std::string, unsigned int> func_map){
        std::vector<std::pair<unsigned int, unsigned char>> conds;
        for (const auto & c  : goal.conditions){
            if (auto pred = std::get_if<InstantiatedPredicate>(&c)){
                std::stringstream ss;
                ss << *pred;
                conds.push_back({func_map[ss.str()], 1});
            } else{
                auto cond = std::get<InstantiatedCondition>(c);
                assert(cond.op == OPERATION::NOT);
                assert(cond.conditions.size()==1);
                std::stringstream ss;
                ss << std::get<InstantiatedPredicate>(cond.conditions[0]);
                conds.push_back({func_map[ss.str()], 0});
            }

        }

        return [conds](const KBState & state){
            bool val = true;
            for (const auto& c : conds ){
                val &= state.data[c.first]==c.second;
            }
            return val;
        };

    }


    std::tuple<KBState, std::array<KBState, 253>,
    std::vector<std::function<std::pair<bool,bool>(KBState &)>>, std::function<bool (const KBState&)>> initialize_problem(const std::string& problem_str){
        pddl_lib::KBState state{};
        std::array<KBState, 253> new_states{};
        memset(new_states.data(), 0, sizeof(new_states));
        auto problem = pddl_lib::parse_problem(problem_str).value();
        std::unordered_map<std::string, unsigned int> func_map;
        std::function<bool (const KBState&)> check_goal;
        std::vector<std::function<std::pair<bool,bool> (KBState &)>> constraints{};
        func_map["(next_time t4 t4)"] = 0;
func_map["(next_time t4 t1)"] = 1;
func_map["(next_time t4 t2)"] = 2;
func_map["(next_time t4 t3)"] = 3;
func_map["(next_time t4 t5)"] = 4;
func_map["(next_time t1 t4)"] = 5;
func_map["(next_time t1 t1)"] = 6;
func_map["(next_time t1 t2)"] = 7;
func_map["(next_time t1 t3)"] = 8;
func_map["(next_time t1 t5)"] = 9;
func_map["(next_time t2 t4)"] = 10;
func_map["(next_time t2 t1)"] = 11;
func_map["(next_time t2 t2)"] = 12;
func_map["(next_time t2 t3)"] = 13;
func_map["(next_time t2 t5)"] = 14;
func_map["(next_time t3 t4)"] = 15;
func_map["(next_time t3 t1)"] = 16;
func_map["(next_time t3 t2)"] = 17;
func_map["(next_time t3 t3)"] = 18;
func_map["(next_time t3 t5)"] = 19;
func_map["(next_time t5 t4)"] = 20;
func_map["(next_time t5 t1)"] = 21;
func_map["(next_time t5 t2)"] = 22;
func_map["(next_time t5 t3)"] = 23;
func_map["(next_time t5 t5)"] = 24;
func_map["(DetectEatingFood_enabled)"] = 25;
func_map["(call_person_not_eating_food_constraint caregiver_call nathan)"] = 26;
func_map["(call_person_not_eating_food_constraint caregiver_call_guide nathan)"] = 27;
func_map["(call_person_location_constraint caregiver_call nathan couch)"] = 28;
func_map["(call_person_location_constraint caregiver_call nathan kitchen)"] = 29;
func_map["(call_person_location_constraint caregiver_call_guide nathan couch)"] = 30;
func_map["(call_person_location_constraint caregiver_call_guide nathan kitchen)"] = 31;
func_map["(current_time t4)"] = 32;
func_map["(current_time t1)"] = 33;
func_map["(current_time t2)"] = 34;
func_map["(current_time t3)"] = 35;
func_map["(current_time t5)"] = 36;
func_map["(executed_reminder automated_reminder)"] = 37;
func_map["(executed_reminder guide_reminder_2)"] = 38;
func_map["(executed_reminder guide_reminder_1)"] = 39;
func_map["(executed_reminder recorded_reminder)"] = 40;
func_map["(time_critical)"] = 41;
func_map["(call_blocks_call caregiver_call caregiver_call)"] = 42;
func_map["(call_blocks_call caregiver_call caregiver_call_guide)"] = 43;
func_map["(call_blocks_call caregiver_call_guide caregiver_call)"] = 44;
func_map["(call_blocks_call caregiver_call_guide caregiver_call_guide)"] = 45;
func_map["(valid_call_message caregiver_call guide_2_msg)"] = 46;
func_map["(valid_call_message caregiver_call guide_1_msg)"] = 47;
func_map["(valid_call_message caregiver_call automated_msg)"] = 48;
func_map["(valid_call_message caregiver_call recorded_msg)"] = 49;
func_map["(valid_call_message caregiver_call call_caregiver_msg)"] = 50;
func_map["(valid_call_message caregiver_call call_caregiver_guide_msg)"] = 51;
func_map["(valid_call_message caregiver_call_guide guide_2_msg)"] = 52;
func_map["(valid_call_message caregiver_call_guide guide_1_msg)"] = 53;
func_map["(valid_call_message caregiver_call_guide automated_msg)"] = 54;
func_map["(valid_call_message caregiver_call_guide recorded_msg)"] = 55;
func_map["(valid_call_message caregiver_call_guide call_caregiver_msg)"] = 56;
func_map["(valid_call_message caregiver_call_guide call_caregiver_guide_msg)"] = 57;
func_map["(DetectTakingMedicine_enabled)"] = 58;
func_map["(executed_call caregiver_call)"] = 59;
func_map["(executed_call caregiver_call_guide)"] = 60;
func_map["(person_at t4 nathan couch)"] = 61;
func_map["(person_at t4 nathan kitchen)"] = 62;
func_map["(person_at t1 nathan couch)"] = 63;
func_map["(person_at t1 nathan kitchen)"] = 64;
func_map["(person_at t2 nathan couch)"] = 65;
func_map["(person_at t2 nathan kitchen)"] = 66;
func_map["(person_at t3 nathan couch)"] = 67;
func_map["(person_at t3 nathan kitchen)"] = 68;
func_map["(person_at t5 nathan couch)"] = 69;
func_map["(person_at t5 nathan kitchen)"] = 70;
func_map["(valid_reminder_message automated_reminder guide_2_msg)"] = 71;
func_map["(valid_reminder_message automated_reminder guide_1_msg)"] = 72;
func_map["(valid_reminder_message automated_reminder automated_msg)"] = 73;
func_map["(valid_reminder_message automated_reminder recorded_msg)"] = 74;
func_map["(valid_reminder_message automated_reminder call_caregiver_msg)"] = 75;
func_map["(valid_reminder_message automated_reminder call_caregiver_guide_msg)"] = 76;
func_map["(valid_reminder_message guide_reminder_2 guide_2_msg)"] = 77;
func_map["(valid_reminder_message guide_reminder_2 guide_1_msg)"] = 78;
func_map["(valid_reminder_message guide_reminder_2 automated_msg)"] = 79;
func_map["(valid_reminder_message guide_reminder_2 recorded_msg)"] = 80;
func_map["(valid_reminder_message guide_reminder_2 call_caregiver_msg)"] = 81;
func_map["(valid_reminder_message guide_reminder_2 call_caregiver_guide_msg)"] = 82;
func_map["(valid_reminder_message guide_reminder_1 guide_2_msg)"] = 83;
func_map["(valid_reminder_message guide_reminder_1 guide_1_msg)"] = 84;
func_map["(valid_reminder_message guide_reminder_1 automated_msg)"] = 85;
func_map["(valid_reminder_message guide_reminder_1 recorded_msg)"] = 86;
func_map["(valid_reminder_message guide_reminder_1 call_caregiver_msg)"] = 87;
func_map["(valid_reminder_message guide_reminder_1 call_caregiver_guide_msg)"] = 88;
func_map["(valid_reminder_message recorded_reminder guide_2_msg)"] = 89;
func_map["(valid_reminder_message recorded_reminder guide_1_msg)"] = 90;
func_map["(valid_reminder_message recorded_reminder automated_msg)"] = 91;
func_map["(valid_reminder_message recorded_reminder recorded_msg)"] = 92;
func_map["(valid_reminder_message recorded_reminder call_caregiver_msg)"] = 93;
func_map["(valid_reminder_message recorded_reminder call_caregiver_guide_msg)"] = 94;
func_map["(message_given guide_2_msg)"] = 95;
func_map["(message_given guide_1_msg)"] = 96;
func_map["(message_given automated_msg)"] = 97;
func_map["(message_given recorded_msg)"] = 98;
func_map["(message_given call_caregiver_msg)"] = 99;
func_map["(message_given call_caregiver_guide_msg)"] = 100;
func_map["(bed_location couch)"] = 101;
func_map["(bed_location kitchen)"] = 102;
func_map["(medicine_taken_success)"] = 103;
func_map["(reminder_person_not_eating_food_constraint automated_reminder nathan)"] = 104;
func_map["(reminder_person_not_eating_food_constraint guide_reminder_2 nathan)"] = 105;
func_map["(reminder_person_not_eating_food_constraint guide_reminder_1 nathan)"] = 106;
func_map["(reminder_person_not_eating_food_constraint recorded_reminder nathan)"] = 107;
func_map["(person_on_ground t4)"] = 108;
func_map["(person_on_ground t1)"] = 109;
func_map["(person_on_ground t2)"] = 110;
func_map["(person_on_ground t3)"] = 111;
func_map["(person_on_ground t5)"] = 112;
func_map["(robot_at couch)"] = 113;
func_map["(robot_at kitchen)"] = 114;
func_map["(door_location couch)"] = 115;
func_map["(door_location kitchen)"] = 116;
func_map["(person_eating_food t4)"] = 117;
func_map["(person_eating_food t1)"] = 118;
func_map["(person_eating_food t2)"] = 119;
func_map["(person_eating_food t3)"] = 120;
func_map["(person_eating_food t5)"] = 121;
func_map["(person_at_success nathan couch)"] = 122;
func_map["(person_at_success nathan kitchen)"] = 123;
func_map["(person_taking_medicine t4)"] = 124;
func_map["(person_taking_medicine t1)"] = 125;
func_map["(person_taking_medicine t2)"] = 126;
func_map["(person_taking_medicine t3)"] = 127;
func_map["(person_taking_medicine t5)"] = 128;
func_map["(person_eating t4)"] = 129;
func_map["(person_eating t1)"] = 130;
func_map["(person_eating t2)"] = 131;
func_map["(person_eating t3)"] = 132;
func_map["(person_eating t5)"] = 133;
func_map["(GiveReminder_enabled)"] = 134;
func_map["(reminder_blocks_call automated_reminder caregiver_call)"] = 135;
func_map["(reminder_blocks_call automated_reminder caregiver_call_guide)"] = 136;
func_map["(reminder_blocks_call guide_reminder_2 caregiver_call)"] = 137;
func_map["(reminder_blocks_call guide_reminder_2 caregiver_call_guide)"] = 138;
func_map["(reminder_blocks_call guide_reminder_1 caregiver_call)"] = 139;
func_map["(reminder_blocks_call guide_reminder_1 caregiver_call_guide)"] = 140;
func_map["(reminder_blocks_call recorded_reminder caregiver_call)"] = 141;
func_map["(reminder_blocks_call recorded_reminder caregiver_call_guide)"] = 142;
func_map["(food_eaten_success)"] = 143;
func_map["(MakeCall_enabled)"] = 144;
func_map["(message_given_success guide_2_msg)"] = 145;
func_map["(message_given_success guide_1_msg)"] = 146;
func_map["(message_given_success automated_msg)"] = 147;
func_map["(message_given_success recorded_msg)"] = 148;
func_map["(message_given_success call_caregiver_msg)"] = 149;
func_map["(message_given_success call_caregiver_guide_msg)"] = 150;
func_map["(reminder_blocks_reminder automated_reminder automated_reminder)"] = 151;
func_map["(reminder_blocks_reminder automated_reminder guide_reminder_2)"] = 152;
func_map["(reminder_blocks_reminder automated_reminder guide_reminder_1)"] = 153;
func_map["(reminder_blocks_reminder automated_reminder recorded_reminder)"] = 154;
func_map["(reminder_blocks_reminder guide_reminder_2 automated_reminder)"] = 155;
func_map["(reminder_blocks_reminder guide_reminder_2 guide_reminder_2)"] = 156;
func_map["(reminder_blocks_reminder guide_reminder_2 guide_reminder_1)"] = 157;
func_map["(reminder_blocks_reminder guide_reminder_2 recorded_reminder)"] = 158;
func_map["(reminder_blocks_reminder guide_reminder_1 automated_reminder)"] = 159;
func_map["(reminder_blocks_reminder guide_reminder_1 guide_reminder_2)"] = 160;
func_map["(reminder_blocks_reminder guide_reminder_1 guide_reminder_1)"] = 161;
func_map["(reminder_blocks_reminder guide_reminder_1 recorded_reminder)"] = 162;
func_map["(reminder_blocks_reminder recorded_reminder automated_reminder)"] = 163;
func_map["(reminder_blocks_reminder recorded_reminder guide_reminder_2)"] = 164;
func_map["(reminder_blocks_reminder recorded_reminder guide_reminder_1)"] = 165;
func_map["(reminder_blocks_reminder recorded_reminder recorded_reminder)"] = 166;
func_map["(call_not_person_location_constraint caregiver_call nathan couch)"] = 167;
func_map["(call_not_person_location_constraint caregiver_call nathan kitchen)"] = 168;
func_map["(call_not_person_location_constraint caregiver_call_guide nathan couch)"] = 169;
func_map["(call_not_person_location_constraint caregiver_call_guide nathan kitchen)"] = 170;
func_map["(success)"] = 171;
func_map["(traversable couch couch)"] = 172;
func_map["(traversable couch kitchen)"] = 173;
func_map["(traversable kitchen couch)"] = 174;
func_map["(traversable kitchen kitchen)"] = 175;
func_map["(reminder_not_person_location_constraint automated_reminder nathan couch)"] = 176;
func_map["(reminder_not_person_location_constraint automated_reminder nathan kitchen)"] = 177;
func_map["(reminder_not_person_location_constraint guide_reminder_2 nathan couch)"] = 178;
func_map["(reminder_not_person_location_constraint guide_reminder_2 nathan kitchen)"] = 179;
func_map["(reminder_not_person_location_constraint guide_reminder_1 nathan couch)"] = 180;
func_map["(reminder_not_person_location_constraint guide_reminder_1 nathan kitchen)"] = 181;
func_map["(reminder_not_person_location_constraint recorded_reminder nathan couch)"] = 182;
func_map["(reminder_not_person_location_constraint recorded_reminder nathan kitchen)"] = 183;
func_map["(used_move t4)"] = 184;
func_map["(used_move t1)"] = 185;
func_map["(used_move t2)"] = 186;
func_map["(used_move t3)"] = 187;
func_map["(used_move t5)"] = 188;
func_map["(call_person_not_taking_medicine_constraint caregiver_call nathan)"] = 189;
func_map["(call_person_not_taking_medicine_constraint caregiver_call_guide nathan)"] = 190;
func_map["(DetectPerson_enabled)"] = 191;
func_map["(reminder_person_location_constraint automated_reminder nathan couch)"] = 192;
func_map["(reminder_person_location_constraint automated_reminder nathan kitchen)"] = 193;
func_map["(reminder_person_location_constraint guide_reminder_2 nathan couch)"] = 194;
func_map["(reminder_person_location_constraint guide_reminder_2 nathan kitchen)"] = 195;
func_map["(reminder_person_location_constraint guide_reminder_1 nathan couch)"] = 196;
func_map["(reminder_person_location_constraint guide_reminder_1 nathan kitchen)"] = 197;
func_map["(reminder_person_location_constraint recorded_reminder nathan couch)"] = 198;
func_map["(reminder_person_location_constraint recorded_reminder nathan kitchen)"] = 199;
func_map["(reminder_person_not_taking_medicine_constraint automated_reminder nathan)"] = 200;
func_map["(reminder_person_not_taking_medicine_constraint guide_reminder_2 nathan)"] = 201;
func_map["(reminder_person_not_taking_medicine_constraint guide_reminder_1 nathan)"] = 202;
func_map["(reminder_person_not_taking_medicine_constraint recorded_reminder nathan)"] = 203;
func_map["(time_limit t4)"] = 204;
func_map["(time_limit t1)"] = 205;
func_map["(time_limit t2)"] = 206;
func_map["(time_limit t3)"] = 207;
func_map["(time_limit t5)"] = 208;
        for (const auto& pred : problem.init){
            std::stringstream ss;
            ss << pred;
            state.data[func_map.at(ss.str())] = 1;
        }
        for (const auto& pred : problem.unknowns){
            std::stringstream ss;
            ss << pred;
            state.data[func_map.at(ss.str())] = 2;
        }
        for (const auto& constraint : problem.constraints){
            constraints.push_back(create_constraint(constraint, func_map));
        }

        check_goal = create_goal(problem.goal, func_map);
        return {state, new_states, constraints, check_goal};
    }

namespace indexers {

    unsigned char & next_timet4t4(KBState & state){
        return state.data[0];
    }
    int next_timet4t4_index(){
        return 0;
    }
    unsigned char & next_timet4t1(KBState & state){
        return state.data[1];
    }
    int next_timet4t1_index(){
        return 1;
    }
    unsigned char & next_timet4t2(KBState & state){
        return state.data[2];
    }
    int next_timet4t2_index(){
        return 2;
    }
    unsigned char & next_timet4t3(KBState & state){
        return state.data[3];
    }
    int next_timet4t3_index(){
        return 3;
    }
    unsigned char & next_timet4t5(KBState & state){
        return state.data[4];
    }
    int next_timet4t5_index(){
        return 4;
    }
    unsigned char & next_timet1t4(KBState & state){
        return state.data[5];
    }
    int next_timet1t4_index(){
        return 5;
    }
    unsigned char & next_timet1t1(KBState & state){
        return state.data[6];
    }
    int next_timet1t1_index(){
        return 6;
    }
    unsigned char & next_timet1t2(KBState & state){
        return state.data[7];
    }
    int next_timet1t2_index(){
        return 7;
    }
    unsigned char & next_timet1t3(KBState & state){
        return state.data[8];
    }
    int next_timet1t3_index(){
        return 8;
    }
    unsigned char & next_timet1t5(KBState & state){
        return state.data[9];
    }
    int next_timet1t5_index(){
        return 9;
    }
    unsigned char & next_timet2t4(KBState & state){
        return state.data[10];
    }
    int next_timet2t4_index(){
        return 10;
    }
    unsigned char & next_timet2t1(KBState & state){
        return state.data[11];
    }
    int next_timet2t1_index(){
        return 11;
    }
    unsigned char & next_timet2t2(KBState & state){
        return state.data[12];
    }
    int next_timet2t2_index(){
        return 12;
    }
    unsigned char & next_timet2t3(KBState & state){
        return state.data[13];
    }
    int next_timet2t3_index(){
        return 13;
    }
    unsigned char & next_timet2t5(KBState & state){
        return state.data[14];
    }
    int next_timet2t5_index(){
        return 14;
    }
    unsigned char & next_timet3t4(KBState & state){
        return state.data[15];
    }
    int next_timet3t4_index(){
        return 15;
    }
    unsigned char & next_timet3t1(KBState & state){
        return state.data[16];
    }
    int next_timet3t1_index(){
        return 16;
    }
    unsigned char & next_timet3t2(KBState & state){
        return state.data[17];
    }
    int next_timet3t2_index(){
        return 17;
    }
    unsigned char & next_timet3t3(KBState & state){
        return state.data[18];
    }
    int next_timet3t3_index(){
        return 18;
    }
    unsigned char & next_timet3t5(KBState & state){
        return state.data[19];
    }
    int next_timet3t5_index(){
        return 19;
    }
    unsigned char & next_timet5t4(KBState & state){
        return state.data[20];
    }
    int next_timet5t4_index(){
        return 20;
    }
    unsigned char & next_timet5t1(KBState & state){
        return state.data[21];
    }
    int next_timet5t1_index(){
        return 21;
    }
    unsigned char & next_timet5t2(KBState & state){
        return state.data[22];
    }
    int next_timet5t2_index(){
        return 22;
    }
    unsigned char & next_timet5t3(KBState & state){
        return state.data[23];
    }
    int next_timet5t3_index(){
        return 23;
    }
    unsigned char & next_timet5t5(KBState & state){
        return state.data[24];
    }
    int next_timet5t5_index(){
        return 24;
    }
    unsigned char & DetectEatingFood_enabled(KBState & state){
        return state.data[25];
    }
    int DetectEatingFood_enabled_index(){
        return 25;
    }
    unsigned char & call_person_not_eating_food_constraintcaregiver_callnathan(KBState & state){
        return state.data[26];
    }
    int call_person_not_eating_food_constraintcaregiver_callnathan_index(){
        return 26;
    }
    unsigned char & call_person_not_eating_food_constraintcaregiver_call_guidenathan(KBState & state){
        return state.data[27];
    }
    int call_person_not_eating_food_constraintcaregiver_call_guidenathan_index(){
        return 27;
    }
    unsigned char & call_person_location_constraintcaregiver_callnathancouch(KBState & state){
        return state.data[28];
    }
    int call_person_location_constraintcaregiver_callnathancouch_index(){
        return 28;
    }
    unsigned char & call_person_location_constraintcaregiver_callnathankitchen(KBState & state){
        return state.data[29];
    }
    int call_person_location_constraintcaregiver_callnathankitchen_index(){
        return 29;
    }
    unsigned char & call_person_location_constraintcaregiver_call_guidenathancouch(KBState & state){
        return state.data[30];
    }
    int call_person_location_constraintcaregiver_call_guidenathancouch_index(){
        return 30;
    }
    unsigned char & call_person_location_constraintcaregiver_call_guidenathankitchen(KBState & state){
        return state.data[31];
    }
    int call_person_location_constraintcaregiver_call_guidenathankitchen_index(){
        return 31;
    }
    unsigned char & current_timet4(KBState & state){
        return state.data[32];
    }
    int current_timet4_index(){
        return 32;
    }
    unsigned char & current_timet1(KBState & state){
        return state.data[33];
    }
    int current_timet1_index(){
        return 33;
    }
    unsigned char & current_timet2(KBState & state){
        return state.data[34];
    }
    int current_timet2_index(){
        return 34;
    }
    unsigned char & current_timet3(KBState & state){
        return state.data[35];
    }
    int current_timet3_index(){
        return 35;
    }
    unsigned char & current_timet5(KBState & state){
        return state.data[36];
    }
    int current_timet5_index(){
        return 36;
    }
    unsigned char & executed_reminderautomated_reminder(KBState & state){
        return state.data[37];
    }
    int executed_reminderautomated_reminder_index(){
        return 37;
    }
    unsigned char & executed_reminderguide_reminder_2(KBState & state){
        return state.data[38];
    }
    int executed_reminderguide_reminder_2_index(){
        return 38;
    }
    unsigned char & executed_reminderguide_reminder_1(KBState & state){
        return state.data[39];
    }
    int executed_reminderguide_reminder_1_index(){
        return 39;
    }
    unsigned char & executed_reminderrecorded_reminder(KBState & state){
        return state.data[40];
    }
    int executed_reminderrecorded_reminder_index(){
        return 40;
    }
    unsigned char & time_critical(KBState & state){
        return state.data[41];
    }
    int time_critical_index(){
        return 41;
    }
    unsigned char & call_blocks_callcaregiver_callcaregiver_call(KBState & state){
        return state.data[42];
    }
    int call_blocks_callcaregiver_callcaregiver_call_index(){
        return 42;
    }
    unsigned char & call_blocks_callcaregiver_callcaregiver_call_guide(KBState & state){
        return state.data[43];
    }
    int call_blocks_callcaregiver_callcaregiver_call_guide_index(){
        return 43;
    }
    unsigned char & call_blocks_callcaregiver_call_guidecaregiver_call(KBState & state){
        return state.data[44];
    }
    int call_blocks_callcaregiver_call_guidecaregiver_call_index(){
        return 44;
    }
    unsigned char & call_blocks_callcaregiver_call_guidecaregiver_call_guide(KBState & state){
        return state.data[45];
    }
    int call_blocks_callcaregiver_call_guidecaregiver_call_guide_index(){
        return 45;
    }
    unsigned char & valid_call_messagecaregiver_callguide_2_msg(KBState & state){
        return state.data[46];
    }
    int valid_call_messagecaregiver_callguide_2_msg_index(){
        return 46;
    }
    unsigned char & valid_call_messagecaregiver_callguide_1_msg(KBState & state){
        return state.data[47];
    }
    int valid_call_messagecaregiver_callguide_1_msg_index(){
        return 47;
    }
    unsigned char & valid_call_messagecaregiver_callautomated_msg(KBState & state){
        return state.data[48];
    }
    int valid_call_messagecaregiver_callautomated_msg_index(){
        return 48;
    }
    unsigned char & valid_call_messagecaregiver_callrecorded_msg(KBState & state){
        return state.data[49];
    }
    int valid_call_messagecaregiver_callrecorded_msg_index(){
        return 49;
    }
    unsigned char & valid_call_messagecaregiver_callcall_caregiver_msg(KBState & state){
        return state.data[50];
    }
    int valid_call_messagecaregiver_callcall_caregiver_msg_index(){
        return 50;
    }
    unsigned char & valid_call_messagecaregiver_callcall_caregiver_guide_msg(KBState & state){
        return state.data[51];
    }
    int valid_call_messagecaregiver_callcall_caregiver_guide_msg_index(){
        return 51;
    }
    unsigned char & valid_call_messagecaregiver_call_guideguide_2_msg(KBState & state){
        return state.data[52];
    }
    int valid_call_messagecaregiver_call_guideguide_2_msg_index(){
        return 52;
    }
    unsigned char & valid_call_messagecaregiver_call_guideguide_1_msg(KBState & state){
        return state.data[53];
    }
    int valid_call_messagecaregiver_call_guideguide_1_msg_index(){
        return 53;
    }
    unsigned char & valid_call_messagecaregiver_call_guideautomated_msg(KBState & state){
        return state.data[54];
    }
    int valid_call_messagecaregiver_call_guideautomated_msg_index(){
        return 54;
    }
    unsigned char & valid_call_messagecaregiver_call_guiderecorded_msg(KBState & state){
        return state.data[55];
    }
    int valid_call_messagecaregiver_call_guiderecorded_msg_index(){
        return 55;
    }
    unsigned char & valid_call_messagecaregiver_call_guidecall_caregiver_msg(KBState & state){
        return state.data[56];
    }
    int valid_call_messagecaregiver_call_guidecall_caregiver_msg_index(){
        return 56;
    }
    unsigned char & valid_call_messagecaregiver_call_guidecall_caregiver_guide_msg(KBState & state){
        return state.data[57];
    }
    int valid_call_messagecaregiver_call_guidecall_caregiver_guide_msg_index(){
        return 57;
    }
    unsigned char & DetectTakingMedicine_enabled(KBState & state){
        return state.data[58];
    }
    int DetectTakingMedicine_enabled_index(){
        return 58;
    }
    unsigned char & executed_callcaregiver_call(KBState & state){
        return state.data[59];
    }
    int executed_callcaregiver_call_index(){
        return 59;
    }
    unsigned char & executed_callcaregiver_call_guide(KBState & state){
        return state.data[60];
    }
    int executed_callcaregiver_call_guide_index(){
        return 60;
    }
    unsigned char & person_att4nathancouch(KBState & state){
        return state.data[61];
    }
    int person_att4nathancouch_index(){
        return 61;
    }
    unsigned char & person_att4nathankitchen(KBState & state){
        return state.data[62];
    }
    int person_att4nathankitchen_index(){
        return 62;
    }
    unsigned char & person_att1nathancouch(KBState & state){
        return state.data[63];
    }
    int person_att1nathancouch_index(){
        return 63;
    }
    unsigned char & person_att1nathankitchen(KBState & state){
        return state.data[64];
    }
    int person_att1nathankitchen_index(){
        return 64;
    }
    unsigned char & person_att2nathancouch(KBState & state){
        return state.data[65];
    }
    int person_att2nathancouch_index(){
        return 65;
    }
    unsigned char & person_att2nathankitchen(KBState & state){
        return state.data[66];
    }
    int person_att2nathankitchen_index(){
        return 66;
    }
    unsigned char & person_att3nathancouch(KBState & state){
        return state.data[67];
    }
    int person_att3nathancouch_index(){
        return 67;
    }
    unsigned char & person_att3nathankitchen(KBState & state){
        return state.data[68];
    }
    int person_att3nathankitchen_index(){
        return 68;
    }
    unsigned char & person_att5nathancouch(KBState & state){
        return state.data[69];
    }
    int person_att5nathancouch_index(){
        return 69;
    }
    unsigned char & person_att5nathankitchen(KBState & state){
        return state.data[70];
    }
    int person_att5nathankitchen_index(){
        return 70;
    }
    unsigned char & valid_reminder_messageautomated_reminderguide_2_msg(KBState & state){
        return state.data[71];
    }
    int valid_reminder_messageautomated_reminderguide_2_msg_index(){
        return 71;
    }
    unsigned char & valid_reminder_messageautomated_reminderguide_1_msg(KBState & state){
        return state.data[72];
    }
    int valid_reminder_messageautomated_reminderguide_1_msg_index(){
        return 72;
    }
    unsigned char & valid_reminder_messageautomated_reminderautomated_msg(KBState & state){
        return state.data[73];
    }
    int valid_reminder_messageautomated_reminderautomated_msg_index(){
        return 73;
    }
    unsigned char & valid_reminder_messageautomated_reminderrecorded_msg(KBState & state){
        return state.data[74];
    }
    int valid_reminder_messageautomated_reminderrecorded_msg_index(){
        return 74;
    }
    unsigned char & valid_reminder_messageautomated_remindercall_caregiver_msg(KBState & state){
        return state.data[75];
    }
    int valid_reminder_messageautomated_remindercall_caregiver_msg_index(){
        return 75;
    }
    unsigned char & valid_reminder_messageautomated_remindercall_caregiver_guide_msg(KBState & state){
        return state.data[76];
    }
    int valid_reminder_messageautomated_remindercall_caregiver_guide_msg_index(){
        return 76;
    }
    unsigned char & valid_reminder_messageguide_reminder_2guide_2_msg(KBState & state){
        return state.data[77];
    }
    int valid_reminder_messageguide_reminder_2guide_2_msg_index(){
        return 77;
    }
    unsigned char & valid_reminder_messageguide_reminder_2guide_1_msg(KBState & state){
        return state.data[78];
    }
    int valid_reminder_messageguide_reminder_2guide_1_msg_index(){
        return 78;
    }
    unsigned char & valid_reminder_messageguide_reminder_2automated_msg(KBState & state){
        return state.data[79];
    }
    int valid_reminder_messageguide_reminder_2automated_msg_index(){
        return 79;
    }
    unsigned char & valid_reminder_messageguide_reminder_2recorded_msg(KBState & state){
        return state.data[80];
    }
    int valid_reminder_messageguide_reminder_2recorded_msg_index(){
        return 80;
    }
    unsigned char & valid_reminder_messageguide_reminder_2call_caregiver_msg(KBState & state){
        return state.data[81];
    }
    int valid_reminder_messageguide_reminder_2call_caregiver_msg_index(){
        return 81;
    }
    unsigned char & valid_reminder_messageguide_reminder_2call_caregiver_guide_msg(KBState & state){
        return state.data[82];
    }
    int valid_reminder_messageguide_reminder_2call_caregiver_guide_msg_index(){
        return 82;
    }
    unsigned char & valid_reminder_messageguide_reminder_1guide_2_msg(KBState & state){
        return state.data[83];
    }
    int valid_reminder_messageguide_reminder_1guide_2_msg_index(){
        return 83;
    }
    unsigned char & valid_reminder_messageguide_reminder_1guide_1_msg(KBState & state){
        return state.data[84];
    }
    int valid_reminder_messageguide_reminder_1guide_1_msg_index(){
        return 84;
    }
    unsigned char & valid_reminder_messageguide_reminder_1automated_msg(KBState & state){
        return state.data[85];
    }
    int valid_reminder_messageguide_reminder_1automated_msg_index(){
        return 85;
    }
    unsigned char & valid_reminder_messageguide_reminder_1recorded_msg(KBState & state){
        return state.data[86];
    }
    int valid_reminder_messageguide_reminder_1recorded_msg_index(){
        return 86;
    }
    unsigned char & valid_reminder_messageguide_reminder_1call_caregiver_msg(KBState & state){
        return state.data[87];
    }
    int valid_reminder_messageguide_reminder_1call_caregiver_msg_index(){
        return 87;
    }
    unsigned char & valid_reminder_messageguide_reminder_1call_caregiver_guide_msg(KBState & state){
        return state.data[88];
    }
    int valid_reminder_messageguide_reminder_1call_caregiver_guide_msg_index(){
        return 88;
    }
    unsigned char & valid_reminder_messagerecorded_reminderguide_2_msg(KBState & state){
        return state.data[89];
    }
    int valid_reminder_messagerecorded_reminderguide_2_msg_index(){
        return 89;
    }
    unsigned char & valid_reminder_messagerecorded_reminderguide_1_msg(KBState & state){
        return state.data[90];
    }
    int valid_reminder_messagerecorded_reminderguide_1_msg_index(){
        return 90;
    }
    unsigned char & valid_reminder_messagerecorded_reminderautomated_msg(KBState & state){
        return state.data[91];
    }
    int valid_reminder_messagerecorded_reminderautomated_msg_index(){
        return 91;
    }
    unsigned char & valid_reminder_messagerecorded_reminderrecorded_msg(KBState & state){
        return state.data[92];
    }
    int valid_reminder_messagerecorded_reminderrecorded_msg_index(){
        return 92;
    }
    unsigned char & valid_reminder_messagerecorded_remindercall_caregiver_msg(KBState & state){
        return state.data[93];
    }
    int valid_reminder_messagerecorded_remindercall_caregiver_msg_index(){
        return 93;
    }
    unsigned char & valid_reminder_messagerecorded_remindercall_caregiver_guide_msg(KBState & state){
        return state.data[94];
    }
    int valid_reminder_messagerecorded_remindercall_caregiver_guide_msg_index(){
        return 94;
    }
    unsigned char & message_givenguide_2_msg(KBState & state){
        return state.data[95];
    }
    int message_givenguide_2_msg_index(){
        return 95;
    }
    unsigned char & message_givenguide_1_msg(KBState & state){
        return state.data[96];
    }
    int message_givenguide_1_msg_index(){
        return 96;
    }
    unsigned char & message_givenautomated_msg(KBState & state){
        return state.data[97];
    }
    int message_givenautomated_msg_index(){
        return 97;
    }
    unsigned char & message_givenrecorded_msg(KBState & state){
        return state.data[98];
    }
    int message_givenrecorded_msg_index(){
        return 98;
    }
    unsigned char & message_givencall_caregiver_msg(KBState & state){
        return state.data[99];
    }
    int message_givencall_caregiver_msg_index(){
        return 99;
    }
    unsigned char & message_givencall_caregiver_guide_msg(KBState & state){
        return state.data[100];
    }
    int message_givencall_caregiver_guide_msg_index(){
        return 100;
    }
    unsigned char & bed_locationcouch(KBState & state){
        return state.data[101];
    }
    int bed_locationcouch_index(){
        return 101;
    }
    unsigned char & bed_locationkitchen(KBState & state){
        return state.data[102];
    }
    int bed_locationkitchen_index(){
        return 102;
    }
    unsigned char & medicine_taken_success(KBState & state){
        return state.data[103];
    }
    int medicine_taken_success_index(){
        return 103;
    }
    unsigned char & reminder_person_not_eating_food_constraintautomated_remindernathan(KBState & state){
        return state.data[104];
    }
    int reminder_person_not_eating_food_constraintautomated_remindernathan_index(){
        return 104;
    }
    unsigned char & reminder_person_not_eating_food_constraintguide_reminder_2nathan(KBState & state){
        return state.data[105];
    }
    int reminder_person_not_eating_food_constraintguide_reminder_2nathan_index(){
        return 105;
    }
    unsigned char & reminder_person_not_eating_food_constraintguide_reminder_1nathan(KBState & state){
        return state.data[106];
    }
    int reminder_person_not_eating_food_constraintguide_reminder_1nathan_index(){
        return 106;
    }
    unsigned char & reminder_person_not_eating_food_constraintrecorded_remindernathan(KBState & state){
        return state.data[107];
    }
    int reminder_person_not_eating_food_constraintrecorded_remindernathan_index(){
        return 107;
    }
    unsigned char & person_on_groundt4(KBState & state){
        return state.data[108];
    }
    int person_on_groundt4_index(){
        return 108;
    }
    unsigned char & person_on_groundt1(KBState & state){
        return state.data[109];
    }
    int person_on_groundt1_index(){
        return 109;
    }
    unsigned char & person_on_groundt2(KBState & state){
        return state.data[110];
    }
    int person_on_groundt2_index(){
        return 110;
    }
    unsigned char & person_on_groundt3(KBState & state){
        return state.data[111];
    }
    int person_on_groundt3_index(){
        return 111;
    }
    unsigned char & person_on_groundt5(KBState & state){
        return state.data[112];
    }
    int person_on_groundt5_index(){
        return 112;
    }
    unsigned char & robot_atcouch(KBState & state){
        return state.data[113];
    }
    int robot_atcouch_index(){
        return 113;
    }
    unsigned char & robot_atkitchen(KBState & state){
        return state.data[114];
    }
    int robot_atkitchen_index(){
        return 114;
    }
    unsigned char & door_locationcouch(KBState & state){
        return state.data[115];
    }
    int door_locationcouch_index(){
        return 115;
    }
    unsigned char & door_locationkitchen(KBState & state){
        return state.data[116];
    }
    int door_locationkitchen_index(){
        return 116;
    }
    unsigned char & person_eating_foodt4(KBState & state){
        return state.data[117];
    }
    int person_eating_foodt4_index(){
        return 117;
    }
    unsigned char & person_eating_foodt1(KBState & state){
        return state.data[118];
    }
    int person_eating_foodt1_index(){
        return 118;
    }
    unsigned char & person_eating_foodt2(KBState & state){
        return state.data[119];
    }
    int person_eating_foodt2_index(){
        return 119;
    }
    unsigned char & person_eating_foodt3(KBState & state){
        return state.data[120];
    }
    int person_eating_foodt3_index(){
        return 120;
    }
    unsigned char & person_eating_foodt5(KBState & state){
        return state.data[121];
    }
    int person_eating_foodt5_index(){
        return 121;
    }
    unsigned char & person_at_successnathancouch(KBState & state){
        return state.data[122];
    }
    int person_at_successnathancouch_index(){
        return 122;
    }
    unsigned char & person_at_successnathankitchen(KBState & state){
        return state.data[123];
    }
    int person_at_successnathankitchen_index(){
        return 123;
    }
    unsigned char & person_taking_medicinet4(KBState & state){
        return state.data[124];
    }
    int person_taking_medicinet4_index(){
        return 124;
    }
    unsigned char & person_taking_medicinet1(KBState & state){
        return state.data[125];
    }
    int person_taking_medicinet1_index(){
        return 125;
    }
    unsigned char & person_taking_medicinet2(KBState & state){
        return state.data[126];
    }
    int person_taking_medicinet2_index(){
        return 126;
    }
    unsigned char & person_taking_medicinet3(KBState & state){
        return state.data[127];
    }
    int person_taking_medicinet3_index(){
        return 127;
    }
    unsigned char & person_taking_medicinet5(KBState & state){
        return state.data[128];
    }
    int person_taking_medicinet5_index(){
        return 128;
    }
    unsigned char & person_eatingt4(KBState & state){
        return state.data[129];
    }
    int person_eatingt4_index(){
        return 129;
    }
    unsigned char & person_eatingt1(KBState & state){
        return state.data[130];
    }
    int person_eatingt1_index(){
        return 130;
    }
    unsigned char & person_eatingt2(KBState & state){
        return state.data[131];
    }
    int person_eatingt2_index(){
        return 131;
    }
    unsigned char & person_eatingt3(KBState & state){
        return state.data[132];
    }
    int person_eatingt3_index(){
        return 132;
    }
    unsigned char & person_eatingt5(KBState & state){
        return state.data[133];
    }
    int person_eatingt5_index(){
        return 133;
    }
    unsigned char & GiveReminder_enabled(KBState & state){
        return state.data[134];
    }
    int GiveReminder_enabled_index(){
        return 134;
    }
    unsigned char & reminder_blocks_callautomated_remindercaregiver_call(KBState & state){
        return state.data[135];
    }
    int reminder_blocks_callautomated_remindercaregiver_call_index(){
        return 135;
    }
    unsigned char & reminder_blocks_callautomated_remindercaregiver_call_guide(KBState & state){
        return state.data[136];
    }
    int reminder_blocks_callautomated_remindercaregiver_call_guide_index(){
        return 136;
    }
    unsigned char & reminder_blocks_callguide_reminder_2caregiver_call(KBState & state){
        return state.data[137];
    }
    int reminder_blocks_callguide_reminder_2caregiver_call_index(){
        return 137;
    }
    unsigned char & reminder_blocks_callguide_reminder_2caregiver_call_guide(KBState & state){
        return state.data[138];
    }
    int reminder_blocks_callguide_reminder_2caregiver_call_guide_index(){
        return 138;
    }
    unsigned char & reminder_blocks_callguide_reminder_1caregiver_call(KBState & state){
        return state.data[139];
    }
    int reminder_blocks_callguide_reminder_1caregiver_call_index(){
        return 139;
    }
    unsigned char & reminder_blocks_callguide_reminder_1caregiver_call_guide(KBState & state){
        return state.data[140];
    }
    int reminder_blocks_callguide_reminder_1caregiver_call_guide_index(){
        return 140;
    }
    unsigned char & reminder_blocks_callrecorded_remindercaregiver_call(KBState & state){
        return state.data[141];
    }
    int reminder_blocks_callrecorded_remindercaregiver_call_index(){
        return 141;
    }
    unsigned char & reminder_blocks_callrecorded_remindercaregiver_call_guide(KBState & state){
        return state.data[142];
    }
    int reminder_blocks_callrecorded_remindercaregiver_call_guide_index(){
        return 142;
    }
    unsigned char & food_eaten_success(KBState & state){
        return state.data[143];
    }
    int food_eaten_success_index(){
        return 143;
    }
    unsigned char & MakeCall_enabled(KBState & state){
        return state.data[144];
    }
    int MakeCall_enabled_index(){
        return 144;
    }
    unsigned char & message_given_successguide_2_msg(KBState & state){
        return state.data[145];
    }
    int message_given_successguide_2_msg_index(){
        return 145;
    }
    unsigned char & message_given_successguide_1_msg(KBState & state){
        return state.data[146];
    }
    int message_given_successguide_1_msg_index(){
        return 146;
    }
    unsigned char & message_given_successautomated_msg(KBState & state){
        return state.data[147];
    }
    int message_given_successautomated_msg_index(){
        return 147;
    }
    unsigned char & message_given_successrecorded_msg(KBState & state){
        return state.data[148];
    }
    int message_given_successrecorded_msg_index(){
        return 148;
    }
    unsigned char & message_given_successcall_caregiver_msg(KBState & state){
        return state.data[149];
    }
    int message_given_successcall_caregiver_msg_index(){
        return 149;
    }
    unsigned char & message_given_successcall_caregiver_guide_msg(KBState & state){
        return state.data[150];
    }
    int message_given_successcall_caregiver_guide_msg_index(){
        return 150;
    }
    unsigned char & reminder_blocks_reminderautomated_reminderautomated_reminder(KBState & state){
        return state.data[151];
    }
    int reminder_blocks_reminderautomated_reminderautomated_reminder_index(){
        return 151;
    }
    unsigned char & reminder_blocks_reminderautomated_reminderguide_reminder_2(KBState & state){
        return state.data[152];
    }
    int reminder_blocks_reminderautomated_reminderguide_reminder_2_index(){
        return 152;
    }
    unsigned char & reminder_blocks_reminderautomated_reminderguide_reminder_1(KBState & state){
        return state.data[153];
    }
    int reminder_blocks_reminderautomated_reminderguide_reminder_1_index(){
        return 153;
    }
    unsigned char & reminder_blocks_reminderautomated_reminderrecorded_reminder(KBState & state){
        return state.data[154];
    }
    int reminder_blocks_reminderautomated_reminderrecorded_reminder_index(){
        return 154;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_2automated_reminder(KBState & state){
        return state.data[155];
    }
    int reminder_blocks_reminderguide_reminder_2automated_reminder_index(){
        return 155;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_2guide_reminder_2(KBState & state){
        return state.data[156];
    }
    int reminder_blocks_reminderguide_reminder_2guide_reminder_2_index(){
        return 156;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_2guide_reminder_1(KBState & state){
        return state.data[157];
    }
    int reminder_blocks_reminderguide_reminder_2guide_reminder_1_index(){
        return 157;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_2recorded_reminder(KBState & state){
        return state.data[158];
    }
    int reminder_blocks_reminderguide_reminder_2recorded_reminder_index(){
        return 158;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_1automated_reminder(KBState & state){
        return state.data[159];
    }
    int reminder_blocks_reminderguide_reminder_1automated_reminder_index(){
        return 159;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_1guide_reminder_2(KBState & state){
        return state.data[160];
    }
    int reminder_blocks_reminderguide_reminder_1guide_reminder_2_index(){
        return 160;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_1guide_reminder_1(KBState & state){
        return state.data[161];
    }
    int reminder_blocks_reminderguide_reminder_1guide_reminder_1_index(){
        return 161;
    }
    unsigned char & reminder_blocks_reminderguide_reminder_1recorded_reminder(KBState & state){
        return state.data[162];
    }
    int reminder_blocks_reminderguide_reminder_1recorded_reminder_index(){
        return 162;
    }
    unsigned char & reminder_blocks_reminderrecorded_reminderautomated_reminder(KBState & state){
        return state.data[163];
    }
    int reminder_blocks_reminderrecorded_reminderautomated_reminder_index(){
        return 163;
    }
    unsigned char & reminder_blocks_reminderrecorded_reminderguide_reminder_2(KBState & state){
        return state.data[164];
    }
    int reminder_blocks_reminderrecorded_reminderguide_reminder_2_index(){
        return 164;
    }
    unsigned char & reminder_blocks_reminderrecorded_reminderguide_reminder_1(KBState & state){
        return state.data[165];
    }
    int reminder_blocks_reminderrecorded_reminderguide_reminder_1_index(){
        return 165;
    }
    unsigned char & reminder_blocks_reminderrecorded_reminderrecorded_reminder(KBState & state){
        return state.data[166];
    }
    int reminder_blocks_reminderrecorded_reminderrecorded_reminder_index(){
        return 166;
    }
    unsigned char & call_not_person_location_constraintcaregiver_callnathancouch(KBState & state){
        return state.data[167];
    }
    int call_not_person_location_constraintcaregiver_callnathancouch_index(){
        return 167;
    }
    unsigned char & call_not_person_location_constraintcaregiver_callnathankitchen(KBState & state){
        return state.data[168];
    }
    int call_not_person_location_constraintcaregiver_callnathankitchen_index(){
        return 168;
    }
    unsigned char & call_not_person_location_constraintcaregiver_call_guidenathancouch(KBState & state){
        return state.data[169];
    }
    int call_not_person_location_constraintcaregiver_call_guidenathancouch_index(){
        return 169;
    }
    unsigned char & call_not_person_location_constraintcaregiver_call_guidenathankitchen(KBState & state){
        return state.data[170];
    }
    int call_not_person_location_constraintcaregiver_call_guidenathankitchen_index(){
        return 170;
    }
    unsigned char & success(KBState & state){
        return state.data[171];
    }
    int success_index(){
        return 171;
    }
    unsigned char & traversablecouchcouch(KBState & state){
        return state.data[172];
    }
    int traversablecouchcouch_index(){
        return 172;
    }
    unsigned char & traversablecouchkitchen(KBState & state){
        return state.data[173];
    }
    int traversablecouchkitchen_index(){
        return 173;
    }
    unsigned char & traversablekitchencouch(KBState & state){
        return state.data[174];
    }
    int traversablekitchencouch_index(){
        return 174;
    }
    unsigned char & traversablekitchenkitchen(KBState & state){
        return state.data[175];
    }
    int traversablekitchenkitchen_index(){
        return 175;
    }
    unsigned char & reminder_not_person_location_constraintautomated_remindernathancouch(KBState & state){
        return state.data[176];
    }
    int reminder_not_person_location_constraintautomated_remindernathancouch_index(){
        return 176;
    }
    unsigned char & reminder_not_person_location_constraintautomated_remindernathankitchen(KBState & state){
        return state.data[177];
    }
    int reminder_not_person_location_constraintautomated_remindernathankitchen_index(){
        return 177;
    }
    unsigned char & reminder_not_person_location_constraintguide_reminder_2nathancouch(KBState & state){
        return state.data[178];
    }
    int reminder_not_person_location_constraintguide_reminder_2nathancouch_index(){
        return 178;
    }
    unsigned char & reminder_not_person_location_constraintguide_reminder_2nathankitchen(KBState & state){
        return state.data[179];
    }
    int reminder_not_person_location_constraintguide_reminder_2nathankitchen_index(){
        return 179;
    }
    unsigned char & reminder_not_person_location_constraintguide_reminder_1nathancouch(KBState & state){
        return state.data[180];
    }
    int reminder_not_person_location_constraintguide_reminder_1nathancouch_index(){
        return 180;
    }
    unsigned char & reminder_not_person_location_constraintguide_reminder_1nathankitchen(KBState & state){
        return state.data[181];
    }
    int reminder_not_person_location_constraintguide_reminder_1nathankitchen_index(){
        return 181;
    }
    unsigned char & reminder_not_person_location_constraintrecorded_remindernathancouch(KBState & state){
        return state.data[182];
    }
    int reminder_not_person_location_constraintrecorded_remindernathancouch_index(){
        return 182;
    }
    unsigned char & reminder_not_person_location_constraintrecorded_remindernathankitchen(KBState & state){
        return state.data[183];
    }
    int reminder_not_person_location_constraintrecorded_remindernathankitchen_index(){
        return 183;
    }
    unsigned char & used_movet4(KBState & state){
        return state.data[184];
    }
    int used_movet4_index(){
        return 184;
    }
    unsigned char & used_movet1(KBState & state){
        return state.data[185];
    }
    int used_movet1_index(){
        return 185;
    }
    unsigned char & used_movet2(KBState & state){
        return state.data[186];
    }
    int used_movet2_index(){
        return 186;
    }
    unsigned char & used_movet3(KBState & state){
        return state.data[187];
    }
    int used_movet3_index(){
        return 187;
    }
    unsigned char & used_movet5(KBState & state){
        return state.data[188];
    }
    int used_movet5_index(){
        return 188;
    }
    unsigned char & call_person_not_taking_medicine_constraintcaregiver_callnathan(KBState & state){
        return state.data[189];
    }
    int call_person_not_taking_medicine_constraintcaregiver_callnathan_index(){
        return 189;
    }
    unsigned char & call_person_not_taking_medicine_constraintcaregiver_call_guidenathan(KBState & state){
        return state.data[190];
    }
    int call_person_not_taking_medicine_constraintcaregiver_call_guidenathan_index(){
        return 190;
    }
    unsigned char & DetectPerson_enabled(KBState & state){
        return state.data[191];
    }
    int DetectPerson_enabled_index(){
        return 191;
    }
    unsigned char & reminder_person_location_constraintautomated_remindernathancouch(KBState & state){
        return state.data[192];
    }
    int reminder_person_location_constraintautomated_remindernathancouch_index(){
        return 192;
    }
    unsigned char & reminder_person_location_constraintautomated_remindernathankitchen(KBState & state){
        return state.data[193];
    }
    int reminder_person_location_constraintautomated_remindernathankitchen_index(){
        return 193;
    }
    unsigned char & reminder_person_location_constraintguide_reminder_2nathancouch(KBState & state){
        return state.data[194];
    }
    int reminder_person_location_constraintguide_reminder_2nathancouch_index(){
        return 194;
    }
    unsigned char & reminder_person_location_constraintguide_reminder_2nathankitchen(KBState & state){
        return state.data[195];
    }
    int reminder_person_location_constraintguide_reminder_2nathankitchen_index(){
        return 195;
    }
    unsigned char & reminder_person_location_constraintguide_reminder_1nathancouch(KBState & state){
        return state.data[196];
    }
    int reminder_person_location_constraintguide_reminder_1nathancouch_index(){
        return 196;
    }
    unsigned char & reminder_person_location_constraintguide_reminder_1nathankitchen(KBState & state){
        return state.data[197];
    }
    int reminder_person_location_constraintguide_reminder_1nathankitchen_index(){
        return 197;
    }
    unsigned char & reminder_person_location_constraintrecorded_remindernathancouch(KBState & state){
        return state.data[198];
    }
    int reminder_person_location_constraintrecorded_remindernathancouch_index(){
        return 198;
    }
    unsigned char & reminder_person_location_constraintrecorded_remindernathankitchen(KBState & state){
        return state.data[199];
    }
    int reminder_person_location_constraintrecorded_remindernathankitchen_index(){
        return 199;
    }
    unsigned char & reminder_person_not_taking_medicine_constraintautomated_remindernathan(KBState & state){
        return state.data[200];
    }
    int reminder_person_not_taking_medicine_constraintautomated_remindernathan_index(){
        return 200;
    }
    unsigned char & reminder_person_not_taking_medicine_constraintguide_reminder_2nathan(KBState & state){
        return state.data[201];
    }
    int reminder_person_not_taking_medicine_constraintguide_reminder_2nathan_index(){
        return 201;
    }
    unsigned char & reminder_person_not_taking_medicine_constraintguide_reminder_1nathan(KBState & state){
        return state.data[202];
    }
    int reminder_person_not_taking_medicine_constraintguide_reminder_1nathan_index(){
        return 202;
    }
    unsigned char & reminder_person_not_taking_medicine_constraintrecorded_remindernathan(KBState & state){
        return state.data[203];
    }
    int reminder_person_not_taking_medicine_constraintrecorded_remindernathan_index(){
        return 203;
    }
    unsigned char & time_limitt4(KBState & state){
        return state.data[204];
    }
    int time_limitt4_index(){
        return 204;
    }
    unsigned char & time_limitt1(KBState & state){
        return state.data[205];
    }
    int time_limitt1_index(){
        return 205;
    }
    unsigned char & time_limitt2(KBState & state){
        return state.data[206];
    }
    int time_limitt2_index(){
        return 206;
    }
    unsigned char & time_limitt3(KBState & state){
        return state.data[207];
    }
    int time_limitt3_index(){
        return 207;
    }
    unsigned char & time_limitt5(KBState & state){
        return state.data[208];
    }
    int time_limitt5_index(){
        return 208;
    }

std::vector<std::string> get_action_string(unsigned int index){


if (index == 0){
    return {"GiveReminder", "automated_reminder", "t4", "nathan", "guide_2_msg"};
}
if (index == 1){
    return {"GiveReminder", "automated_reminder", "t4", "nathan", "guide_1_msg"};
}
if (index == 2){
    return {"GiveReminder", "automated_reminder", "t4", "nathan", "automated_msg"};
}
if (index == 3){
    return {"GiveReminder", "automated_reminder", "t4", "nathan", "recorded_msg"};
}
if (index == 4){
    return {"GiveReminder", "automated_reminder", "t4", "nathan", "call_caregiver_msg"};
}
if (index == 5){
    return {"GiveReminder", "automated_reminder", "t4", "nathan", "call_caregiver_guide_msg"};
}
if (index == 6){
    return {"GiveReminder", "automated_reminder", "t1", "nathan", "guide_2_msg"};
}
if (index == 7){
    return {"GiveReminder", "automated_reminder", "t1", "nathan", "guide_1_msg"};
}
if (index == 8){
    return {"GiveReminder", "automated_reminder", "t1", "nathan", "automated_msg"};
}
if (index == 9){
    return {"GiveReminder", "automated_reminder", "t1", "nathan", "recorded_msg"};
}
if (index == 10){
    return {"GiveReminder", "automated_reminder", "t1", "nathan", "call_caregiver_msg"};
}
if (index == 11){
    return {"GiveReminder", "automated_reminder", "t1", "nathan", "call_caregiver_guide_msg"};
}
if (index == 12){
    return {"GiveReminder", "automated_reminder", "t2", "nathan", "guide_2_msg"};
}
if (index == 13){
    return {"GiveReminder", "automated_reminder", "t2", "nathan", "guide_1_msg"};
}
if (index == 14){
    return {"GiveReminder", "automated_reminder", "t2", "nathan", "automated_msg"};
}
if (index == 15){
    return {"GiveReminder", "automated_reminder", "t2", "nathan", "recorded_msg"};
}
if (index == 16){
    return {"GiveReminder", "automated_reminder", "t2", "nathan", "call_caregiver_msg"};
}
if (index == 17){
    return {"GiveReminder", "automated_reminder", "t2", "nathan", "call_caregiver_guide_msg"};
}
if (index == 18){
    return {"GiveReminder", "automated_reminder", "t3", "nathan", "guide_2_msg"};
}
if (index == 19){
    return {"GiveReminder", "automated_reminder", "t3", "nathan", "guide_1_msg"};
}
if (index == 20){
    return {"GiveReminder", "automated_reminder", "t3", "nathan", "automated_msg"};
}
if (index == 21){
    return {"GiveReminder", "automated_reminder", "t3", "nathan", "recorded_msg"};
}
if (index == 22){
    return {"GiveReminder", "automated_reminder", "t3", "nathan", "call_caregiver_msg"};
}
if (index == 23){
    return {"GiveReminder", "automated_reminder", "t3", "nathan", "call_caregiver_guide_msg"};
}
if (index == 24){
    return {"GiveReminder", "automated_reminder", "t5", "nathan", "guide_2_msg"};
}
if (index == 25){
    return {"GiveReminder", "automated_reminder", "t5", "nathan", "guide_1_msg"};
}
if (index == 26){
    return {"GiveReminder", "automated_reminder", "t5", "nathan", "automated_msg"};
}
if (index == 27){
    return {"GiveReminder", "automated_reminder", "t5", "nathan", "recorded_msg"};
}
if (index == 28){
    return {"GiveReminder", "automated_reminder", "t5", "nathan", "call_caregiver_msg"};
}
if (index == 29){
    return {"GiveReminder", "automated_reminder", "t5", "nathan", "call_caregiver_guide_msg"};
}
if (index == 30){
    return {"GiveReminder", "guide_reminder_2", "t4", "nathan", "guide_2_msg"};
}
if (index == 31){
    return {"GiveReminder", "guide_reminder_2", "t4", "nathan", "guide_1_msg"};
}
if (index == 32){
    return {"GiveReminder", "guide_reminder_2", "t4", "nathan", "automated_msg"};
}
if (index == 33){
    return {"GiveReminder", "guide_reminder_2", "t4", "nathan", "recorded_msg"};
}
if (index == 34){
    return {"GiveReminder", "guide_reminder_2", "t4", "nathan", "call_caregiver_msg"};
}
if (index == 35){
    return {"GiveReminder", "guide_reminder_2", "t4", "nathan", "call_caregiver_guide_msg"};
}
if (index == 36){
    return {"GiveReminder", "guide_reminder_2", "t1", "nathan", "guide_2_msg"};
}
if (index == 37){
    return {"GiveReminder", "guide_reminder_2", "t1", "nathan", "guide_1_msg"};
}
if (index == 38){
    return {"GiveReminder", "guide_reminder_2", "t1", "nathan", "automated_msg"};
}
if (index == 39){
    return {"GiveReminder", "guide_reminder_2", "t1", "nathan", "recorded_msg"};
}
if (index == 40){
    return {"GiveReminder", "guide_reminder_2", "t1", "nathan", "call_caregiver_msg"};
}
if (index == 41){
    return {"GiveReminder", "guide_reminder_2", "t1", "nathan", "call_caregiver_guide_msg"};
}
if (index == 42){
    return {"GiveReminder", "guide_reminder_2", "t2", "nathan", "guide_2_msg"};
}
if (index == 43){
    return {"GiveReminder", "guide_reminder_2", "t2", "nathan", "guide_1_msg"};
}
if (index == 44){
    return {"GiveReminder", "guide_reminder_2", "t2", "nathan", "automated_msg"};
}
if (index == 45){
    return {"GiveReminder", "guide_reminder_2", "t2", "nathan", "recorded_msg"};
}
if (index == 46){
    return {"GiveReminder", "guide_reminder_2", "t2", "nathan", "call_caregiver_msg"};
}
if (index == 47){
    return {"GiveReminder", "guide_reminder_2", "t2", "nathan", "call_caregiver_guide_msg"};
}
if (index == 48){
    return {"GiveReminder", "guide_reminder_2", "t3", "nathan", "guide_2_msg"};
}
if (index == 49){
    return {"GiveReminder", "guide_reminder_2", "t3", "nathan", "guide_1_msg"};
}
if (index == 50){
    return {"GiveReminder", "guide_reminder_2", "t3", "nathan", "automated_msg"};
}
if (index == 51){
    return {"GiveReminder", "guide_reminder_2", "t3", "nathan", "recorded_msg"};
}
if (index == 52){
    return {"GiveReminder", "guide_reminder_2", "t3", "nathan", "call_caregiver_msg"};
}
if (index == 53){
    return {"GiveReminder", "guide_reminder_2", "t3", "nathan", "call_caregiver_guide_msg"};
}
if (index == 54){
    return {"GiveReminder", "guide_reminder_2", "t5", "nathan", "guide_2_msg"};
}
if (index == 55){
    return {"GiveReminder", "guide_reminder_2", "t5", "nathan", "guide_1_msg"};
}
if (index == 56){
    return {"GiveReminder", "guide_reminder_2", "t5", "nathan", "automated_msg"};
}
if (index == 57){
    return {"GiveReminder", "guide_reminder_2", "t5", "nathan", "recorded_msg"};
}
if (index == 58){
    return {"GiveReminder", "guide_reminder_2", "t5", "nathan", "call_caregiver_msg"};
}
if (index == 59){
    return {"GiveReminder", "guide_reminder_2", "t5", "nathan", "call_caregiver_guide_msg"};
}
if (index == 60){
    return {"GiveReminder", "guide_reminder_1", "t4", "nathan", "guide_2_msg"};
}
if (index == 61){
    return {"GiveReminder", "guide_reminder_1", "t4", "nathan", "guide_1_msg"};
}
if (index == 62){
    return {"GiveReminder", "guide_reminder_1", "t4", "nathan", "automated_msg"};
}
if (index == 63){
    return {"GiveReminder", "guide_reminder_1", "t4", "nathan", "recorded_msg"};
}
if (index == 64){
    return {"GiveReminder", "guide_reminder_1", "t4", "nathan", "call_caregiver_msg"};
}
if (index == 65){
    return {"GiveReminder", "guide_reminder_1", "t4", "nathan", "call_caregiver_guide_msg"};
}
if (index == 66){
    return {"GiveReminder", "guide_reminder_1", "t1", "nathan", "guide_2_msg"};
}
if (index == 67){
    return {"GiveReminder", "guide_reminder_1", "t1", "nathan", "guide_1_msg"};
}
if (index == 68){
    return {"GiveReminder", "guide_reminder_1", "t1", "nathan", "automated_msg"};
}
if (index == 69){
    return {"GiveReminder", "guide_reminder_1", "t1", "nathan", "recorded_msg"};
}
if (index == 70){
    return {"GiveReminder", "guide_reminder_1", "t1", "nathan", "call_caregiver_msg"};
}
if (index == 71){
    return {"GiveReminder", "guide_reminder_1", "t1", "nathan", "call_caregiver_guide_msg"};
}
if (index == 72){
    return {"GiveReminder", "guide_reminder_1", "t2", "nathan", "guide_2_msg"};
}
if (index == 73){
    return {"GiveReminder", "guide_reminder_1", "t2", "nathan", "guide_1_msg"};
}
if (index == 74){
    return {"GiveReminder", "guide_reminder_1", "t2", "nathan", "automated_msg"};
}
if (index == 75){
    return {"GiveReminder", "guide_reminder_1", "t2", "nathan", "recorded_msg"};
}
if (index == 76){
    return {"GiveReminder", "guide_reminder_1", "t2", "nathan", "call_caregiver_msg"};
}
if (index == 77){
    return {"GiveReminder", "guide_reminder_1", "t2", "nathan", "call_caregiver_guide_msg"};
}
if (index == 78){
    return {"GiveReminder", "guide_reminder_1", "t3", "nathan", "guide_2_msg"};
}
if (index == 79){
    return {"GiveReminder", "guide_reminder_1", "t3", "nathan", "guide_1_msg"};
}
if (index == 80){
    return {"GiveReminder", "guide_reminder_1", "t3", "nathan", "automated_msg"};
}
if (index == 81){
    return {"GiveReminder", "guide_reminder_1", "t3", "nathan", "recorded_msg"};
}
if (index == 82){
    return {"GiveReminder", "guide_reminder_1", "t3", "nathan", "call_caregiver_msg"};
}
if (index == 83){
    return {"GiveReminder", "guide_reminder_1", "t3", "nathan", "call_caregiver_guide_msg"};
}
if (index == 84){
    return {"GiveReminder", "guide_reminder_1", "t5", "nathan", "guide_2_msg"};
}
if (index == 85){
    return {"GiveReminder", "guide_reminder_1", "t5", "nathan", "guide_1_msg"};
}
if (index == 86){
    return {"GiveReminder", "guide_reminder_1", "t5", "nathan", "automated_msg"};
}
if (index == 87){
    return {"GiveReminder", "guide_reminder_1", "t5", "nathan", "recorded_msg"};
}
if (index == 88){
    return {"GiveReminder", "guide_reminder_1", "t5", "nathan", "call_caregiver_msg"};
}
if (index == 89){
    return {"GiveReminder", "guide_reminder_1", "t5", "nathan", "call_caregiver_guide_msg"};
}
if (index == 90){
    return {"GiveReminder", "recorded_reminder", "t4", "nathan", "guide_2_msg"};
}
if (index == 91){
    return {"GiveReminder", "recorded_reminder", "t4", "nathan", "guide_1_msg"};
}
if (index == 92){
    return {"GiveReminder", "recorded_reminder", "t4", "nathan", "automated_msg"};
}
if (index == 93){
    return {"GiveReminder", "recorded_reminder", "t4", "nathan", "recorded_msg"};
}
if (index == 94){
    return {"GiveReminder", "recorded_reminder", "t4", "nathan", "call_caregiver_msg"};
}
if (index == 95){
    return {"GiveReminder", "recorded_reminder", "t4", "nathan", "call_caregiver_guide_msg"};
}
if (index == 96){
    return {"GiveReminder", "recorded_reminder", "t1", "nathan", "guide_2_msg"};
}
if (index == 97){
    return {"GiveReminder", "recorded_reminder", "t1", "nathan", "guide_1_msg"};
}
if (index == 98){
    return {"GiveReminder", "recorded_reminder", "t1", "nathan", "automated_msg"};
}
if (index == 99){
    return {"GiveReminder", "recorded_reminder", "t1", "nathan", "recorded_msg"};
}
if (index == 100){
    return {"GiveReminder", "recorded_reminder", "t1", "nathan", "call_caregiver_msg"};
}
if (index == 101){
    return {"GiveReminder", "recorded_reminder", "t1", "nathan", "call_caregiver_guide_msg"};
}
if (index == 102){
    return {"GiveReminder", "recorded_reminder", "t2", "nathan", "guide_2_msg"};
}
if (index == 103){
    return {"GiveReminder", "recorded_reminder", "t2", "nathan", "guide_1_msg"};
}
if (index == 104){
    return {"GiveReminder", "recorded_reminder", "t2", "nathan", "automated_msg"};
}
if (index == 105){
    return {"GiveReminder", "recorded_reminder", "t2", "nathan", "recorded_msg"};
}
if (index == 106){
    return {"GiveReminder", "recorded_reminder", "t2", "nathan", "call_caregiver_msg"};
}
if (index == 107){
    return {"GiveReminder", "recorded_reminder", "t2", "nathan", "call_caregiver_guide_msg"};
}
if (index == 108){
    return {"GiveReminder", "recorded_reminder", "t3", "nathan", "guide_2_msg"};
}
if (index == 109){
    return {"GiveReminder", "recorded_reminder", "t3", "nathan", "guide_1_msg"};
}
if (index == 110){
    return {"GiveReminder", "recorded_reminder", "t3", "nathan", "automated_msg"};
}
if (index == 111){
    return {"GiveReminder", "recorded_reminder", "t3", "nathan", "recorded_msg"};
}
if (index == 112){
    return {"GiveReminder", "recorded_reminder", "t3", "nathan", "call_caregiver_msg"};
}
if (index == 113){
    return {"GiveReminder", "recorded_reminder", "t3", "nathan", "call_caregiver_guide_msg"};
}
if (index == 114){
    return {"GiveReminder", "recorded_reminder", "t5", "nathan", "guide_2_msg"};
}
if (index == 115){
    return {"GiveReminder", "recorded_reminder", "t5", "nathan", "guide_1_msg"};
}
if (index == 116){
    return {"GiveReminder", "recorded_reminder", "t5", "nathan", "automated_msg"};
}
if (index == 117){
    return {"GiveReminder", "recorded_reminder", "t5", "nathan", "recorded_msg"};
}
if (index == 118){
    return {"GiveReminder", "recorded_reminder", "t5", "nathan", "call_caregiver_msg"};
}
if (index == 119){
    return {"GiveReminder", "recorded_reminder", "t5", "nathan", "call_caregiver_guide_msg"};
}
if (index == 120){
    return {"MoveToLandmark", "t4", "couch", "couch"};
}
if (index == 121){
    return {"MoveToLandmark", "t4", "couch", "kitchen"};
}
if (index == 122){
    return {"MoveToLandmark", "t4", "kitchen", "couch"};
}
if (index == 123){
    return {"MoveToLandmark", "t4", "kitchen", "kitchen"};
}
if (index == 124){
    return {"MoveToLandmark", "t1", "couch", "couch"};
}
if (index == 125){
    return {"MoveToLandmark", "t1", "couch", "kitchen"};
}
if (index == 126){
    return {"MoveToLandmark", "t1", "kitchen", "couch"};
}
if (index == 127){
    return {"MoveToLandmark", "t1", "kitchen", "kitchen"};
}
if (index == 128){
    return {"MoveToLandmark", "t2", "couch", "couch"};
}
if (index == 129){
    return {"MoveToLandmark", "t2", "couch", "kitchen"};
}
if (index == 130){
    return {"MoveToLandmark", "t2", "kitchen", "couch"};
}
if (index == 131){
    return {"MoveToLandmark", "t2", "kitchen", "kitchen"};
}
if (index == 132){
    return {"MoveToLandmark", "t3", "couch", "couch"};
}
if (index == 133){
    return {"MoveToLandmark", "t3", "couch", "kitchen"};
}
if (index == 134){
    return {"MoveToLandmark", "t3", "kitchen", "couch"};
}
if (index == 135){
    return {"MoveToLandmark", "t3", "kitchen", "kitchen"};
}
if (index == 136){
    return {"MoveToLandmark", "t5", "couch", "couch"};
}
if (index == 137){
    return {"MoveToLandmark", "t5", "couch", "kitchen"};
}
if (index == 138){
    return {"MoveToLandmark", "t5", "kitchen", "couch"};
}
if (index == 139){
    return {"MoveToLandmark", "t5", "kitchen", "kitchen"};
}
if (index == 140){
    return {"MakeCall", "caregiver_call", "t4", "nathan", "guide_2_msg"};
}
if (index == 141){
    return {"MakeCall", "caregiver_call", "t4", "nathan", "guide_1_msg"};
}
if (index == 142){
    return {"MakeCall", "caregiver_call", "t4", "nathan", "automated_msg"};
}
if (index == 143){
    return {"MakeCall", "caregiver_call", "t4", "nathan", "recorded_msg"};
}
if (index == 144){
    return {"MakeCall", "caregiver_call", "t4", "nathan", "call_caregiver_msg"};
}
if (index == 145){
    return {"MakeCall", "caregiver_call", "t4", "nathan", "call_caregiver_guide_msg"};
}
if (index == 146){
    return {"MakeCall", "caregiver_call", "t1", "nathan", "guide_2_msg"};
}
if (index == 147){
    return {"MakeCall", "caregiver_call", "t1", "nathan", "guide_1_msg"};
}
if (index == 148){
    return {"MakeCall", "caregiver_call", "t1", "nathan", "automated_msg"};
}
if (index == 149){
    return {"MakeCall", "caregiver_call", "t1", "nathan", "recorded_msg"};
}
if (index == 150){
    return {"MakeCall", "caregiver_call", "t1", "nathan", "call_caregiver_msg"};
}
if (index == 151){
    return {"MakeCall", "caregiver_call", "t1", "nathan", "call_caregiver_guide_msg"};
}
if (index == 152){
    return {"MakeCall", "caregiver_call", "t2", "nathan", "guide_2_msg"};
}
if (index == 153){
    return {"MakeCall", "caregiver_call", "t2", "nathan", "guide_1_msg"};
}
if (index == 154){
    return {"MakeCall", "caregiver_call", "t2", "nathan", "automated_msg"};
}
if (index == 155){
    return {"MakeCall", "caregiver_call", "t2", "nathan", "recorded_msg"};
}
if (index == 156){
    return {"MakeCall", "caregiver_call", "t2", "nathan", "call_caregiver_msg"};
}
if (index == 157){
    return {"MakeCall", "caregiver_call", "t2", "nathan", "call_caregiver_guide_msg"};
}
if (index == 158){
    return {"MakeCall", "caregiver_call", "t3", "nathan", "guide_2_msg"};
}
if (index == 159){
    return {"MakeCall", "caregiver_call", "t3", "nathan", "guide_1_msg"};
}
if (index == 160){
    return {"MakeCall", "caregiver_call", "t3", "nathan", "automated_msg"};
}
if (index == 161){
    return {"MakeCall", "caregiver_call", "t3", "nathan", "recorded_msg"};
}
if (index == 162){
    return {"MakeCall", "caregiver_call", "t3", "nathan", "call_caregiver_msg"};
}
if (index == 163){
    return {"MakeCall", "caregiver_call", "t3", "nathan", "call_caregiver_guide_msg"};
}
if (index == 164){
    return {"MakeCall", "caregiver_call", "t5", "nathan", "guide_2_msg"};
}
if (index == 165){
    return {"MakeCall", "caregiver_call", "t5", "nathan", "guide_1_msg"};
}
if (index == 166){
    return {"MakeCall", "caregiver_call", "t5", "nathan", "automated_msg"};
}
if (index == 167){
    return {"MakeCall", "caregiver_call", "t5", "nathan", "recorded_msg"};
}
if (index == 168){
    return {"MakeCall", "caregiver_call", "t5", "nathan", "call_caregiver_msg"};
}
if (index == 169){
    return {"MakeCall", "caregiver_call", "t5", "nathan", "call_caregiver_guide_msg"};
}
if (index == 170){
    return {"MakeCall", "caregiver_call_guide", "t4", "nathan", "guide_2_msg"};
}
if (index == 171){
    return {"MakeCall", "caregiver_call_guide", "t4", "nathan", "guide_1_msg"};
}
if (index == 172){
    return {"MakeCall", "caregiver_call_guide", "t4", "nathan", "automated_msg"};
}
if (index == 173){
    return {"MakeCall", "caregiver_call_guide", "t4", "nathan", "recorded_msg"};
}
if (index == 174){
    return {"MakeCall", "caregiver_call_guide", "t4", "nathan", "call_caregiver_msg"};
}
if (index == 175){
    return {"MakeCall", "caregiver_call_guide", "t4", "nathan", "call_caregiver_guide_msg"};
}
if (index == 176){
    return {"MakeCall", "caregiver_call_guide", "t1", "nathan", "guide_2_msg"};
}
if (index == 177){
    return {"MakeCall", "caregiver_call_guide", "t1", "nathan", "guide_1_msg"};
}
if (index == 178){
    return {"MakeCall", "caregiver_call_guide", "t1", "nathan", "automated_msg"};
}
if (index == 179){
    return {"MakeCall", "caregiver_call_guide", "t1", "nathan", "recorded_msg"};
}
if (index == 180){
    return {"MakeCall", "caregiver_call_guide", "t1", "nathan", "call_caregiver_msg"};
}
if (index == 181){
    return {"MakeCall", "caregiver_call_guide", "t1", "nathan", "call_caregiver_guide_msg"};
}
if (index == 182){
    return {"MakeCall", "caregiver_call_guide", "t2", "nathan", "guide_2_msg"};
}
if (index == 183){
    return {"MakeCall", "caregiver_call_guide", "t2", "nathan", "guide_1_msg"};
}
if (index == 184){
    return {"MakeCall", "caregiver_call_guide", "t2", "nathan", "automated_msg"};
}
if (index == 185){
    return {"MakeCall", "caregiver_call_guide", "t2", "nathan", "recorded_msg"};
}
if (index == 186){
    return {"MakeCall", "caregiver_call_guide", "t2", "nathan", "call_caregiver_msg"};
}
if (index == 187){
    return {"MakeCall", "caregiver_call_guide", "t2", "nathan", "call_caregiver_guide_msg"};
}
if (index == 188){
    return {"MakeCall", "caregiver_call_guide", "t3", "nathan", "guide_2_msg"};
}
if (index == 189){
    return {"MakeCall", "caregiver_call_guide", "t3", "nathan", "guide_1_msg"};
}
if (index == 190){
    return {"MakeCall", "caregiver_call_guide", "t3", "nathan", "automated_msg"};
}
if (index == 191){
    return {"MakeCall", "caregiver_call_guide", "t3", "nathan", "recorded_msg"};
}
if (index == 192){
    return {"MakeCall", "caregiver_call_guide", "t3", "nathan", "call_caregiver_msg"};
}
if (index == 193){
    return {"MakeCall", "caregiver_call_guide", "t3", "nathan", "call_caregiver_guide_msg"};
}
if (index == 194){
    return {"MakeCall", "caregiver_call_guide", "t5", "nathan", "guide_2_msg"};
}
if (index == 195){
    return {"MakeCall", "caregiver_call_guide", "t5", "nathan", "guide_1_msg"};
}
if (index == 196){
    return {"MakeCall", "caregiver_call_guide", "t5", "nathan", "automated_msg"};
}
if (index == 197){
    return {"MakeCall", "caregiver_call_guide", "t5", "nathan", "recorded_msg"};
}
if (index == 198){
    return {"MakeCall", "caregiver_call_guide", "t5", "nathan", "call_caregiver_msg"};
}
if (index == 199){
    return {"MakeCall", "caregiver_call_guide", "t5", "nathan", "call_caregiver_guide_msg"};
}
if (index == 200){
    return {"MessageGivenSuccess"};
}
if (index == 201){
    return {"MedicineTakenSuccess"};
}
if (index == 202){
    return {"FoodEatenSuccess"};
}
if (index == 203){
    return {"PersonAtSuccess", "nathan", "t4", "couch"};
}
if (index == 204){
    return {"PersonAtSuccess", "nathan", "t4", "kitchen"};
}
if (index == 205){
    return {"PersonAtSuccess", "nathan", "t1", "couch"};
}
if (index == 206){
    return {"PersonAtSuccess", "nathan", "t1", "kitchen"};
}
if (index == 207){
    return {"PersonAtSuccess", "nathan", "t2", "couch"};
}
if (index == 208){
    return {"PersonAtSuccess", "nathan", "t2", "kitchen"};
}
if (index == 209){
    return {"PersonAtSuccess", "nathan", "t3", "couch"};
}
if (index == 210){
    return {"PersonAtSuccess", "nathan", "t3", "kitchen"};
}
if (index == 211){
    return {"PersonAtSuccess", "nathan", "t5", "couch"};
}
if (index == 212){
    return {"PersonAtSuccess", "nathan", "t5", "kitchen"};
}


if (index == 213){
    return {"DetectTakingMedicine", "t4"};
}
if (index == 214){
    return {"DetectTakingMedicine", "t1"};
}
if (index == 215){
    return {"DetectTakingMedicine", "t2"};
}
if (index == 216){
    return {"DetectTakingMedicine", "t3"};
}
if (index == 217){
    return {"DetectTakingMedicine", "t5"};
}
if (index == 218){
    return {"DetectEatingFood", "t4"};
}
if (index == 219){
    return {"DetectEatingFood", "t1"};
}
if (index == 220){
    return {"DetectEatingFood", "t2"};
}
if (index == 221){
    return {"DetectEatingFood", "t3"};
}
if (index == 222){
    return {"DetectEatingFood", "t5"};
}
if (index == 223){
    return {"DetectPersonLocation", "t4", "nathan", "couch"};
}
if (index == 224){
    return {"DetectPersonLocation", "t4", "nathan", "kitchen"};
}
if (index == 225){
    return {"DetectPersonLocation", "t1", "nathan", "couch"};
}
if (index == 226){
    return {"DetectPersonLocation", "t1", "nathan", "kitchen"};
}
if (index == 227){
    return {"DetectPersonLocation", "t2", "nathan", "couch"};
}
if (index == 228){
    return {"DetectPersonLocation", "t2", "nathan", "kitchen"};
}
if (index == 229){
    return {"DetectPersonLocation", "t3", "nathan", "couch"};
}
if (index == 230){
    return {"DetectPersonLocation", "t3", "nathan", "kitchen"};
}
if (index == 231){
    return {"DetectPersonLocation", "t5", "nathan", "couch"};
}
if (index == 232){
    return {"DetectPersonLocation", "t5", "nathan", "kitchen"};
}
    throw std::runtime_error("index not in action set");
}

std::string get_state_string(const KBState & state){
    std::stringstream ss;

if (state.data[0]==1){
    ss << "next_timet4t4\n";
}
if (state.data[0]==2){
    ss << "unknown(next_timet4t4)\n";
}
if (state.data[1]==1){
    ss << "next_timet4t1\n";
}
if (state.data[1]==2){
    ss << "unknown(next_timet4t1)\n";
}
if (state.data[2]==1){
    ss << "next_timet4t2\n";
}
if (state.data[2]==2){
    ss << "unknown(next_timet4t2)\n";
}
if (state.data[3]==1){
    ss << "next_timet4t3\n";
}
if (state.data[3]==2){
    ss << "unknown(next_timet4t3)\n";
}
if (state.data[4]==1){
    ss << "next_timet4t5\n";
}
if (state.data[4]==2){
    ss << "unknown(next_timet4t5)\n";
}
if (state.data[5]==1){
    ss << "next_timet1t4\n";
}
if (state.data[5]==2){
    ss << "unknown(next_timet1t4)\n";
}
if (state.data[6]==1){
    ss << "next_timet1t1\n";
}
if (state.data[6]==2){
    ss << "unknown(next_timet1t1)\n";
}
if (state.data[7]==1){
    ss << "next_timet1t2\n";
}
if (state.data[7]==2){
    ss << "unknown(next_timet1t2)\n";
}
if (state.data[8]==1){
    ss << "next_timet1t3\n";
}
if (state.data[8]==2){
    ss << "unknown(next_timet1t3)\n";
}
if (state.data[9]==1){
    ss << "next_timet1t5\n";
}
if (state.data[9]==2){
    ss << "unknown(next_timet1t5)\n";
}
if (state.data[10]==1){
    ss << "next_timet2t4\n";
}
if (state.data[10]==2){
    ss << "unknown(next_timet2t4)\n";
}
if (state.data[11]==1){
    ss << "next_timet2t1\n";
}
if (state.data[11]==2){
    ss << "unknown(next_timet2t1)\n";
}
if (state.data[12]==1){
    ss << "next_timet2t2\n";
}
if (state.data[12]==2){
    ss << "unknown(next_timet2t2)\n";
}
if (state.data[13]==1){
    ss << "next_timet2t3\n";
}
if (state.data[13]==2){
    ss << "unknown(next_timet2t3)\n";
}
if (state.data[14]==1){
    ss << "next_timet2t5\n";
}
if (state.data[14]==2){
    ss << "unknown(next_timet2t5)\n";
}
if (state.data[15]==1){
    ss << "next_timet3t4\n";
}
if (state.data[15]==2){
    ss << "unknown(next_timet3t4)\n";
}
if (state.data[16]==1){
    ss << "next_timet3t1\n";
}
if (state.data[16]==2){
    ss << "unknown(next_timet3t1)\n";
}
if (state.data[17]==1){
    ss << "next_timet3t2\n";
}
if (state.data[17]==2){
    ss << "unknown(next_timet3t2)\n";
}
if (state.data[18]==1){
    ss << "next_timet3t3\n";
}
if (state.data[18]==2){
    ss << "unknown(next_timet3t3)\n";
}
if (state.data[19]==1){
    ss << "next_timet3t5\n";
}
if (state.data[19]==2){
    ss << "unknown(next_timet3t5)\n";
}
if (state.data[20]==1){
    ss << "next_timet5t4\n";
}
if (state.data[20]==2){
    ss << "unknown(next_timet5t4)\n";
}
if (state.data[21]==1){
    ss << "next_timet5t1\n";
}
if (state.data[21]==2){
    ss << "unknown(next_timet5t1)\n";
}
if (state.data[22]==1){
    ss << "next_timet5t2\n";
}
if (state.data[22]==2){
    ss << "unknown(next_timet5t2)\n";
}
if (state.data[23]==1){
    ss << "next_timet5t3\n";
}
if (state.data[23]==2){
    ss << "unknown(next_timet5t3)\n";
}
if (state.data[24]==1){
    ss << "next_timet5t5\n";
}
if (state.data[24]==2){
    ss << "unknown(next_timet5t5)\n";
}
if (state.data[25]==1){
    ss << "DetectEatingFood_enabled\n";
}
if (state.data[25]==2){
    ss << "unknown(DetectEatingFood_enabled)\n";
}
if (state.data[26]==1){
    ss << "call_person_not_eating_food_constraintcaregiver_callnathan\n";
}
if (state.data[26]==2){
    ss << "unknown(call_person_not_eating_food_constraintcaregiver_callnathan)\n";
}
if (state.data[27]==1){
    ss << "call_person_not_eating_food_constraintcaregiver_call_guidenathan\n";
}
if (state.data[27]==2){
    ss << "unknown(call_person_not_eating_food_constraintcaregiver_call_guidenathan)\n";
}
if (state.data[28]==1){
    ss << "call_person_location_constraintcaregiver_callnathancouch\n";
}
if (state.data[28]==2){
    ss << "unknown(call_person_location_constraintcaregiver_callnathancouch)\n";
}
if (state.data[29]==1){
    ss << "call_person_location_constraintcaregiver_callnathankitchen\n";
}
if (state.data[29]==2){
    ss << "unknown(call_person_location_constraintcaregiver_callnathankitchen)\n";
}
if (state.data[30]==1){
    ss << "call_person_location_constraintcaregiver_call_guidenathancouch\n";
}
if (state.data[30]==2){
    ss << "unknown(call_person_location_constraintcaregiver_call_guidenathancouch)\n";
}
if (state.data[31]==1){
    ss << "call_person_location_constraintcaregiver_call_guidenathankitchen\n";
}
if (state.data[31]==2){
    ss << "unknown(call_person_location_constraintcaregiver_call_guidenathankitchen)\n";
}
if (state.data[32]==1){
    ss << "current_timet4\n";
}
if (state.data[32]==2){
    ss << "unknown(current_timet4)\n";
}
if (state.data[33]==1){
    ss << "current_timet1\n";
}
if (state.data[33]==2){
    ss << "unknown(current_timet1)\n";
}
if (state.data[34]==1){
    ss << "current_timet2\n";
}
if (state.data[34]==2){
    ss << "unknown(current_timet2)\n";
}
if (state.data[35]==1){
    ss << "current_timet3\n";
}
if (state.data[35]==2){
    ss << "unknown(current_timet3)\n";
}
if (state.data[36]==1){
    ss << "current_timet5\n";
}
if (state.data[36]==2){
    ss << "unknown(current_timet5)\n";
}
if (state.data[37]==1){
    ss << "executed_reminderautomated_reminder\n";
}
if (state.data[37]==2){
    ss << "unknown(executed_reminderautomated_reminder)\n";
}
if (state.data[38]==1){
    ss << "executed_reminderguide_reminder_2\n";
}
if (state.data[38]==2){
    ss << "unknown(executed_reminderguide_reminder_2)\n";
}
if (state.data[39]==1){
    ss << "executed_reminderguide_reminder_1\n";
}
if (state.data[39]==2){
    ss << "unknown(executed_reminderguide_reminder_1)\n";
}
if (state.data[40]==1){
    ss << "executed_reminderrecorded_reminder\n";
}
if (state.data[40]==2){
    ss << "unknown(executed_reminderrecorded_reminder)\n";
}
if (state.data[41]==1){
    ss << "time_critical\n";
}
if (state.data[41]==2){
    ss << "unknown(time_critical)\n";
}
if (state.data[42]==1){
    ss << "call_blocks_callcaregiver_callcaregiver_call\n";
}
if (state.data[42]==2){
    ss << "unknown(call_blocks_callcaregiver_callcaregiver_call)\n";
}
if (state.data[43]==1){
    ss << "call_blocks_callcaregiver_callcaregiver_call_guide\n";
}
if (state.data[43]==2){
    ss << "unknown(call_blocks_callcaregiver_callcaregiver_call_guide)\n";
}
if (state.data[44]==1){
    ss << "call_blocks_callcaregiver_call_guidecaregiver_call\n";
}
if (state.data[44]==2){
    ss << "unknown(call_blocks_callcaregiver_call_guidecaregiver_call)\n";
}
if (state.data[45]==1){
    ss << "call_blocks_callcaregiver_call_guidecaregiver_call_guide\n";
}
if (state.data[45]==2){
    ss << "unknown(call_blocks_callcaregiver_call_guidecaregiver_call_guide)\n";
}
if (state.data[46]==1){
    ss << "valid_call_messagecaregiver_callguide_2_msg\n";
}
if (state.data[46]==2){
    ss << "unknown(valid_call_messagecaregiver_callguide_2_msg)\n";
}
if (state.data[47]==1){
    ss << "valid_call_messagecaregiver_callguide_1_msg\n";
}
if (state.data[47]==2){
    ss << "unknown(valid_call_messagecaregiver_callguide_1_msg)\n";
}
if (state.data[48]==1){
    ss << "valid_call_messagecaregiver_callautomated_msg\n";
}
if (state.data[48]==2){
    ss << "unknown(valid_call_messagecaregiver_callautomated_msg)\n";
}
if (state.data[49]==1){
    ss << "valid_call_messagecaregiver_callrecorded_msg\n";
}
if (state.data[49]==2){
    ss << "unknown(valid_call_messagecaregiver_callrecorded_msg)\n";
}
if (state.data[50]==1){
    ss << "valid_call_messagecaregiver_callcall_caregiver_msg\n";
}
if (state.data[50]==2){
    ss << "unknown(valid_call_messagecaregiver_callcall_caregiver_msg)\n";
}
if (state.data[51]==1){
    ss << "valid_call_messagecaregiver_callcall_caregiver_guide_msg\n";
}
if (state.data[51]==2){
    ss << "unknown(valid_call_messagecaregiver_callcall_caregiver_guide_msg)\n";
}
if (state.data[52]==1){
    ss << "valid_call_messagecaregiver_call_guideguide_2_msg\n";
}
if (state.data[52]==2){
    ss << "unknown(valid_call_messagecaregiver_call_guideguide_2_msg)\n";
}
if (state.data[53]==1){
    ss << "valid_call_messagecaregiver_call_guideguide_1_msg\n";
}
if (state.data[53]==2){
    ss << "unknown(valid_call_messagecaregiver_call_guideguide_1_msg)\n";
}
if (state.data[54]==1){
    ss << "valid_call_messagecaregiver_call_guideautomated_msg\n";
}
if (state.data[54]==2){
    ss << "unknown(valid_call_messagecaregiver_call_guideautomated_msg)\n";
}
if (state.data[55]==1){
    ss << "valid_call_messagecaregiver_call_guiderecorded_msg\n";
}
if (state.data[55]==2){
    ss << "unknown(valid_call_messagecaregiver_call_guiderecorded_msg)\n";
}
if (state.data[56]==1){
    ss << "valid_call_messagecaregiver_call_guidecall_caregiver_msg\n";
}
if (state.data[56]==2){
    ss << "unknown(valid_call_messagecaregiver_call_guidecall_caregiver_msg)\n";
}
if (state.data[57]==1){
    ss << "valid_call_messagecaregiver_call_guidecall_caregiver_guide_msg\n";
}
if (state.data[57]==2){
    ss << "unknown(valid_call_messagecaregiver_call_guidecall_caregiver_guide_msg)\n";
}
if (state.data[58]==1){
    ss << "DetectTakingMedicine_enabled\n";
}
if (state.data[58]==2){
    ss << "unknown(DetectTakingMedicine_enabled)\n";
}
if (state.data[59]==1){
    ss << "executed_callcaregiver_call\n";
}
if (state.data[59]==2){
    ss << "unknown(executed_callcaregiver_call)\n";
}
if (state.data[60]==1){
    ss << "executed_callcaregiver_call_guide\n";
}
if (state.data[60]==2){
    ss << "unknown(executed_callcaregiver_call_guide)\n";
}
if (state.data[61]==1){
    ss << "person_att4nathancouch\n";
}
if (state.data[61]==2){
    ss << "unknown(person_att4nathancouch)\n";
}
if (state.data[62]==1){
    ss << "person_att4nathankitchen\n";
}
if (state.data[62]==2){
    ss << "unknown(person_att4nathankitchen)\n";
}
if (state.data[63]==1){
    ss << "person_att1nathancouch\n";
}
if (state.data[63]==2){
    ss << "unknown(person_att1nathancouch)\n";
}
if (state.data[64]==1){
    ss << "person_att1nathankitchen\n";
}
if (state.data[64]==2){
    ss << "unknown(person_att1nathankitchen)\n";
}
if (state.data[65]==1){
    ss << "person_att2nathancouch\n";
}
if (state.data[65]==2){
    ss << "unknown(person_att2nathancouch)\n";
}
if (state.data[66]==1){
    ss << "person_att2nathankitchen\n";
}
if (state.data[66]==2){
    ss << "unknown(person_att2nathankitchen)\n";
}
if (state.data[67]==1){
    ss << "person_att3nathancouch\n";
}
if (state.data[67]==2){
    ss << "unknown(person_att3nathancouch)\n";
}
if (state.data[68]==1){
    ss << "person_att3nathankitchen\n";
}
if (state.data[68]==2){
    ss << "unknown(person_att3nathankitchen)\n";
}
if (state.data[69]==1){
    ss << "person_att5nathancouch\n";
}
if (state.data[69]==2){
    ss << "unknown(person_att5nathancouch)\n";
}
if (state.data[70]==1){
    ss << "person_att5nathankitchen\n";
}
if (state.data[70]==2){
    ss << "unknown(person_att5nathankitchen)\n";
}
if (state.data[71]==1){
    ss << "valid_reminder_messageautomated_reminderguide_2_msg\n";
}
if (state.data[71]==2){
    ss << "unknown(valid_reminder_messageautomated_reminderguide_2_msg)\n";
}
if (state.data[72]==1){
    ss << "valid_reminder_messageautomated_reminderguide_1_msg\n";
}
if (state.data[72]==2){
    ss << "unknown(valid_reminder_messageautomated_reminderguide_1_msg)\n";
}
if (state.data[73]==1){
    ss << "valid_reminder_messageautomated_reminderautomated_msg\n";
}
if (state.data[73]==2){
    ss << "unknown(valid_reminder_messageautomated_reminderautomated_msg)\n";
}
if (state.data[74]==1){
    ss << "valid_reminder_messageautomated_reminderrecorded_msg\n";
}
if (state.data[74]==2){
    ss << "unknown(valid_reminder_messageautomated_reminderrecorded_msg)\n";
}
if (state.data[75]==1){
    ss << "valid_reminder_messageautomated_remindercall_caregiver_msg\n";
}
if (state.data[75]==2){
    ss << "unknown(valid_reminder_messageautomated_remindercall_caregiver_msg)\n";
}
if (state.data[76]==1){
    ss << "valid_reminder_messageautomated_remindercall_caregiver_guide_msg\n";
}
if (state.data[76]==2){
    ss << "unknown(valid_reminder_messageautomated_remindercall_caregiver_guide_msg)\n";
}
if (state.data[77]==1){
    ss << "valid_reminder_messageguide_reminder_2guide_2_msg\n";
}
if (state.data[77]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_2guide_2_msg)\n";
}
if (state.data[78]==1){
    ss << "valid_reminder_messageguide_reminder_2guide_1_msg\n";
}
if (state.data[78]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_2guide_1_msg)\n";
}
if (state.data[79]==1){
    ss << "valid_reminder_messageguide_reminder_2automated_msg\n";
}
if (state.data[79]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_2automated_msg)\n";
}
if (state.data[80]==1){
    ss << "valid_reminder_messageguide_reminder_2recorded_msg\n";
}
if (state.data[80]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_2recorded_msg)\n";
}
if (state.data[81]==1){
    ss << "valid_reminder_messageguide_reminder_2call_caregiver_msg\n";
}
if (state.data[81]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_2call_caregiver_msg)\n";
}
if (state.data[82]==1){
    ss << "valid_reminder_messageguide_reminder_2call_caregiver_guide_msg\n";
}
if (state.data[82]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_2call_caregiver_guide_msg)\n";
}
if (state.data[83]==1){
    ss << "valid_reminder_messageguide_reminder_1guide_2_msg\n";
}
if (state.data[83]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_1guide_2_msg)\n";
}
if (state.data[84]==1){
    ss << "valid_reminder_messageguide_reminder_1guide_1_msg\n";
}
if (state.data[84]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_1guide_1_msg)\n";
}
if (state.data[85]==1){
    ss << "valid_reminder_messageguide_reminder_1automated_msg\n";
}
if (state.data[85]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_1automated_msg)\n";
}
if (state.data[86]==1){
    ss << "valid_reminder_messageguide_reminder_1recorded_msg\n";
}
if (state.data[86]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_1recorded_msg)\n";
}
if (state.data[87]==1){
    ss << "valid_reminder_messageguide_reminder_1call_caregiver_msg\n";
}
if (state.data[87]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_1call_caregiver_msg)\n";
}
if (state.data[88]==1){
    ss << "valid_reminder_messageguide_reminder_1call_caregiver_guide_msg\n";
}
if (state.data[88]==2){
    ss << "unknown(valid_reminder_messageguide_reminder_1call_caregiver_guide_msg)\n";
}
if (state.data[89]==1){
    ss << "valid_reminder_messagerecorded_reminderguide_2_msg\n";
}
if (state.data[89]==2){
    ss << "unknown(valid_reminder_messagerecorded_reminderguide_2_msg)\n";
}
if (state.data[90]==1){
    ss << "valid_reminder_messagerecorded_reminderguide_1_msg\n";
}
if (state.data[90]==2){
    ss << "unknown(valid_reminder_messagerecorded_reminderguide_1_msg)\n";
}
if (state.data[91]==1){
    ss << "valid_reminder_messagerecorded_reminderautomated_msg\n";
}
if (state.data[91]==2){
    ss << "unknown(valid_reminder_messagerecorded_reminderautomated_msg)\n";
}
if (state.data[92]==1){
    ss << "valid_reminder_messagerecorded_reminderrecorded_msg\n";
}
if (state.data[92]==2){
    ss << "unknown(valid_reminder_messagerecorded_reminderrecorded_msg)\n";
}
if (state.data[93]==1){
    ss << "valid_reminder_messagerecorded_remindercall_caregiver_msg\n";
}
if (state.data[93]==2){
    ss << "unknown(valid_reminder_messagerecorded_remindercall_caregiver_msg)\n";
}
if (state.data[94]==1){
    ss << "valid_reminder_messagerecorded_remindercall_caregiver_guide_msg\n";
}
if (state.data[94]==2){
    ss << "unknown(valid_reminder_messagerecorded_remindercall_caregiver_guide_msg)\n";
}
if (state.data[95]==1){
    ss << "message_givenguide_2_msg\n";
}
if (state.data[95]==2){
    ss << "unknown(message_givenguide_2_msg)\n";
}
if (state.data[96]==1){
    ss << "message_givenguide_1_msg\n";
}
if (state.data[96]==2){
    ss << "unknown(message_givenguide_1_msg)\n";
}
if (state.data[97]==1){
    ss << "message_givenautomated_msg\n";
}
if (state.data[97]==2){
    ss << "unknown(message_givenautomated_msg)\n";
}
if (state.data[98]==1){
    ss << "message_givenrecorded_msg\n";
}
if (state.data[98]==2){
    ss << "unknown(message_givenrecorded_msg)\n";
}
if (state.data[99]==1){
    ss << "message_givencall_caregiver_msg\n";
}
if (state.data[99]==2){
    ss << "unknown(message_givencall_caregiver_msg)\n";
}
if (state.data[100]==1){
    ss << "message_givencall_caregiver_guide_msg\n";
}
if (state.data[100]==2){
    ss << "unknown(message_givencall_caregiver_guide_msg)\n";
}
if (state.data[101]==1){
    ss << "bed_locationcouch\n";
}
if (state.data[101]==2){
    ss << "unknown(bed_locationcouch)\n";
}
if (state.data[102]==1){
    ss << "bed_locationkitchen\n";
}
if (state.data[102]==2){
    ss << "unknown(bed_locationkitchen)\n";
}
if (state.data[103]==1){
    ss << "medicine_taken_success\n";
}
if (state.data[103]==2){
    ss << "unknown(medicine_taken_success)\n";
}
if (state.data[104]==1){
    ss << "reminder_person_not_eating_food_constraintautomated_remindernathan\n";
}
if (state.data[104]==2){
    ss << "unknown(reminder_person_not_eating_food_constraintautomated_remindernathan)\n";
}
if (state.data[105]==1){
    ss << "reminder_person_not_eating_food_constraintguide_reminder_2nathan\n";
}
if (state.data[105]==2){
    ss << "unknown(reminder_person_not_eating_food_constraintguide_reminder_2nathan)\n";
}
if (state.data[106]==1){
    ss << "reminder_person_not_eating_food_constraintguide_reminder_1nathan\n";
}
if (state.data[106]==2){
    ss << "unknown(reminder_person_not_eating_food_constraintguide_reminder_1nathan)\n";
}
if (state.data[107]==1){
    ss << "reminder_person_not_eating_food_constraintrecorded_remindernathan\n";
}
if (state.data[107]==2){
    ss << "unknown(reminder_person_not_eating_food_constraintrecorded_remindernathan)\n";
}
if (state.data[108]==1){
    ss << "person_on_groundt4\n";
}
if (state.data[108]==2){
    ss << "unknown(person_on_groundt4)\n";
}
if (state.data[109]==1){
    ss << "person_on_groundt1\n";
}
if (state.data[109]==2){
    ss << "unknown(person_on_groundt1)\n";
}
if (state.data[110]==1){
    ss << "person_on_groundt2\n";
}
if (state.data[110]==2){
    ss << "unknown(person_on_groundt2)\n";
}
if (state.data[111]==1){
    ss << "person_on_groundt3\n";
}
if (state.data[111]==2){
    ss << "unknown(person_on_groundt3)\n";
}
if (state.data[112]==1){
    ss << "person_on_groundt5\n";
}
if (state.data[112]==2){
    ss << "unknown(person_on_groundt5)\n";
}
if (state.data[113]==1){
    ss << "robot_atcouch\n";
}
if (state.data[113]==2){
    ss << "unknown(robot_atcouch)\n";
}
if (state.data[114]==1){
    ss << "robot_atkitchen\n";
}
if (state.data[114]==2){
    ss << "unknown(robot_atkitchen)\n";
}
if (state.data[115]==1){
    ss << "door_locationcouch\n";
}
if (state.data[115]==2){
    ss << "unknown(door_locationcouch)\n";
}
if (state.data[116]==1){
    ss << "door_locationkitchen\n";
}
if (state.data[116]==2){
    ss << "unknown(door_locationkitchen)\n";
}
if (state.data[117]==1){
    ss << "person_eating_foodt4\n";
}
if (state.data[117]==2){
    ss << "unknown(person_eating_foodt4)\n";
}
if (state.data[118]==1){
    ss << "person_eating_foodt1\n";
}
if (state.data[118]==2){
    ss << "unknown(person_eating_foodt1)\n";
}
if (state.data[119]==1){
    ss << "person_eating_foodt2\n";
}
if (state.data[119]==2){
    ss << "unknown(person_eating_foodt2)\n";
}
if (state.data[120]==1){
    ss << "person_eating_foodt3\n";
}
if (state.data[120]==2){
    ss << "unknown(person_eating_foodt3)\n";
}
if (state.data[121]==1){
    ss << "person_eating_foodt5\n";
}
if (state.data[121]==2){
    ss << "unknown(person_eating_foodt5)\n";
}
if (state.data[122]==1){
    ss << "person_at_successnathancouch\n";
}
if (state.data[122]==2){
    ss << "unknown(person_at_successnathancouch)\n";
}
if (state.data[123]==1){
    ss << "person_at_successnathankitchen\n";
}
if (state.data[123]==2){
    ss << "unknown(person_at_successnathankitchen)\n";
}
if (state.data[124]==1){
    ss << "person_taking_medicinet4\n";
}
if (state.data[124]==2){
    ss << "unknown(person_taking_medicinet4)\n";
}
if (state.data[125]==1){
    ss << "person_taking_medicinet1\n";
}
if (state.data[125]==2){
    ss << "unknown(person_taking_medicinet1)\n";
}
if (state.data[126]==1){
    ss << "person_taking_medicinet2\n";
}
if (state.data[126]==2){
    ss << "unknown(person_taking_medicinet2)\n";
}
if (state.data[127]==1){
    ss << "person_taking_medicinet3\n";
}
if (state.data[127]==2){
    ss << "unknown(person_taking_medicinet3)\n";
}
if (state.data[128]==1){
    ss << "person_taking_medicinet5\n";
}
if (state.data[128]==2){
    ss << "unknown(person_taking_medicinet5)\n";
}
if (state.data[129]==1){
    ss << "person_eatingt4\n";
}
if (state.data[129]==2){
    ss << "unknown(person_eatingt4)\n";
}
if (state.data[130]==1){
    ss << "person_eatingt1\n";
}
if (state.data[130]==2){
    ss << "unknown(person_eatingt1)\n";
}
if (state.data[131]==1){
    ss << "person_eatingt2\n";
}
if (state.data[131]==2){
    ss << "unknown(person_eatingt2)\n";
}
if (state.data[132]==1){
    ss << "person_eatingt3\n";
}
if (state.data[132]==2){
    ss << "unknown(person_eatingt3)\n";
}
if (state.data[133]==1){
    ss << "person_eatingt5\n";
}
if (state.data[133]==2){
    ss << "unknown(person_eatingt5)\n";
}
if (state.data[134]==1){
    ss << "GiveReminder_enabled\n";
}
if (state.data[134]==2){
    ss << "unknown(GiveReminder_enabled)\n";
}
if (state.data[135]==1){
    ss << "reminder_blocks_callautomated_remindercaregiver_call\n";
}
if (state.data[135]==2){
    ss << "unknown(reminder_blocks_callautomated_remindercaregiver_call)\n";
}
if (state.data[136]==1){
    ss << "reminder_blocks_callautomated_remindercaregiver_call_guide\n";
}
if (state.data[136]==2){
    ss << "unknown(reminder_blocks_callautomated_remindercaregiver_call_guide)\n";
}
if (state.data[137]==1){
    ss << "reminder_blocks_callguide_reminder_2caregiver_call\n";
}
if (state.data[137]==2){
    ss << "unknown(reminder_blocks_callguide_reminder_2caregiver_call)\n";
}
if (state.data[138]==1){
    ss << "reminder_blocks_callguide_reminder_2caregiver_call_guide\n";
}
if (state.data[138]==2){
    ss << "unknown(reminder_blocks_callguide_reminder_2caregiver_call_guide)\n";
}
if (state.data[139]==1){
    ss << "reminder_blocks_callguide_reminder_1caregiver_call\n";
}
if (state.data[139]==2){
    ss << "unknown(reminder_blocks_callguide_reminder_1caregiver_call)\n";
}
if (state.data[140]==1){
    ss << "reminder_blocks_callguide_reminder_1caregiver_call_guide\n";
}
if (state.data[140]==2){
    ss << "unknown(reminder_blocks_callguide_reminder_1caregiver_call_guide)\n";
}
if (state.data[141]==1){
    ss << "reminder_blocks_callrecorded_remindercaregiver_call\n";
}
if (state.data[141]==2){
    ss << "unknown(reminder_blocks_callrecorded_remindercaregiver_call)\n";
}
if (state.data[142]==1){
    ss << "reminder_blocks_callrecorded_remindercaregiver_call_guide\n";
}
if (state.data[142]==2){
    ss << "unknown(reminder_blocks_callrecorded_remindercaregiver_call_guide)\n";
}
if (state.data[143]==1){
    ss << "food_eaten_success\n";
}
if (state.data[143]==2){
    ss << "unknown(food_eaten_success)\n";
}
if (state.data[144]==1){
    ss << "MakeCall_enabled\n";
}
if (state.data[144]==2){
    ss << "unknown(MakeCall_enabled)\n";
}
if (state.data[145]==1){
    ss << "message_given_successguide_2_msg\n";
}
if (state.data[145]==2){
    ss << "unknown(message_given_successguide_2_msg)\n";
}
if (state.data[146]==1){
    ss << "message_given_successguide_1_msg\n";
}
if (state.data[146]==2){
    ss << "unknown(message_given_successguide_1_msg)\n";
}
if (state.data[147]==1){
    ss << "message_given_successautomated_msg\n";
}
if (state.data[147]==2){
    ss << "unknown(message_given_successautomated_msg)\n";
}
if (state.data[148]==1){
    ss << "message_given_successrecorded_msg\n";
}
if (state.data[148]==2){
    ss << "unknown(message_given_successrecorded_msg)\n";
}
if (state.data[149]==1){
    ss << "message_given_successcall_caregiver_msg\n";
}
if (state.data[149]==2){
    ss << "unknown(message_given_successcall_caregiver_msg)\n";
}
if (state.data[150]==1){
    ss << "message_given_successcall_caregiver_guide_msg\n";
}
if (state.data[150]==2){
    ss << "unknown(message_given_successcall_caregiver_guide_msg)\n";
}
if (state.data[151]==1){
    ss << "reminder_blocks_reminderautomated_reminderautomated_reminder\n";
}
if (state.data[151]==2){
    ss << "unknown(reminder_blocks_reminderautomated_reminderautomated_reminder)\n";
}
if (state.data[152]==1){
    ss << "reminder_blocks_reminderautomated_reminderguide_reminder_2\n";
}
if (state.data[152]==2){
    ss << "unknown(reminder_blocks_reminderautomated_reminderguide_reminder_2)\n";
}
if (state.data[153]==1){
    ss << "reminder_blocks_reminderautomated_reminderguide_reminder_1\n";
}
if (state.data[153]==2){
    ss << "unknown(reminder_blocks_reminderautomated_reminderguide_reminder_1)\n";
}
if (state.data[154]==1){
    ss << "reminder_blocks_reminderautomated_reminderrecorded_reminder\n";
}
if (state.data[154]==2){
    ss << "unknown(reminder_blocks_reminderautomated_reminderrecorded_reminder)\n";
}
if (state.data[155]==1){
    ss << "reminder_blocks_reminderguide_reminder_2automated_reminder\n";
}
if (state.data[155]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_2automated_reminder)\n";
}
if (state.data[156]==1){
    ss << "reminder_blocks_reminderguide_reminder_2guide_reminder_2\n";
}
if (state.data[156]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_2guide_reminder_2)\n";
}
if (state.data[157]==1){
    ss << "reminder_blocks_reminderguide_reminder_2guide_reminder_1\n";
}
if (state.data[157]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_2guide_reminder_1)\n";
}
if (state.data[158]==1){
    ss << "reminder_blocks_reminderguide_reminder_2recorded_reminder\n";
}
if (state.data[158]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_2recorded_reminder)\n";
}
if (state.data[159]==1){
    ss << "reminder_blocks_reminderguide_reminder_1automated_reminder\n";
}
if (state.data[159]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_1automated_reminder)\n";
}
if (state.data[160]==1){
    ss << "reminder_blocks_reminderguide_reminder_1guide_reminder_2\n";
}
if (state.data[160]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_1guide_reminder_2)\n";
}
if (state.data[161]==1){
    ss << "reminder_blocks_reminderguide_reminder_1guide_reminder_1\n";
}
if (state.data[161]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_1guide_reminder_1)\n";
}
if (state.data[162]==1){
    ss << "reminder_blocks_reminderguide_reminder_1recorded_reminder\n";
}
if (state.data[162]==2){
    ss << "unknown(reminder_blocks_reminderguide_reminder_1recorded_reminder)\n";
}
if (state.data[163]==1){
    ss << "reminder_blocks_reminderrecorded_reminderautomated_reminder\n";
}
if (state.data[163]==2){
    ss << "unknown(reminder_blocks_reminderrecorded_reminderautomated_reminder)\n";
}
if (state.data[164]==1){
    ss << "reminder_blocks_reminderrecorded_reminderguide_reminder_2\n";
}
if (state.data[164]==2){
    ss << "unknown(reminder_blocks_reminderrecorded_reminderguide_reminder_2)\n";
}
if (state.data[165]==1){
    ss << "reminder_blocks_reminderrecorded_reminderguide_reminder_1\n";
}
if (state.data[165]==2){
    ss << "unknown(reminder_blocks_reminderrecorded_reminderguide_reminder_1)\n";
}
if (state.data[166]==1){
    ss << "reminder_blocks_reminderrecorded_reminderrecorded_reminder\n";
}
if (state.data[166]==2){
    ss << "unknown(reminder_blocks_reminderrecorded_reminderrecorded_reminder)\n";
}
if (state.data[167]==1){
    ss << "call_not_person_location_constraintcaregiver_callnathancouch\n";
}
if (state.data[167]==2){
    ss << "unknown(call_not_person_location_constraintcaregiver_callnathancouch)\n";
}
if (state.data[168]==1){
    ss << "call_not_person_location_constraintcaregiver_callnathankitchen\n";
}
if (state.data[168]==2){
    ss << "unknown(call_not_person_location_constraintcaregiver_callnathankitchen)\n";
}
if (state.data[169]==1){
    ss << "call_not_person_location_constraintcaregiver_call_guidenathancouch\n";
}
if (state.data[169]==2){
    ss << "unknown(call_not_person_location_constraintcaregiver_call_guidenathancouch)\n";
}
if (state.data[170]==1){
    ss << "call_not_person_location_constraintcaregiver_call_guidenathankitchen\n";
}
if (state.data[170]==2){
    ss << "unknown(call_not_person_location_constraintcaregiver_call_guidenathankitchen)\n";
}
if (state.data[171]==1){
    ss << "success\n";
}
if (state.data[171]==2){
    ss << "unknown(success)\n";
}
if (state.data[172]==1){
    ss << "traversablecouchcouch\n";
}
if (state.data[172]==2){
    ss << "unknown(traversablecouchcouch)\n";
}
if (state.data[173]==1){
    ss << "traversablecouchkitchen\n";
}
if (state.data[173]==2){
    ss << "unknown(traversablecouchkitchen)\n";
}
if (state.data[174]==1){
    ss << "traversablekitchencouch\n";
}
if (state.data[174]==2){
    ss << "unknown(traversablekitchencouch)\n";
}
if (state.data[175]==1){
    ss << "traversablekitchenkitchen\n";
}
if (state.data[175]==2){
    ss << "unknown(traversablekitchenkitchen)\n";
}
if (state.data[176]==1){
    ss << "reminder_not_person_location_constraintautomated_remindernathancouch\n";
}
if (state.data[176]==2){
    ss << "unknown(reminder_not_person_location_constraintautomated_remindernathancouch)\n";
}
if (state.data[177]==1){
    ss << "reminder_not_person_location_constraintautomated_remindernathankitchen\n";
}
if (state.data[177]==2){
    ss << "unknown(reminder_not_person_location_constraintautomated_remindernathankitchen)\n";
}
if (state.data[178]==1){
    ss << "reminder_not_person_location_constraintguide_reminder_2nathancouch\n";
}
if (state.data[178]==2){
    ss << "unknown(reminder_not_person_location_constraintguide_reminder_2nathancouch)\n";
}
if (state.data[179]==1){
    ss << "reminder_not_person_location_constraintguide_reminder_2nathankitchen\n";
}
if (state.data[179]==2){
    ss << "unknown(reminder_not_person_location_constraintguide_reminder_2nathankitchen)\n";
}
if (state.data[180]==1){
    ss << "reminder_not_person_location_constraintguide_reminder_1nathancouch\n";
}
if (state.data[180]==2){
    ss << "unknown(reminder_not_person_location_constraintguide_reminder_1nathancouch)\n";
}
if (state.data[181]==1){
    ss << "reminder_not_person_location_constraintguide_reminder_1nathankitchen\n";
}
if (state.data[181]==2){
    ss << "unknown(reminder_not_person_location_constraintguide_reminder_1nathankitchen)\n";
}
if (state.data[182]==1){
    ss << "reminder_not_person_location_constraintrecorded_remindernathancouch\n";
}
if (state.data[182]==2){
    ss << "unknown(reminder_not_person_location_constraintrecorded_remindernathancouch)\n";
}
if (state.data[183]==1){
    ss << "reminder_not_person_location_constraintrecorded_remindernathankitchen\n";
}
if (state.data[183]==2){
    ss << "unknown(reminder_not_person_location_constraintrecorded_remindernathankitchen)\n";
}
if (state.data[184]==1){
    ss << "used_movet4\n";
}
if (state.data[184]==2){
    ss << "unknown(used_movet4)\n";
}
if (state.data[185]==1){
    ss << "used_movet1\n";
}
if (state.data[185]==2){
    ss << "unknown(used_movet1)\n";
}
if (state.data[186]==1){
    ss << "used_movet2\n";
}
if (state.data[186]==2){
    ss << "unknown(used_movet2)\n";
}
if (state.data[187]==1){
    ss << "used_movet3\n";
}
if (state.data[187]==2){
    ss << "unknown(used_movet3)\n";
}
if (state.data[188]==1){
    ss << "used_movet5\n";
}
if (state.data[188]==2){
    ss << "unknown(used_movet5)\n";
}
if (state.data[189]==1){
    ss << "call_person_not_taking_medicine_constraintcaregiver_callnathan\n";
}
if (state.data[189]==2){
    ss << "unknown(call_person_not_taking_medicine_constraintcaregiver_callnathan)\n";
}
if (state.data[190]==1){
    ss << "call_person_not_taking_medicine_constraintcaregiver_call_guidenathan\n";
}
if (state.data[190]==2){
    ss << "unknown(call_person_not_taking_medicine_constraintcaregiver_call_guidenathan)\n";
}
if (state.data[191]==1){
    ss << "DetectPerson_enabled\n";
}
if (state.data[191]==2){
    ss << "unknown(DetectPerson_enabled)\n";
}
if (state.data[192]==1){
    ss << "reminder_person_location_constraintautomated_remindernathancouch\n";
}
if (state.data[192]==2){
    ss << "unknown(reminder_person_location_constraintautomated_remindernathancouch)\n";
}
if (state.data[193]==1){
    ss << "reminder_person_location_constraintautomated_remindernathankitchen\n";
}
if (state.data[193]==2){
    ss << "unknown(reminder_person_location_constraintautomated_remindernathankitchen)\n";
}
if (state.data[194]==1){
    ss << "reminder_person_location_constraintguide_reminder_2nathancouch\n";
}
if (state.data[194]==2){
    ss << "unknown(reminder_person_location_constraintguide_reminder_2nathancouch)\n";
}
if (state.data[195]==1){
    ss << "reminder_person_location_constraintguide_reminder_2nathankitchen\n";
}
if (state.data[195]==2){
    ss << "unknown(reminder_person_location_constraintguide_reminder_2nathankitchen)\n";
}
if (state.data[196]==1){
    ss << "reminder_person_location_constraintguide_reminder_1nathancouch\n";
}
if (state.data[196]==2){
    ss << "unknown(reminder_person_location_constraintguide_reminder_1nathancouch)\n";
}
if (state.data[197]==1){
    ss << "reminder_person_location_constraintguide_reminder_1nathankitchen\n";
}
if (state.data[197]==2){
    ss << "unknown(reminder_person_location_constraintguide_reminder_1nathankitchen)\n";
}
if (state.data[198]==1){
    ss << "reminder_person_location_constraintrecorded_remindernathancouch\n";
}
if (state.data[198]==2){
    ss << "unknown(reminder_person_location_constraintrecorded_remindernathancouch)\n";
}
if (state.data[199]==1){
    ss << "reminder_person_location_constraintrecorded_remindernathankitchen\n";
}
if (state.data[199]==2){
    ss << "unknown(reminder_person_location_constraintrecorded_remindernathankitchen)\n";
}
if (state.data[200]==1){
    ss << "reminder_person_not_taking_medicine_constraintautomated_remindernathan\n";
}
if (state.data[200]==2){
    ss << "unknown(reminder_person_not_taking_medicine_constraintautomated_remindernathan)\n";
}
if (state.data[201]==1){
    ss << "reminder_person_not_taking_medicine_constraintguide_reminder_2nathan\n";
}
if (state.data[201]==2){
    ss << "unknown(reminder_person_not_taking_medicine_constraintguide_reminder_2nathan)\n";
}
if (state.data[202]==1){
    ss << "reminder_person_not_taking_medicine_constraintguide_reminder_1nathan\n";
}
if (state.data[202]==2){
    ss << "unknown(reminder_person_not_taking_medicine_constraintguide_reminder_1nathan)\n";
}
if (state.data[203]==1){
    ss << "reminder_person_not_taking_medicine_constraintrecorded_remindernathan\n";
}
if (state.data[203]==2){
    ss << "unknown(reminder_person_not_taking_medicine_constraintrecorded_remindernathan)\n";
}
if (state.data[204]==1){
    ss << "time_limitt4\n";
}
if (state.data[204]==2){
    ss << "unknown(time_limitt4)\n";
}
if (state.data[205]==1){
    ss << "time_limitt1\n";
}
if (state.data[205]==2){
    ss << "unknown(time_limitt1)\n";
}
if (state.data[206]==1){
    ss << "time_limitt2\n";
}
if (state.data[206]==2){
    ss << "unknown(time_limitt2)\n";
}
if (state.data[207]==1){
    ss << "time_limitt3\n";
}
if (state.data[207]==2){
    ss << "unknown(time_limitt3)\n";
}
if (state.data[208]==1){
    ss << "time_limitt5\n";
}
if (state.data[208]==2){
    ss << "unknown(time_limitt5)\n";
}

return ss.str();
}

} // indexers
} // pddl_lib

namespace std {
    template<>
    struct equal_to<pddl_lib::KBState*>{
        bool operator()(const pddl_lib::KBState * state1, const pddl_lib::KBState *state2) const {
            if (state1->associated_state == nullptr && state2->associated_state == nullptr){
                return  state1->action == state2->action && std::memcmp(state1->data, state2->data, 216) == 0;
            }
            else if ((state1->associated_state == nullptr && state2->associated_state != nullptr)
                       || (state1->associated_state != nullptr && state2->associated_state == nullptr) ){
                return false;
            } else{
                return state1->action == state2->action && (std::memcmp(state1->data, state2->data, 216) == 0)
                       && (std::memcmp(state1->associated_state->data, state2->associated_state->data, 216) == 0);
            }
        }
    };

    template<>
    struct hash<pddl_lib::KBState*> {
        std::size_t operator()(const pddl_lib::KBState* obj) const {
            std::size_t* data = (std::size_t*) obj->data;
//            std::size_t hashValue = 0;
//            std::hash<std::size_t> intHash;
//            for (size_t i = 0; i < 216/8; ++i) {
//                auto val = intHash(data[i]);
//                hashValue = hashValue ^ val;
//            }
            constexpr std::size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
            constexpr std::size_t FNV_PRIME = 1099511628211ULL;

            std::size_t hashValue = FNV_OFFSET_BASIS;
            for (std::size_t i = 0; i < 216/8; ++i) {
                hashValue ^= data[i];
                hashValue *= FNV_PRIME;
            }

            return hashValue;
        }
    };
}

namespace pddl_lib {


namespace GiveReminder_automated_reminder_t4_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[71]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[124]==0))))) && (((state.data[104]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[193]==0))) && (((((state.data[61]==1)) || state.data[192]==0)))))) && ((((((state.data[62]==0 || state.data[177]==0))) && (((state.data[61]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[37] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t4_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[72]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[124]==0))))) && (((state.data[104]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[193]==0))) && (((((state.data[61]==1)) || state.data[192]==0)))))) && ((((((state.data[62]==0 || state.data[177]==0))) && (((state.data[61]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[37] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t4_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[73]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[124]==0))))) && (((state.data[104]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[193]==0))) && (((((state.data[61]==1)) || state.data[192]==0)))))) && ((((((state.data[62]==0 || state.data[177]==0))) && (((state.data[61]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[37] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t4_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[74]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[124]==0))))) && (((state.data[104]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[193]==0))) && (((((state.data[61]==1)) || state.data[192]==0)))))) && ((((((state.data[62]==0 || state.data[177]==0))) && (((state.data[61]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[37] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t4_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[75]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[124]==0))))) && (((state.data[104]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[193]==0))) && (((((state.data[61]==1)) || state.data[192]==0)))))) && ((((((state.data[62]==0 || state.data[177]==0))) && (((state.data[61]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[37] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t4_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[76]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[124]==0))))) && (((state.data[104]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[193]==0))) && (((((state.data[61]==1)) || state.data[192]==0)))))) && ((((((state.data[62]==0 || state.data[177]==0))) && (((state.data[61]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[37] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t1_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[71]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[125]==0))))) && (((state.data[104]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[193]==0))) && (((((state.data[63]==1)) || state.data[192]==0)))))) && ((((((state.data[64]==0 || state.data[177]==0))) && (((state.data[63]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[37] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t1_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[72]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[125]==0))))) && (((state.data[104]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[193]==0))) && (((((state.data[63]==1)) || state.data[192]==0)))))) && ((((((state.data[64]==0 || state.data[177]==0))) && (((state.data[63]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[37] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t1_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[73]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[125]==0))))) && (((state.data[104]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[193]==0))) && (((((state.data[63]==1)) || state.data[192]==0)))))) && ((((((state.data[64]==0 || state.data[177]==0))) && (((state.data[63]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[37] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t1_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[74]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[125]==0))))) && (((state.data[104]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[193]==0))) && (((((state.data[63]==1)) || state.data[192]==0)))))) && ((((((state.data[64]==0 || state.data[177]==0))) && (((state.data[63]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[37] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t1_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[75]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[125]==0))))) && (((state.data[104]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[193]==0))) && (((((state.data[63]==1)) || state.data[192]==0)))))) && ((((((state.data[64]==0 || state.data[177]==0))) && (((state.data[63]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[37] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t1_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[76]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[125]==0))))) && (((state.data[104]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[193]==0))) && (((((state.data[63]==1)) || state.data[192]==0)))))) && ((((((state.data[64]==0 || state.data[177]==0))) && (((state.data[63]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[37] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t2_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[71]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[126]==0))))) && (((state.data[104]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[193]==0))) && (((((state.data[65]==1)) || state.data[192]==0)))))) && ((((((state.data[66]==0 || state.data[177]==0))) && (((state.data[65]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[37] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t2_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[72]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[126]==0))))) && (((state.data[104]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[193]==0))) && (((((state.data[65]==1)) || state.data[192]==0)))))) && ((((((state.data[66]==0 || state.data[177]==0))) && (((state.data[65]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[37] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t2_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[73]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[126]==0))))) && (((state.data[104]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[193]==0))) && (((((state.data[65]==1)) || state.data[192]==0)))))) && ((((((state.data[66]==0 || state.data[177]==0))) && (((state.data[65]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[37] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t2_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[74]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[126]==0))))) && (((state.data[104]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[193]==0))) && (((((state.data[65]==1)) || state.data[192]==0)))))) && ((((((state.data[66]==0 || state.data[177]==0))) && (((state.data[65]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[37] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t2_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[75]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[126]==0))))) && (((state.data[104]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[193]==0))) && (((((state.data[65]==1)) || state.data[192]==0)))))) && ((((((state.data[66]==0 || state.data[177]==0))) && (((state.data[65]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[37] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t2_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[76]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[126]==0))))) && (((state.data[104]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[193]==0))) && (((((state.data[65]==1)) || state.data[192]==0)))))) && ((((((state.data[66]==0 || state.data[177]==0))) && (((state.data[65]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[37] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t3_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[71]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[127]==0))))) && (((state.data[104]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[193]==0))) && (((((state.data[67]==1)) || state.data[192]==0)))))) && ((((((state.data[68]==0 || state.data[177]==0))) && (((state.data[67]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[37] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t3_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[72]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[127]==0))))) && (((state.data[104]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[193]==0))) && (((((state.data[67]==1)) || state.data[192]==0)))))) && ((((((state.data[68]==0 || state.data[177]==0))) && (((state.data[67]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[37] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t3_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[73]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[127]==0))))) && (((state.data[104]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[193]==0))) && (((((state.data[67]==1)) || state.data[192]==0)))))) && ((((((state.data[68]==0 || state.data[177]==0))) && (((state.data[67]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[37] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t3_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[74]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[127]==0))))) && (((state.data[104]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[193]==0))) && (((((state.data[67]==1)) || state.data[192]==0)))))) && ((((((state.data[68]==0 || state.data[177]==0))) && (((state.data[67]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[37] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t3_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[75]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[127]==0))))) && (((state.data[104]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[193]==0))) && (((((state.data[67]==1)) || state.data[192]==0)))))) && ((((((state.data[68]==0 || state.data[177]==0))) && (((state.data[67]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[37] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t3_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[76]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[127]==0))))) && (((state.data[104]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[193]==0))) && (((((state.data[67]==1)) || state.data[192]==0)))))) && ((((((state.data[68]==0 || state.data[177]==0))) && (((state.data[67]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[37] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t5_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[71]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[128]==0))))) && (((state.data[104]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[193]==0))) && (((((state.data[69]==1)) || state.data[192]==0)))))) && ((((((state.data[70]==0 || state.data[177]==0))) && (((state.data[69]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[37] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t5_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[72]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[128]==0))))) && (((state.data[104]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[193]==0))) && (((((state.data[69]==1)) || state.data[192]==0)))))) && ((((((state.data[70]==0 || state.data[177]==0))) && (((state.data[69]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[37] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t5_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[73]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[128]==0))))) && (((state.data[104]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[193]==0))) && (((((state.data[69]==1)) || state.data[192]==0)))))) && ((((((state.data[70]==0 || state.data[177]==0))) && (((state.data[69]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[37] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t5_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[74]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[128]==0))))) && (((state.data[104]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[193]==0))) && (((((state.data[69]==1)) || state.data[192]==0)))))) && ((((((state.data[70]==0 || state.data[177]==0))) && (((state.data[69]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[37] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t5_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[75]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[128]==0))))) && (((state.data[104]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[193]==0))) && (((((state.data[69]==1)) || state.data[192]==0)))))) && ((((((state.data[70]==0 || state.data[177]==0))) && (((state.data[69]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[37] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_automated_reminder_t5_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[76]==1 && ((state.data[37]==0)) && (((state.data[200]==0 || ((state.data[128]==0))))) && (((state.data[104]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[151]==0 || ((state.data[37]==1))))) && (((state.data[163]==0 || ((state.data[40]==1))))) && (((state.data[159]==0 || ((state.data[39]==1))))) && (((state.data[155]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[193]==0))) && (((((state.data[69]==1)) || state.data[192]==0)))))) && ((((((state.data[70]==0 || state.data[177]==0))) && (((state.data[69]==0 || state.data[176]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[37] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t4_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[77]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[124]==0))))) && (((state.data[105]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[195]==0))) && (((((state.data[61]==1)) || state.data[194]==0)))))) && ((((((state.data[62]==0 || state.data[179]==0))) && (((state.data[61]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[38] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t4_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[78]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[124]==0))))) && (((state.data[105]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[195]==0))) && (((((state.data[61]==1)) || state.data[194]==0)))))) && ((((((state.data[62]==0 || state.data[179]==0))) && (((state.data[61]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[38] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t4_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[79]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[124]==0))))) && (((state.data[105]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[195]==0))) && (((((state.data[61]==1)) || state.data[194]==0)))))) && ((((((state.data[62]==0 || state.data[179]==0))) && (((state.data[61]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[38] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t4_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[80]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[124]==0))))) && (((state.data[105]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[195]==0))) && (((((state.data[61]==1)) || state.data[194]==0)))))) && ((((((state.data[62]==0 || state.data[179]==0))) && (((state.data[61]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[38] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t4_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[81]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[124]==0))))) && (((state.data[105]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[195]==0))) && (((((state.data[61]==1)) || state.data[194]==0)))))) && ((((((state.data[62]==0 || state.data[179]==0))) && (((state.data[61]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[38] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t4_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[82]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[124]==0))))) && (((state.data[105]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[195]==0))) && (((((state.data[61]==1)) || state.data[194]==0)))))) && ((((((state.data[62]==0 || state.data[179]==0))) && (((state.data[61]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[38] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t1_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[77]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[125]==0))))) && (((state.data[105]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[195]==0))) && (((((state.data[63]==1)) || state.data[194]==0)))))) && ((((((state.data[64]==0 || state.data[179]==0))) && (((state.data[63]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[38] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t1_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[78]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[125]==0))))) && (((state.data[105]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[195]==0))) && (((((state.data[63]==1)) || state.data[194]==0)))))) && ((((((state.data[64]==0 || state.data[179]==0))) && (((state.data[63]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[38] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t1_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[79]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[125]==0))))) && (((state.data[105]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[195]==0))) && (((((state.data[63]==1)) || state.data[194]==0)))))) && ((((((state.data[64]==0 || state.data[179]==0))) && (((state.data[63]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[38] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t1_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[80]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[125]==0))))) && (((state.data[105]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[195]==0))) && (((((state.data[63]==1)) || state.data[194]==0)))))) && ((((((state.data[64]==0 || state.data[179]==0))) && (((state.data[63]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[38] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t1_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[81]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[125]==0))))) && (((state.data[105]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[195]==0))) && (((((state.data[63]==1)) || state.data[194]==0)))))) && ((((((state.data[64]==0 || state.data[179]==0))) && (((state.data[63]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[38] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t1_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[82]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[125]==0))))) && (((state.data[105]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[195]==0))) && (((((state.data[63]==1)) || state.data[194]==0)))))) && ((((((state.data[64]==0 || state.data[179]==0))) && (((state.data[63]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[38] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t2_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[77]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[126]==0))))) && (((state.data[105]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[195]==0))) && (((((state.data[65]==1)) || state.data[194]==0)))))) && ((((((state.data[66]==0 || state.data[179]==0))) && (((state.data[65]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[38] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t2_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[78]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[126]==0))))) && (((state.data[105]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[195]==0))) && (((((state.data[65]==1)) || state.data[194]==0)))))) && ((((((state.data[66]==0 || state.data[179]==0))) && (((state.data[65]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[38] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t2_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[79]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[126]==0))))) && (((state.data[105]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[195]==0))) && (((((state.data[65]==1)) || state.data[194]==0)))))) && ((((((state.data[66]==0 || state.data[179]==0))) && (((state.data[65]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[38] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t2_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[80]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[126]==0))))) && (((state.data[105]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[195]==0))) && (((((state.data[65]==1)) || state.data[194]==0)))))) && ((((((state.data[66]==0 || state.data[179]==0))) && (((state.data[65]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[38] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t2_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[81]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[126]==0))))) && (((state.data[105]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[195]==0))) && (((((state.data[65]==1)) || state.data[194]==0)))))) && ((((((state.data[66]==0 || state.data[179]==0))) && (((state.data[65]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[38] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t2_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[82]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[126]==0))))) && (((state.data[105]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[195]==0))) && (((((state.data[65]==1)) || state.data[194]==0)))))) && ((((((state.data[66]==0 || state.data[179]==0))) && (((state.data[65]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[38] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t3_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[77]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[127]==0))))) && (((state.data[105]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[195]==0))) && (((((state.data[67]==1)) || state.data[194]==0)))))) && ((((((state.data[68]==0 || state.data[179]==0))) && (((state.data[67]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[38] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t3_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[78]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[127]==0))))) && (((state.data[105]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[195]==0))) && (((((state.data[67]==1)) || state.data[194]==0)))))) && ((((((state.data[68]==0 || state.data[179]==0))) && (((state.data[67]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[38] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t3_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[79]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[127]==0))))) && (((state.data[105]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[195]==0))) && (((((state.data[67]==1)) || state.data[194]==0)))))) && ((((((state.data[68]==0 || state.data[179]==0))) && (((state.data[67]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[38] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t3_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[80]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[127]==0))))) && (((state.data[105]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[195]==0))) && (((((state.data[67]==1)) || state.data[194]==0)))))) && ((((((state.data[68]==0 || state.data[179]==0))) && (((state.data[67]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[38] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t3_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[81]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[127]==0))))) && (((state.data[105]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[195]==0))) && (((((state.data[67]==1)) || state.data[194]==0)))))) && ((((((state.data[68]==0 || state.data[179]==0))) && (((state.data[67]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[38] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t3_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[82]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[127]==0))))) && (((state.data[105]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[195]==0))) && (((((state.data[67]==1)) || state.data[194]==0)))))) && ((((((state.data[68]==0 || state.data[179]==0))) && (((state.data[67]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[38] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t5_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[77]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[128]==0))))) && (((state.data[105]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[195]==0))) && (((((state.data[69]==1)) || state.data[194]==0)))))) && ((((((state.data[70]==0 || state.data[179]==0))) && (((state.data[69]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[38] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t5_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[78]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[128]==0))))) && (((state.data[105]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[195]==0))) && (((((state.data[69]==1)) || state.data[194]==0)))))) && ((((((state.data[70]==0 || state.data[179]==0))) && (((state.data[69]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[38] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t5_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[79]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[128]==0))))) && (((state.data[105]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[195]==0))) && (((((state.data[69]==1)) || state.data[194]==0)))))) && ((((((state.data[70]==0 || state.data[179]==0))) && (((state.data[69]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[38] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t5_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[80]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[128]==0))))) && (((state.data[105]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[195]==0))) && (((((state.data[69]==1)) || state.data[194]==0)))))) && ((((((state.data[70]==0 || state.data[179]==0))) && (((state.data[69]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[38] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t5_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[81]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[128]==0))))) && (((state.data[105]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[195]==0))) && (((((state.data[69]==1)) || state.data[194]==0)))))) && ((((((state.data[70]==0 || state.data[179]==0))) && (((state.data[69]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[38] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_2_t5_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[82]==1 && ((state.data[38]==0)) && (((state.data[201]==0 || ((state.data[128]==0))))) && (((state.data[105]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[152]==0 || ((state.data[37]==1))))) && (((state.data[164]==0 || ((state.data[40]==1))))) && (((state.data[160]==0 || ((state.data[39]==1))))) && (((state.data[156]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[195]==0))) && (((((state.data[69]==1)) || state.data[194]==0)))))) && ((((((state.data[70]==0 || state.data[179]==0))) && (((state.data[69]==0 || state.data[178]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[38] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t4_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[83]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[124]==0))))) && (((state.data[106]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[197]==0))) && (((((state.data[61]==1)) || state.data[196]==0)))))) && ((((((state.data[62]==0 || state.data[181]==0))) && (((state.data[61]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[39] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t4_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[84]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[124]==0))))) && (((state.data[106]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[197]==0))) && (((((state.data[61]==1)) || state.data[196]==0)))))) && ((((((state.data[62]==0 || state.data[181]==0))) && (((state.data[61]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[39] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t4_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[85]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[124]==0))))) && (((state.data[106]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[197]==0))) && (((((state.data[61]==1)) || state.data[196]==0)))))) && ((((((state.data[62]==0 || state.data[181]==0))) && (((state.data[61]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[39] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t4_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[86]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[124]==0))))) && (((state.data[106]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[197]==0))) && (((((state.data[61]==1)) || state.data[196]==0)))))) && ((((((state.data[62]==0 || state.data[181]==0))) && (((state.data[61]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[39] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t4_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[87]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[124]==0))))) && (((state.data[106]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[197]==0))) && (((((state.data[61]==1)) || state.data[196]==0)))))) && ((((((state.data[62]==0 || state.data[181]==0))) && (((state.data[61]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[39] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t4_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[88]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[124]==0))))) && (((state.data[106]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[197]==0))) && (((((state.data[61]==1)) || state.data[196]==0)))))) && ((((((state.data[62]==0 || state.data[181]==0))) && (((state.data[61]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[39] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t1_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[83]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[125]==0))))) && (((state.data[106]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[197]==0))) && (((((state.data[63]==1)) || state.data[196]==0)))))) && ((((((state.data[64]==0 || state.data[181]==0))) && (((state.data[63]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[39] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t1_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[84]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[125]==0))))) && (((state.data[106]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[197]==0))) && (((((state.data[63]==1)) || state.data[196]==0)))))) && ((((((state.data[64]==0 || state.data[181]==0))) && (((state.data[63]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[39] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t1_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[85]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[125]==0))))) && (((state.data[106]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[197]==0))) && (((((state.data[63]==1)) || state.data[196]==0)))))) && ((((((state.data[64]==0 || state.data[181]==0))) && (((state.data[63]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[39] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t1_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[86]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[125]==0))))) && (((state.data[106]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[197]==0))) && (((((state.data[63]==1)) || state.data[196]==0)))))) && ((((((state.data[64]==0 || state.data[181]==0))) && (((state.data[63]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[39] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t1_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[87]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[125]==0))))) && (((state.data[106]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[197]==0))) && (((((state.data[63]==1)) || state.data[196]==0)))))) && ((((((state.data[64]==0 || state.data[181]==0))) && (((state.data[63]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[39] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t1_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[88]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[125]==0))))) && (((state.data[106]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[197]==0))) && (((((state.data[63]==1)) || state.data[196]==0)))))) && ((((((state.data[64]==0 || state.data[181]==0))) && (((state.data[63]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[39] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t2_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[83]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[126]==0))))) && (((state.data[106]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[197]==0))) && (((((state.data[65]==1)) || state.data[196]==0)))))) && ((((((state.data[66]==0 || state.data[181]==0))) && (((state.data[65]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[39] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t2_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[84]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[126]==0))))) && (((state.data[106]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[197]==0))) && (((((state.data[65]==1)) || state.data[196]==0)))))) && ((((((state.data[66]==0 || state.data[181]==0))) && (((state.data[65]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[39] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t2_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[85]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[126]==0))))) && (((state.data[106]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[197]==0))) && (((((state.data[65]==1)) || state.data[196]==0)))))) && ((((((state.data[66]==0 || state.data[181]==0))) && (((state.data[65]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[39] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t2_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[86]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[126]==0))))) && (((state.data[106]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[197]==0))) && (((((state.data[65]==1)) || state.data[196]==0)))))) && ((((((state.data[66]==0 || state.data[181]==0))) && (((state.data[65]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[39] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t2_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[87]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[126]==0))))) && (((state.data[106]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[197]==0))) && (((((state.data[65]==1)) || state.data[196]==0)))))) && ((((((state.data[66]==0 || state.data[181]==0))) && (((state.data[65]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[39] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t2_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[88]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[126]==0))))) && (((state.data[106]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[197]==0))) && (((((state.data[65]==1)) || state.data[196]==0)))))) && ((((((state.data[66]==0 || state.data[181]==0))) && (((state.data[65]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[39] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t3_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[83]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[127]==0))))) && (((state.data[106]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[197]==0))) && (((((state.data[67]==1)) || state.data[196]==0)))))) && ((((((state.data[68]==0 || state.data[181]==0))) && (((state.data[67]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[39] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t3_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[84]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[127]==0))))) && (((state.data[106]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[197]==0))) && (((((state.data[67]==1)) || state.data[196]==0)))))) && ((((((state.data[68]==0 || state.data[181]==0))) && (((state.data[67]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[39] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t3_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[85]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[127]==0))))) && (((state.data[106]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[197]==0))) && (((((state.data[67]==1)) || state.data[196]==0)))))) && ((((((state.data[68]==0 || state.data[181]==0))) && (((state.data[67]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[39] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t3_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[86]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[127]==0))))) && (((state.data[106]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[197]==0))) && (((((state.data[67]==1)) || state.data[196]==0)))))) && ((((((state.data[68]==0 || state.data[181]==0))) && (((state.data[67]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[39] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t3_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[87]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[127]==0))))) && (((state.data[106]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[197]==0))) && (((((state.data[67]==1)) || state.data[196]==0)))))) && ((((((state.data[68]==0 || state.data[181]==0))) && (((state.data[67]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[39] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t3_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[88]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[127]==0))))) && (((state.data[106]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[197]==0))) && (((((state.data[67]==1)) || state.data[196]==0)))))) && ((((((state.data[68]==0 || state.data[181]==0))) && (((state.data[67]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[39] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t5_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[83]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[128]==0))))) && (((state.data[106]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[197]==0))) && (((((state.data[69]==1)) || state.data[196]==0)))))) && ((((((state.data[70]==0 || state.data[181]==0))) && (((state.data[69]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[39] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t5_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[84]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[128]==0))))) && (((state.data[106]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[197]==0))) && (((((state.data[69]==1)) || state.data[196]==0)))))) && ((((((state.data[70]==0 || state.data[181]==0))) && (((state.data[69]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[39] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t5_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[85]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[128]==0))))) && (((state.data[106]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[197]==0))) && (((((state.data[69]==1)) || state.data[196]==0)))))) && ((((((state.data[70]==0 || state.data[181]==0))) && (((state.data[69]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[39] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t5_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[86]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[128]==0))))) && (((state.data[106]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[197]==0))) && (((((state.data[69]==1)) || state.data[196]==0)))))) && ((((((state.data[70]==0 || state.data[181]==0))) && (((state.data[69]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[39] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t5_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[87]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[128]==0))))) && (((state.data[106]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[197]==0))) && (((((state.data[69]==1)) || state.data[196]==0)))))) && ((((((state.data[70]==0 || state.data[181]==0))) && (((state.data[69]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[39] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_guide_reminder_1_t5_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[88]==1 && ((state.data[39]==0)) && (((state.data[202]==0 || ((state.data[128]==0))))) && (((state.data[106]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[153]==0 || ((state.data[37]==1))))) && (((state.data[165]==0 || ((state.data[40]==1))))) && (((state.data[161]==0 || ((state.data[39]==1))))) && (((state.data[157]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[197]==0))) && (((((state.data[69]==1)) || state.data[196]==0)))))) && ((((((state.data[70]==0 || state.data[181]==0))) && (((state.data[69]==0 || state.data[180]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[39] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t4_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[89]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[124]==0))))) && (((state.data[107]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[199]==0))) && (((((state.data[61]==1)) || state.data[198]==0)))))) && ((((((state.data[62]==0 || state.data[183]==0))) && (((state.data[61]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[40] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t4_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[90]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[124]==0))))) && (((state.data[107]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[199]==0))) && (((((state.data[61]==1)) || state.data[198]==0)))))) && ((((((state.data[62]==0 || state.data[183]==0))) && (((state.data[61]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[40] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t4_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[91]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[124]==0))))) && (((state.data[107]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[199]==0))) && (((((state.data[61]==1)) || state.data[198]==0)))))) && ((((((state.data[62]==0 || state.data[183]==0))) && (((state.data[61]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[40] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t4_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[92]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[124]==0))))) && (((state.data[107]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[199]==0))) && (((((state.data[61]==1)) || state.data[198]==0)))))) && ((((((state.data[62]==0 || state.data[183]==0))) && (((state.data[61]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[40] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t4_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[93]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[124]==0))))) && (((state.data[107]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[199]==0))) && (((((state.data[61]==1)) || state.data[198]==0)))))) && ((((((state.data[62]==0 || state.data[183]==0))) && (((state.data[61]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[40] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t4_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[32]==1 && state.data[94]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[124]==0))))) && (((state.data[107]==0 || ((state.data[117]==0))))) && ((((((state.data[62]==1 && state.data[114]==1))) || (((state.data[61]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[199]==0))) && (((((state.data[61]==1)) || state.data[198]==0)))))) && ((((((state.data[62]==0 || state.data[183]==0))) && (((state.data[61]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[40] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t1_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[89]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[125]==0))))) && (((state.data[107]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[199]==0))) && (((((state.data[63]==1)) || state.data[198]==0)))))) && ((((((state.data[64]==0 || state.data[183]==0))) && (((state.data[63]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[40] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t1_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[90]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[125]==0))))) && (((state.data[107]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[199]==0))) && (((((state.data[63]==1)) || state.data[198]==0)))))) && ((((((state.data[64]==0 || state.data[183]==0))) && (((state.data[63]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[40] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t1_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[91]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[125]==0))))) && (((state.data[107]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[199]==0))) && (((((state.data[63]==1)) || state.data[198]==0)))))) && ((((((state.data[64]==0 || state.data[183]==0))) && (((state.data[63]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[40] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t1_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[92]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[125]==0))))) && (((state.data[107]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[199]==0))) && (((((state.data[63]==1)) || state.data[198]==0)))))) && ((((((state.data[64]==0 || state.data[183]==0))) && (((state.data[63]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[40] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t1_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[93]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[125]==0))))) && (((state.data[107]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[199]==0))) && (((((state.data[63]==1)) || state.data[198]==0)))))) && ((((((state.data[64]==0 || state.data[183]==0))) && (((state.data[63]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[40] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t1_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[33]==1 && state.data[94]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[125]==0))))) && (((state.data[107]==0 || ((state.data[118]==0))))) && ((((((state.data[64]==1 && state.data[114]==1))) || (((state.data[63]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[199]==0))) && (((((state.data[63]==1)) || state.data[198]==0)))))) && ((((((state.data[64]==0 || state.data[183]==0))) && (((state.data[63]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[40] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t2_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[89]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[126]==0))))) && (((state.data[107]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[199]==0))) && (((((state.data[65]==1)) || state.data[198]==0)))))) && ((((((state.data[66]==0 || state.data[183]==0))) && (((state.data[65]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[40] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t2_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[90]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[126]==0))))) && (((state.data[107]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[199]==0))) && (((((state.data[65]==1)) || state.data[198]==0)))))) && ((((((state.data[66]==0 || state.data[183]==0))) && (((state.data[65]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[40] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t2_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[91]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[126]==0))))) && (((state.data[107]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[199]==0))) && (((((state.data[65]==1)) || state.data[198]==0)))))) && ((((((state.data[66]==0 || state.data[183]==0))) && (((state.data[65]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[40] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t2_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[92]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[126]==0))))) && (((state.data[107]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[199]==0))) && (((((state.data[65]==1)) || state.data[198]==0)))))) && ((((((state.data[66]==0 || state.data[183]==0))) && (((state.data[65]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[40] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t2_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[93]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[126]==0))))) && (((state.data[107]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[199]==0))) && (((((state.data[65]==1)) || state.data[198]==0)))))) && ((((((state.data[66]==0 || state.data[183]==0))) && (((state.data[65]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[40] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t2_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[34]==1 && state.data[94]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[126]==0))))) && (((state.data[107]==0 || ((state.data[119]==0))))) && ((((((state.data[66]==1 && state.data[114]==1))) || (((state.data[65]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[199]==0))) && (((((state.data[65]==1)) || state.data[198]==0)))))) && ((((((state.data[66]==0 || state.data[183]==0))) && (((state.data[65]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[40] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t3_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[89]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[127]==0))))) && (((state.data[107]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[199]==0))) && (((((state.data[67]==1)) || state.data[198]==0)))))) && ((((((state.data[68]==0 || state.data[183]==0))) && (((state.data[67]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[40] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t3_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[90]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[127]==0))))) && (((state.data[107]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[199]==0))) && (((((state.data[67]==1)) || state.data[198]==0)))))) && ((((((state.data[68]==0 || state.data[183]==0))) && (((state.data[67]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[40] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t3_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[91]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[127]==0))))) && (((state.data[107]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[199]==0))) && (((((state.data[67]==1)) || state.data[198]==0)))))) && ((((((state.data[68]==0 || state.data[183]==0))) && (((state.data[67]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[40] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t3_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[92]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[127]==0))))) && (((state.data[107]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[199]==0))) && (((((state.data[67]==1)) || state.data[198]==0)))))) && ((((((state.data[68]==0 || state.data[183]==0))) && (((state.data[67]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[40] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t3_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[93]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[127]==0))))) && (((state.data[107]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[199]==0))) && (((((state.data[67]==1)) || state.data[198]==0)))))) && ((((((state.data[68]==0 || state.data[183]==0))) && (((state.data[67]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[40] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t3_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[35]==1 && state.data[94]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[127]==0))))) && (((state.data[107]==0 || ((state.data[120]==0))))) && ((((((state.data[68]==1 && state.data[114]==1))) || (((state.data[67]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[199]==0))) && (((((state.data[67]==1)) || state.data[198]==0)))))) && ((((((state.data[68]==0 || state.data[183]==0))) && (((state.data[67]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[40] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t5_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[89]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[128]==0))))) && (((state.data[107]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[199]==0))) && (((((state.data[69]==1)) || state.data[198]==0)))))) && ((((((state.data[70]==0 || state.data[183]==0))) && (((state.data[69]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[40] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t5_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[90]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[128]==0))))) && (((state.data[107]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[199]==0))) && (((((state.data[69]==1)) || state.data[198]==0)))))) && ((((((state.data[70]==0 || state.data[183]==0))) && (((state.data[69]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[40] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t5_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[91]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[128]==0))))) && (((state.data[107]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[199]==0))) && (((((state.data[69]==1)) || state.data[198]==0)))))) && ((((((state.data[70]==0 || state.data[183]==0))) && (((state.data[69]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[40] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t5_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[92]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[128]==0))))) && (((state.data[107]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[199]==0))) && (((((state.data[69]==1)) || state.data[198]==0)))))) && ((((((state.data[70]==0 || state.data[183]==0))) && (((state.data[69]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[40] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t5_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[93]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[128]==0))))) && (((state.data[107]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[199]==0))) && (((((state.data[69]==1)) || state.data[198]==0)))))) && ((((((state.data[70]==0 || state.data[183]==0))) && (((state.data[69]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[40] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace GiveReminder_recorded_reminder_t5_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[134]==1 && state.data[36]==1 && state.data[94]==1 && ((state.data[40]==0)) && (((state.data[203]==0 || ((state.data[128]==0))))) && (((state.data[107]==0 || ((state.data[121]==0))))) && ((((((state.data[70]==1 && state.data[114]==1))) || (((state.data[69]==1 && state.data[113]==1)))))) && ((((((state.data[154]==0 || ((state.data[37]==1))))) && (((state.data[166]==0 || ((state.data[40]==1))))) && (((state.data[162]==0 || ((state.data[39]==1))))) && (((state.data[158]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[199]==0))) && (((((state.data[69]==1)) || state.data[198]==0)))))) && ((((((state.data[70]==0 || state.data[183]==0))) && (((state.data[69]==0 || state.data[182]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[40] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MoveToLandmark_t4_couch_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && ((state.data[184]==0)) && state.data[113]==1 && state.data[172]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[113] = 0;
state.data[184] = 1;
if (state.data[41]==1){if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t4_couch_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && ((state.data[184]==0)) && state.data[113]==1 && state.data[173]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[113] = 0;
state.data[184] = 1;
if (state.data[41]==1){if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t4_kitchen_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && ((state.data[184]==0)) && state.data[114]==1 && state.data[174]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[114] = 0;
state.data[184] = 1;
if (state.data[41]==1){if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t4_kitchen_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && ((state.data[184]==0)) && state.data[114]==1 && state.data[175]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[114] = 0;
state.data[184] = 1;
if (state.data[41]==1){if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t1_couch_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && ((state.data[185]==0)) && state.data[113]==1 && state.data[172]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[113] = 0;
state.data[185] = 1;
if (state.data[41]==1){if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t1_couch_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && ((state.data[185]==0)) && state.data[113]==1 && state.data[173]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[113] = 0;
state.data[185] = 1;
if (state.data[41]==1){if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t1_kitchen_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && ((state.data[185]==0)) && state.data[114]==1 && state.data[174]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[114] = 0;
state.data[185] = 1;
if (state.data[41]==1){if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t1_kitchen_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && ((state.data[185]==0)) && state.data[114]==1 && state.data[175]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[114] = 0;
state.data[185] = 1;
if (state.data[41]==1){if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t2_couch_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && ((state.data[186]==0)) && state.data[113]==1 && state.data[172]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[113] = 0;
state.data[186] = 1;
if (state.data[41]==1){if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t2_couch_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && ((state.data[186]==0)) && state.data[113]==1 && state.data[173]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[113] = 0;
state.data[186] = 1;
if (state.data[41]==1){if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t2_kitchen_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && ((state.data[186]==0)) && state.data[114]==1 && state.data[174]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[114] = 0;
state.data[186] = 1;
if (state.data[41]==1){if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t2_kitchen_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && ((state.data[186]==0)) && state.data[114]==1 && state.data[175]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[114] = 0;
state.data[186] = 1;
if (state.data[41]==1){if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t3_couch_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && ((state.data[187]==0)) && state.data[113]==1 && state.data[172]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[113] = 0;
state.data[187] = 1;
if (state.data[41]==1){if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t3_couch_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && ((state.data[187]==0)) && state.data[113]==1 && state.data[173]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[113] = 0;
state.data[187] = 1;
if (state.data[41]==1){if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t3_kitchen_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && ((state.data[187]==0)) && state.data[114]==1 && state.data[174]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[114] = 0;
state.data[187] = 1;
if (state.data[41]==1){if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t3_kitchen_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && ((state.data[187]==0)) && state.data[114]==1 && state.data[175]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[114] = 0;
state.data[187] = 1;
if (state.data[41]==1){if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t5_couch_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && ((state.data[188]==0)) && state.data[113]==1 && state.data[172]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[113] = 0;
state.data[188] = 1;
if (state.data[41]==1){if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t5_couch_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && ((state.data[188]==0)) && state.data[113]==1 && state.data[173]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[113] = 0;
state.data[188] = 1;
if (state.data[41]==1){if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t5_kitchen_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && ((state.data[188]==0)) && state.data[114]==1 && state.data[174]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[113] = 1;
state.data[114] = 0;
state.data[188] = 1;
if (state.data[41]==1){if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}}
    }
}

namespace MoveToLandmark_t5_kitchen_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && ((state.data[188]==0)) && state.data[114]==1 && state.data[175]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[114] = 1;
state.data[114] = 0;
state.data[188] = 1;
if (state.data[41]==1){if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}}
    }
}

namespace MakeCall_caregiver_call_t4_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[46]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[124]==0))))) && (((state.data[26]==0 || ((state.data[117]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[29]==0))) && (((((state.data[61]==1)) || state.data[28]==0)))))) && ((((((state.data[62]==0 || state.data[168]==0))) && (((state.data[61]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[59] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t4_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[47]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[124]==0))))) && (((state.data[26]==0 || ((state.data[117]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[29]==0))) && (((((state.data[61]==1)) || state.data[28]==0)))))) && ((((((state.data[62]==0 || state.data[168]==0))) && (((state.data[61]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[59] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t4_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[48]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[124]==0))))) && (((state.data[26]==0 || ((state.data[117]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[29]==0))) && (((((state.data[61]==1)) || state.data[28]==0)))))) && ((((((state.data[62]==0 || state.data[168]==0))) && (((state.data[61]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[59] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t4_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[49]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[124]==0))))) && (((state.data[26]==0 || ((state.data[117]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[29]==0))) && (((((state.data[61]==1)) || state.data[28]==0)))))) && ((((((state.data[62]==0 || state.data[168]==0))) && (((state.data[61]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[59] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t4_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[50]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[124]==0))))) && (((state.data[26]==0 || ((state.data[117]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[29]==0))) && (((((state.data[61]==1)) || state.data[28]==0)))))) && ((((((state.data[62]==0 || state.data[168]==0))) && (((state.data[61]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[59] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t4_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[51]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[124]==0))))) && (((state.data[26]==0 || ((state.data[117]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[29]==0))) && (((((state.data[61]==1)) || state.data[28]==0)))))) && ((((((state.data[62]==0 || state.data[168]==0))) && (((state.data[61]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[59] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t1_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[46]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[125]==0))))) && (((state.data[26]==0 || ((state.data[118]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[29]==0))) && (((((state.data[63]==1)) || state.data[28]==0)))))) && ((((((state.data[64]==0 || state.data[168]==0))) && (((state.data[63]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[59] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t1_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[47]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[125]==0))))) && (((state.data[26]==0 || ((state.data[118]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[29]==0))) && (((((state.data[63]==1)) || state.data[28]==0)))))) && ((((((state.data[64]==0 || state.data[168]==0))) && (((state.data[63]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[59] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t1_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[48]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[125]==0))))) && (((state.data[26]==0 || ((state.data[118]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[29]==0))) && (((((state.data[63]==1)) || state.data[28]==0)))))) && ((((((state.data[64]==0 || state.data[168]==0))) && (((state.data[63]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[59] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t1_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[49]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[125]==0))))) && (((state.data[26]==0 || ((state.data[118]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[29]==0))) && (((((state.data[63]==1)) || state.data[28]==0)))))) && ((((((state.data[64]==0 || state.data[168]==0))) && (((state.data[63]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[59] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t1_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[50]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[125]==0))))) && (((state.data[26]==0 || ((state.data[118]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[29]==0))) && (((((state.data[63]==1)) || state.data[28]==0)))))) && ((((((state.data[64]==0 || state.data[168]==0))) && (((state.data[63]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[59] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t1_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[51]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[125]==0))))) && (((state.data[26]==0 || ((state.data[118]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[29]==0))) && (((((state.data[63]==1)) || state.data[28]==0)))))) && ((((((state.data[64]==0 || state.data[168]==0))) && (((state.data[63]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[59] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t2_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[46]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[126]==0))))) && (((state.data[26]==0 || ((state.data[119]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[29]==0))) && (((((state.data[65]==1)) || state.data[28]==0)))))) && ((((((state.data[66]==0 || state.data[168]==0))) && (((state.data[65]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[59] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t2_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[47]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[126]==0))))) && (((state.data[26]==0 || ((state.data[119]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[29]==0))) && (((((state.data[65]==1)) || state.data[28]==0)))))) && ((((((state.data[66]==0 || state.data[168]==0))) && (((state.data[65]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[59] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t2_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[48]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[126]==0))))) && (((state.data[26]==0 || ((state.data[119]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[29]==0))) && (((((state.data[65]==1)) || state.data[28]==0)))))) && ((((((state.data[66]==0 || state.data[168]==0))) && (((state.data[65]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[59] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t2_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[49]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[126]==0))))) && (((state.data[26]==0 || ((state.data[119]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[29]==0))) && (((((state.data[65]==1)) || state.data[28]==0)))))) && ((((((state.data[66]==0 || state.data[168]==0))) && (((state.data[65]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[59] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t2_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[50]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[126]==0))))) && (((state.data[26]==0 || ((state.data[119]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[29]==0))) && (((((state.data[65]==1)) || state.data[28]==0)))))) && ((((((state.data[66]==0 || state.data[168]==0))) && (((state.data[65]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[59] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t2_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[51]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[126]==0))))) && (((state.data[26]==0 || ((state.data[119]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[29]==0))) && (((((state.data[65]==1)) || state.data[28]==0)))))) && ((((((state.data[66]==0 || state.data[168]==0))) && (((state.data[65]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[59] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t3_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[46]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[127]==0))))) && (((state.data[26]==0 || ((state.data[120]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[29]==0))) && (((((state.data[67]==1)) || state.data[28]==0)))))) && ((((((state.data[68]==0 || state.data[168]==0))) && (((state.data[67]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[59] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t3_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[47]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[127]==0))))) && (((state.data[26]==0 || ((state.data[120]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[29]==0))) && (((((state.data[67]==1)) || state.data[28]==0)))))) && ((((((state.data[68]==0 || state.data[168]==0))) && (((state.data[67]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[59] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t3_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[48]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[127]==0))))) && (((state.data[26]==0 || ((state.data[120]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[29]==0))) && (((((state.data[67]==1)) || state.data[28]==0)))))) && ((((((state.data[68]==0 || state.data[168]==0))) && (((state.data[67]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[59] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t3_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[49]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[127]==0))))) && (((state.data[26]==0 || ((state.data[120]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[29]==0))) && (((((state.data[67]==1)) || state.data[28]==0)))))) && ((((((state.data[68]==0 || state.data[168]==0))) && (((state.data[67]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[59] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t3_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[50]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[127]==0))))) && (((state.data[26]==0 || ((state.data[120]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[29]==0))) && (((((state.data[67]==1)) || state.data[28]==0)))))) && ((((((state.data[68]==0 || state.data[168]==0))) && (((state.data[67]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[59] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t3_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[51]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[127]==0))))) && (((state.data[26]==0 || ((state.data[120]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[29]==0))) && (((((state.data[67]==1)) || state.data[28]==0)))))) && ((((((state.data[68]==0 || state.data[168]==0))) && (((state.data[67]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[59] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t5_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[46]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[128]==0))))) && (((state.data[26]==0 || ((state.data[121]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[29]==0))) && (((((state.data[69]==1)) || state.data[28]==0)))))) && ((((((state.data[70]==0 || state.data[168]==0))) && (((state.data[69]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[59] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t5_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[47]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[128]==0))))) && (((state.data[26]==0 || ((state.data[121]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[29]==0))) && (((((state.data[69]==1)) || state.data[28]==0)))))) && ((((((state.data[70]==0 || state.data[168]==0))) && (((state.data[69]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[59] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t5_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[48]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[128]==0))))) && (((state.data[26]==0 || ((state.data[121]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[29]==0))) && (((((state.data[69]==1)) || state.data[28]==0)))))) && ((((((state.data[70]==0 || state.data[168]==0))) && (((state.data[69]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[59] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t5_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[49]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[128]==0))))) && (((state.data[26]==0 || ((state.data[121]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[29]==0))) && (((((state.data[69]==1)) || state.data[28]==0)))))) && ((((((state.data[70]==0 || state.data[168]==0))) && (((state.data[69]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[59] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t5_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[50]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[128]==0))))) && (((state.data[26]==0 || ((state.data[121]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[29]==0))) && (((((state.data[69]==1)) || state.data[28]==0)))))) && ((((((state.data[70]==0 || state.data[168]==0))) && (((state.data[69]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[59] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_t5_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[51]==1 && ((state.data[59]==0)) && (((state.data[189]==0 || ((state.data[128]==0))))) && (((state.data[26]==0 || ((state.data[121]==0))))) && ((((((state.data[44]==0 || ((state.data[60]==1))))) && (((state.data[42]==0 || ((state.data[59]==1)))))))) && ((((((state.data[135]==0 || ((state.data[37]==1))))) && (((state.data[141]==0 || ((state.data[40]==1))))) && (((state.data[139]==0 || ((state.data[39]==1))))) && (((state.data[137]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[29]==0))) && (((((state.data[69]==1)) || state.data[28]==0)))))) && ((((((state.data[70]==0 || state.data[168]==0))) && (((state.data[69]==0 || state.data[167]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[59] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t4_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[52]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[124]==0))))) && (((state.data[27]==0 || ((state.data[117]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[31]==0))) && (((((state.data[61]==1)) || state.data[30]==0)))))) && ((((((state.data[62]==0 || state.data[170]==0))) && (((state.data[61]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[60] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t4_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[53]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[124]==0))))) && (((state.data[27]==0 || ((state.data[117]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[31]==0))) && (((((state.data[61]==1)) || state.data[30]==0)))))) && ((((((state.data[62]==0 || state.data[170]==0))) && (((state.data[61]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[60] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t4_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[54]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[124]==0))))) && (((state.data[27]==0 || ((state.data[117]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[31]==0))) && (((((state.data[61]==1)) || state.data[30]==0)))))) && ((((((state.data[62]==0 || state.data[170]==0))) && (((state.data[61]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[60] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t4_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[55]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[124]==0))))) && (((state.data[27]==0 || ((state.data[117]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[31]==0))) && (((((state.data[61]==1)) || state.data[30]==0)))))) && ((((((state.data[62]==0 || state.data[170]==0))) && (((state.data[61]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[60] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t4_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[56]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[124]==0))))) && (((state.data[27]==0 || ((state.data[117]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[31]==0))) && (((((state.data[61]==1)) || state.data[30]==0)))))) && ((((((state.data[62]==0 || state.data[170]==0))) && (((state.data[61]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[60] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t4_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[32]==1 && state.data[57]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[124]==0))))) && (((state.data[27]==0 || ((state.data[117]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[62]==1)) || state.data[31]==0))) && (((((state.data[61]==1)) || state.data[30]==0)))))) && ((((((state.data[62]==0 || state.data[170]==0))) && (((state.data[61]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[60] = 1;
if (state.data[0]==1){state.data[32] = 0;
state.data[32] = 1;
}if (state.data[1]==1){state.data[32] = 0;
state.data[33] = 1;
}if (state.data[2]==1){state.data[32] = 0;
state.data[34] = 1;
}if (state.data[4]==1){state.data[32] = 0;
state.data[36] = 1;
}if (state.data[3]==1){state.data[32] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t1_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[52]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[125]==0))))) && (((state.data[27]==0 || ((state.data[118]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[31]==0))) && (((((state.data[63]==1)) || state.data[30]==0)))))) && ((((((state.data[64]==0 || state.data[170]==0))) && (((state.data[63]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[60] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t1_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[53]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[125]==0))))) && (((state.data[27]==0 || ((state.data[118]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[31]==0))) && (((((state.data[63]==1)) || state.data[30]==0)))))) && ((((((state.data[64]==0 || state.data[170]==0))) && (((state.data[63]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[60] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t1_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[54]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[125]==0))))) && (((state.data[27]==0 || ((state.data[118]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[31]==0))) && (((((state.data[63]==1)) || state.data[30]==0)))))) && ((((((state.data[64]==0 || state.data[170]==0))) && (((state.data[63]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[60] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t1_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[55]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[125]==0))))) && (((state.data[27]==0 || ((state.data[118]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[31]==0))) && (((((state.data[63]==1)) || state.data[30]==0)))))) && ((((((state.data[64]==0 || state.data[170]==0))) && (((state.data[63]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[60] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t1_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[56]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[125]==0))))) && (((state.data[27]==0 || ((state.data[118]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[31]==0))) && (((((state.data[63]==1)) || state.data[30]==0)))))) && ((((((state.data[64]==0 || state.data[170]==0))) && (((state.data[63]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[60] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t1_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[33]==1 && state.data[57]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[125]==0))))) && (((state.data[27]==0 || ((state.data[118]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[64]==1)) || state.data[31]==0))) && (((((state.data[63]==1)) || state.data[30]==0)))))) && ((((((state.data[64]==0 || state.data[170]==0))) && (((state.data[63]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[60] = 1;
if (state.data[5]==1){state.data[33] = 0;
state.data[32] = 1;
}if (state.data[6]==1){state.data[33] = 0;
state.data[33] = 1;
}if (state.data[7]==1){state.data[33] = 0;
state.data[34] = 1;
}if (state.data[9]==1){state.data[33] = 0;
state.data[36] = 1;
}if (state.data[8]==1){state.data[33] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t2_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[52]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[126]==0))))) && (((state.data[27]==0 || ((state.data[119]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[31]==0))) && (((((state.data[65]==1)) || state.data[30]==0)))))) && ((((((state.data[66]==0 || state.data[170]==0))) && (((state.data[65]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[60] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t2_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[53]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[126]==0))))) && (((state.data[27]==0 || ((state.data[119]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[31]==0))) && (((((state.data[65]==1)) || state.data[30]==0)))))) && ((((((state.data[66]==0 || state.data[170]==0))) && (((state.data[65]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[60] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t2_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[54]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[126]==0))))) && (((state.data[27]==0 || ((state.data[119]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[31]==0))) && (((((state.data[65]==1)) || state.data[30]==0)))))) && ((((((state.data[66]==0 || state.data[170]==0))) && (((state.data[65]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[60] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t2_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[55]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[126]==0))))) && (((state.data[27]==0 || ((state.data[119]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[31]==0))) && (((((state.data[65]==1)) || state.data[30]==0)))))) && ((((((state.data[66]==0 || state.data[170]==0))) && (((state.data[65]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[60] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t2_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[56]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[126]==0))))) && (((state.data[27]==0 || ((state.data[119]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[31]==0))) && (((((state.data[65]==1)) || state.data[30]==0)))))) && ((((((state.data[66]==0 || state.data[170]==0))) && (((state.data[65]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[60] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t2_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[34]==1 && state.data[57]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[126]==0))))) && (((state.data[27]==0 || ((state.data[119]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[66]==1)) || state.data[31]==0))) && (((((state.data[65]==1)) || state.data[30]==0)))))) && ((((((state.data[66]==0 || state.data[170]==0))) && (((state.data[65]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[60] = 1;
if (state.data[10]==1){state.data[34] = 0;
state.data[32] = 1;
}if (state.data[11]==1){state.data[34] = 0;
state.data[33] = 1;
}if (state.data[12]==1){state.data[34] = 0;
state.data[34] = 1;
}if (state.data[14]==1){state.data[34] = 0;
state.data[36] = 1;
}if (state.data[13]==1){state.data[34] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t3_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[52]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[127]==0))))) && (((state.data[27]==0 || ((state.data[120]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[31]==0))) && (((((state.data[67]==1)) || state.data[30]==0)))))) && ((((((state.data[68]==0 || state.data[170]==0))) && (((state.data[67]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[60] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t3_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[53]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[127]==0))))) && (((state.data[27]==0 || ((state.data[120]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[31]==0))) && (((((state.data[67]==1)) || state.data[30]==0)))))) && ((((((state.data[68]==0 || state.data[170]==0))) && (((state.data[67]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[60] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t3_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[54]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[127]==0))))) && (((state.data[27]==0 || ((state.data[120]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[31]==0))) && (((((state.data[67]==1)) || state.data[30]==0)))))) && ((((((state.data[68]==0 || state.data[170]==0))) && (((state.data[67]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[60] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t3_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[55]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[127]==0))))) && (((state.data[27]==0 || ((state.data[120]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[31]==0))) && (((((state.data[67]==1)) || state.data[30]==0)))))) && ((((((state.data[68]==0 || state.data[170]==0))) && (((state.data[67]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[60] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t3_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[56]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[127]==0))))) && (((state.data[27]==0 || ((state.data[120]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[31]==0))) && (((((state.data[67]==1)) || state.data[30]==0)))))) && ((((((state.data[68]==0 || state.data[170]==0))) && (((state.data[67]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[60] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t3_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[35]==1 && state.data[57]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[127]==0))))) && (((state.data[27]==0 || ((state.data[120]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[68]==1)) || state.data[31]==0))) && (((((state.data[67]==1)) || state.data[30]==0)))))) && ((((((state.data[68]==0 || state.data[170]==0))) && (((state.data[67]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[60] = 1;
if (state.data[15]==1){state.data[35] = 0;
state.data[32] = 1;
}if (state.data[16]==1){state.data[35] = 0;
state.data[33] = 1;
}if (state.data[17]==1){state.data[35] = 0;
state.data[34] = 1;
}if (state.data[19]==1){state.data[35] = 0;
state.data[36] = 1;
}if (state.data[18]==1){state.data[35] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t5_nathan_guide_2_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[52]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[128]==0))))) && (((state.data[27]==0 || ((state.data[121]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[31]==0))) && (((((state.data[69]==1)) || state.data[30]==0)))))) && ((((((state.data[70]==0 || state.data[170]==0))) && (((state.data[69]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[95] = 1;
state.data[60] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t5_nathan_guide_1_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[53]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[128]==0))))) && (((state.data[27]==0 || ((state.data[121]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[31]==0))) && (((((state.data[69]==1)) || state.data[30]==0)))))) && ((((((state.data[70]==0 || state.data[170]==0))) && (((state.data[69]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[96] = 1;
state.data[60] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t5_nathan_automated_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[54]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[128]==0))))) && (((state.data[27]==0 || ((state.data[121]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[31]==0))) && (((((state.data[69]==1)) || state.data[30]==0)))))) && ((((((state.data[70]==0 || state.data[170]==0))) && (((state.data[69]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[97] = 1;
state.data[60] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t5_nathan_recorded_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[55]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[128]==0))))) && (((state.data[27]==0 || ((state.data[121]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[31]==0))) && (((((state.data[69]==1)) || state.data[30]==0)))))) && ((((((state.data[70]==0 || state.data[170]==0))) && (((state.data[69]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[98] = 1;
state.data[60] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t5_nathan_call_caregiver_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[56]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[128]==0))))) && (((state.data[27]==0 || ((state.data[121]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[31]==0))) && (((((state.data[69]==1)) || state.data[30]==0)))))) && ((((((state.data[70]==0 || state.data[170]==0))) && (((state.data[69]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[99] = 1;
state.data[60] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MakeCall_caregiver_call_guide_t5_nathan_call_caregiver_guide_msg {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[144]==1 && state.data[36]==1 && state.data[57]==1 && ((state.data[60]==0)) && (((state.data[190]==0 || ((state.data[128]==0))))) && (((state.data[27]==0 || ((state.data[121]==0))))) && ((((((state.data[45]==0 || ((state.data[60]==1))))) && (((state.data[43]==0 || ((state.data[59]==1)))))))) && ((((((state.data[136]==0 || ((state.data[37]==1))))) && (((state.data[142]==0 || ((state.data[40]==1))))) && (((state.data[140]==0 || ((state.data[39]==1))))) && (((state.data[138]==0 || ((state.data[38]==1)))))))) && ((((((((state.data[70]==1)) || state.data[31]==0))) && (((((state.data[69]==1)) || state.data[30]==0)))))) && ((((((state.data[70]==0 || state.data[170]==0))) && (((state.data[69]==0 || state.data[169]==0))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[100] = 1;
state.data[60] = 1;
if (state.data[20]==1){state.data[36] = 0;
state.data[32] = 1;
}if (state.data[21]==1){state.data[36] = 0;
state.data[33] = 1;
}if (state.data[22]==1){state.data[36] = 0;
state.data[34] = 1;
}if (state.data[24]==1){state.data[36] = 0;
state.data[36] = 1;
}if (state.data[23]==1){state.data[36] = 0;
state.data[35] = 1;
}
    }
}

namespace MessageGivenSuccess_ {
    inline bool check_preconditions(const KBState & state) {
        return ((((((((state.data[146]==1 && state.data[96]==1))) || (((state.data[148]==1 && state.data[98]==1))) || (((state.data[149]==1 && state.data[99]==1))) || (((state.data[147]==1 && state.data[97]==1))) || (((state.data[145]==1 && state.data[95]==1))) || (((state.data[150]==1 && state.data[100]==1))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace MedicineTakenSuccess_ {
    inline bool check_preconditions(const KBState & state) {
        return ((((((((state.data[103]==1 && state.data[124]==1))) || (((state.data[103]==1 && state.data[125]==1))) || (((state.data[103]==1 && state.data[126]==1))) || (((state.data[103]==1 && state.data[128]==1))) || (((state.data[103]==1 && state.data[127]==1))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace FoodEatenSuccess_ {
    inline bool check_preconditions(const KBState & state) {
        return ((((((((state.data[143]==1 && state.data[117]==1))) || (((state.data[143]==1 && state.data[118]==1))) || (((state.data[143]==1 && state.data[119]==1))) || (((state.data[143]==1 && state.data[121]==1))) || (((state.data[143]==1 && state.data[120]==1))))))));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t4_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && state.data[61]==1 && state.data[122]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t4_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && state.data[62]==1 && state.data[123]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t1_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && state.data[63]==1 && state.data[122]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t1_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && state.data[64]==1 && state.data[123]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t2_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && state.data[65]==1 && state.data[122]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t2_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && state.data[66]==1 && state.data[123]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t3_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && state.data[67]==1 && state.data[122]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t3_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && state.data[68]==1 && state.data[123]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t5_couch {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && state.data[69]==1 && state.data[122]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}

namespace PersonAtSuccess_nathan_t5_kitchen {
    inline bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && state.data[70]==1 && state.data[123]==1));
    }
    inline void apply_effect(KBState & state) {
        state.data[171] = 1;

    }
}


void apply_observe_debug_1(){
    int o = 0;
}
void apply_observe_debug_2(){
    int o = 0;
}

namespace DetectTakingMedicine_t4 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[58]==1 && state.data[32]==1)) && (state.data[124]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[124] = 1;
state2.data[124] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectTakingMedicine_t1 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[58]==1 && state.data[33]==1)) && (state.data[125]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[125] = 1;
state2.data[125] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectTakingMedicine_t2 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[58]==1 && state.data[34]==1)) && (state.data[126]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[126] = 1;
state2.data[126] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectTakingMedicine_t3 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[58]==1 && state.data[35]==1)) && (state.data[127]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[127] = 1;
state2.data[127] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectTakingMedicine_t5 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[58]==1 && state.data[36]==1)) && (state.data[128]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[128] = 1;
state2.data[128] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectEatingFood_t4 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[25]==1 && state.data[32]==1)) && (state.data[117]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[117] = 1;
state2.data[117] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectEatingFood_t1 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[25]==1 && state.data[33]==1)) && (state.data[118]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[118] = 1;
state2.data[118] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectEatingFood_t2 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[25]==1 && state.data[34]==1)) && (state.data[119]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[119] = 1;
state2.data[119] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectEatingFood_t3 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[25]==1 && state.data[35]==1)) && (state.data[120]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[120] = 1;
state2.data[120] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectEatingFood_t5 {
    bool check_preconditions(const KBState & state) {
        return ((state.data[25]==1 && state.data[36]==1)) && (state.data[121]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[121] = 1;
state2.data[121] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t4_nathan_couch {
    bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && state.data[191]==1)) && (state.data[61]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[61] = 1;
state2.data[61] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t4_nathan_kitchen {
    bool check_preconditions(const KBState & state) {
        return ((state.data[32]==1 && state.data[191]==1)) && (state.data[62]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[62] = 1;
state2.data[62] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t1_nathan_couch {
    bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && state.data[191]==1)) && (state.data[63]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[63] = 1;
state2.data[63] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t1_nathan_kitchen {
    bool check_preconditions(const KBState & state) {
        return ((state.data[33]==1 && state.data[191]==1)) && (state.data[64]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[64] = 1;
state2.data[64] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t2_nathan_couch {
    bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && state.data[191]==1)) && (state.data[65]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[65] = 1;
state2.data[65] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t2_nathan_kitchen {
    bool check_preconditions(const KBState & state) {
        return ((state.data[34]==1 && state.data[191]==1)) && (state.data[66]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[66] = 1;
state2.data[66] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t3_nathan_couch {
    bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && state.data[191]==1)) && (state.data[67]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[67] = 1;
state2.data[67] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t3_nathan_kitchen {
    bool check_preconditions(const KBState & state) {
        return ((state.data[35]==1 && state.data[191]==1)) && (state.data[68]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[68] = 1;
state2.data[68] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t5_nathan_couch {
    bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && state.data[191]==1)) && (state.data[69]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[69] = 1;
state2.data[69] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}

namespace DetectPersonLocation_t5_nathan_kitchen {
    bool check_preconditions(const KBState & state) {
        return ((state.data[36]==1 && state.data[191]==1)) && (state.data[70]==2);
    }
    void apply_effect(KBState & state) {
        
    }
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        state1.data[70] = 1;
state2.data[70] = 0;

        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}


void expand(const KBState& cur_state, std::array<KBState, 253> & new_states,
            const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints={}){
    int num = 0;
    
    new_states[0].valid = 0;
    new_states[1].valid = 0;
    if (DetectTakingMedicine_t4::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 213;
        new_states[num+1].action = 213;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectTakingMedicine_t4::apply_effect(new_states[num]);
        DetectTakingMedicine_t4::apply_effect(new_states[num+1]);
        DetectTakingMedicine_t4::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[2].valid = 0;
    new_states[3].valid = 0;
    if (DetectTakingMedicine_t1::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 214;
        new_states[num+1].action = 214;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectTakingMedicine_t1::apply_effect(new_states[num]);
        DetectTakingMedicine_t1::apply_effect(new_states[num+1]);
        DetectTakingMedicine_t1::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[4].valid = 0;
    new_states[5].valid = 0;
    if (DetectTakingMedicine_t2::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 215;
        new_states[num+1].action = 215;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectTakingMedicine_t2::apply_effect(new_states[num]);
        DetectTakingMedicine_t2::apply_effect(new_states[num+1]);
        DetectTakingMedicine_t2::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[6].valid = 0;
    new_states[7].valid = 0;
    if (DetectTakingMedicine_t3::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 216;
        new_states[num+1].action = 216;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectTakingMedicine_t3::apply_effect(new_states[num]);
        DetectTakingMedicine_t3::apply_effect(new_states[num+1]);
        DetectTakingMedicine_t3::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[8].valid = 0;
    new_states[9].valid = 0;
    if (DetectTakingMedicine_t5::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 217;
        new_states[num+1].action = 217;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectTakingMedicine_t5::apply_effect(new_states[num]);
        DetectTakingMedicine_t5::apply_effect(new_states[num+1]);
        DetectTakingMedicine_t5::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[10].valid = 0;
    new_states[11].valid = 0;
    if (DetectEatingFood_t4::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 218;
        new_states[num+1].action = 218;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectEatingFood_t4::apply_effect(new_states[num]);
        DetectEatingFood_t4::apply_effect(new_states[num+1]);
        DetectEatingFood_t4::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[12].valid = 0;
    new_states[13].valid = 0;
    if (DetectEatingFood_t1::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 219;
        new_states[num+1].action = 219;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectEatingFood_t1::apply_effect(new_states[num]);
        DetectEatingFood_t1::apply_effect(new_states[num+1]);
        DetectEatingFood_t1::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[14].valid = 0;
    new_states[15].valid = 0;
    if (DetectEatingFood_t2::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 220;
        new_states[num+1].action = 220;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectEatingFood_t2::apply_effect(new_states[num]);
        DetectEatingFood_t2::apply_effect(new_states[num+1]);
        DetectEatingFood_t2::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[16].valid = 0;
    new_states[17].valid = 0;
    if (DetectEatingFood_t3::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 221;
        new_states[num+1].action = 221;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectEatingFood_t3::apply_effect(new_states[num]);
        DetectEatingFood_t3::apply_effect(new_states[num+1]);
        DetectEatingFood_t3::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[18].valid = 0;
    new_states[19].valid = 0;
    if (DetectEatingFood_t5::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 222;
        new_states[num+1].action = 222;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectEatingFood_t5::apply_effect(new_states[num]);
        DetectEatingFood_t5::apply_effect(new_states[num+1]);
        DetectEatingFood_t5::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[20].valid = 0;
    new_states[21].valid = 0;
    if (DetectPersonLocation_t4_nathan_couch::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 223;
        new_states[num+1].action = 223;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t4_nathan_couch::apply_effect(new_states[num]);
        DetectPersonLocation_t4_nathan_couch::apply_effect(new_states[num+1]);
        DetectPersonLocation_t4_nathan_couch::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[22].valid = 0;
    new_states[23].valid = 0;
    if (DetectPersonLocation_t4_nathan_kitchen::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 224;
        new_states[num+1].action = 224;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t4_nathan_kitchen::apply_effect(new_states[num]);
        DetectPersonLocation_t4_nathan_kitchen::apply_effect(new_states[num+1]);
        DetectPersonLocation_t4_nathan_kitchen::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[24].valid = 0;
    new_states[25].valid = 0;
    if (DetectPersonLocation_t1_nathan_couch::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 225;
        new_states[num+1].action = 225;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t1_nathan_couch::apply_effect(new_states[num]);
        DetectPersonLocation_t1_nathan_couch::apply_effect(new_states[num+1]);
        DetectPersonLocation_t1_nathan_couch::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[26].valid = 0;
    new_states[27].valid = 0;
    if (DetectPersonLocation_t1_nathan_kitchen::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 226;
        new_states[num+1].action = 226;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t1_nathan_kitchen::apply_effect(new_states[num]);
        DetectPersonLocation_t1_nathan_kitchen::apply_effect(new_states[num+1]);
        DetectPersonLocation_t1_nathan_kitchen::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[28].valid = 0;
    new_states[29].valid = 0;
    if (DetectPersonLocation_t2_nathan_couch::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 227;
        new_states[num+1].action = 227;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t2_nathan_couch::apply_effect(new_states[num]);
        DetectPersonLocation_t2_nathan_couch::apply_effect(new_states[num+1]);
        DetectPersonLocation_t2_nathan_couch::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[30].valid = 0;
    new_states[31].valid = 0;
    if (DetectPersonLocation_t2_nathan_kitchen::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 228;
        new_states[num+1].action = 228;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t2_nathan_kitchen::apply_effect(new_states[num]);
        DetectPersonLocation_t2_nathan_kitchen::apply_effect(new_states[num+1]);
        DetectPersonLocation_t2_nathan_kitchen::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[32].valid = 0;
    new_states[33].valid = 0;
    if (DetectPersonLocation_t3_nathan_couch::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 229;
        new_states[num+1].action = 229;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t3_nathan_couch::apply_effect(new_states[num]);
        DetectPersonLocation_t3_nathan_couch::apply_effect(new_states[num+1]);
        DetectPersonLocation_t3_nathan_couch::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[34].valid = 0;
    new_states[35].valid = 0;
    if (DetectPersonLocation_t3_nathan_kitchen::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 230;
        new_states[num+1].action = 230;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t3_nathan_kitchen::apply_effect(new_states[num]);
        DetectPersonLocation_t3_nathan_kitchen::apply_effect(new_states[num+1]);
        DetectPersonLocation_t3_nathan_kitchen::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[36].valid = 0;
    new_states[37].valid = 0;
    if (DetectPersonLocation_t5_nathan_couch::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 231;
        new_states[num+1].action = 231;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t5_nathan_couch::apply_effect(new_states[num]);
        DetectPersonLocation_t5_nathan_couch::apply_effect(new_states[num+1]);
        DetectPersonLocation_t5_nathan_couch::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    
    new_states[38].valid = 0;
    new_states[39].valid = 0;
    if (DetectPersonLocation_t5_nathan_kitchen::check_preconditions(cur_state)){
        new_states[num] = cur_state;
        new_states[num+1] = cur_state;
        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = 232;
        new_states[num+1].action = 232;
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        DetectPersonLocation_t5_nathan_kitchen::apply_effect(new_states[num]);
        DetectPersonLocation_t5_nathan_kitchen::apply_effect(new_states[num+1]);
        DetectPersonLocation_t5_nathan_kitchen::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    

    
    new_states[40].valid = 0;
    if (GiveReminder_automated_reminder_t4_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 0;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t4_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[41].valid = 0;
    if (GiveReminder_automated_reminder_t4_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 1;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t4_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[42].valid = 0;
    if (GiveReminder_automated_reminder_t4_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 2;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t4_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[43].valid = 0;
    if (GiveReminder_automated_reminder_t4_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 3;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t4_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[44].valid = 0;
    if (GiveReminder_automated_reminder_t4_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 4;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t4_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[45].valid = 0;
    if (GiveReminder_automated_reminder_t4_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 5;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t4_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[46].valid = 0;
    if (GiveReminder_automated_reminder_t1_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 6;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t1_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[47].valid = 0;
    if (GiveReminder_automated_reminder_t1_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 7;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t1_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[48].valid = 0;
    if (GiveReminder_automated_reminder_t1_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 8;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t1_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[49].valid = 0;
    if (GiveReminder_automated_reminder_t1_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 9;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t1_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[50].valid = 0;
    if (GiveReminder_automated_reminder_t1_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 10;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t1_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[51].valid = 0;
    if (GiveReminder_automated_reminder_t1_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 11;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t1_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[52].valid = 0;
    if (GiveReminder_automated_reminder_t2_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 12;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t2_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[53].valid = 0;
    if (GiveReminder_automated_reminder_t2_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 13;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t2_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[54].valid = 0;
    if (GiveReminder_automated_reminder_t2_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 14;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t2_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[55].valid = 0;
    if (GiveReminder_automated_reminder_t2_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 15;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t2_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[56].valid = 0;
    if (GiveReminder_automated_reminder_t2_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 16;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t2_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[57].valid = 0;
    if (GiveReminder_automated_reminder_t2_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 17;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t2_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[58].valid = 0;
    if (GiveReminder_automated_reminder_t3_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 18;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t3_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[59].valid = 0;
    if (GiveReminder_automated_reminder_t3_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 19;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t3_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[60].valid = 0;
    if (GiveReminder_automated_reminder_t3_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 20;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t3_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[61].valid = 0;
    if (GiveReminder_automated_reminder_t3_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 21;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t3_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[62].valid = 0;
    if (GiveReminder_automated_reminder_t3_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 22;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t3_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[63].valid = 0;
    if (GiveReminder_automated_reminder_t3_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 23;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t3_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[64].valid = 0;
    if (GiveReminder_automated_reminder_t5_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 24;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t5_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[65].valid = 0;
    if (GiveReminder_automated_reminder_t5_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 25;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t5_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[66].valid = 0;
    if (GiveReminder_automated_reminder_t5_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 26;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t5_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[67].valid = 0;
    if (GiveReminder_automated_reminder_t5_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 27;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t5_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[68].valid = 0;
    if (GiveReminder_automated_reminder_t5_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 28;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t5_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[69].valid = 0;
    if (GiveReminder_automated_reminder_t5_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 29;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_automated_reminder_t5_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[70].valid = 0;
    if (GiveReminder_guide_reminder_2_t4_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 30;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t4_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[71].valid = 0;
    if (GiveReminder_guide_reminder_2_t4_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 31;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t4_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[72].valid = 0;
    if (GiveReminder_guide_reminder_2_t4_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 32;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t4_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[73].valid = 0;
    if (GiveReminder_guide_reminder_2_t4_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 33;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t4_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[74].valid = 0;
    if (GiveReminder_guide_reminder_2_t4_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 34;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t4_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[75].valid = 0;
    if (GiveReminder_guide_reminder_2_t4_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 35;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t4_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[76].valid = 0;
    if (GiveReminder_guide_reminder_2_t1_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 36;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t1_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[77].valid = 0;
    if (GiveReminder_guide_reminder_2_t1_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 37;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t1_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[78].valid = 0;
    if (GiveReminder_guide_reminder_2_t1_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 38;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t1_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[79].valid = 0;
    if (GiveReminder_guide_reminder_2_t1_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 39;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t1_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[80].valid = 0;
    if (GiveReminder_guide_reminder_2_t1_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 40;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t1_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[81].valid = 0;
    if (GiveReminder_guide_reminder_2_t1_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 41;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t1_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[82].valid = 0;
    if (GiveReminder_guide_reminder_2_t2_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 42;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t2_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[83].valid = 0;
    if (GiveReminder_guide_reminder_2_t2_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 43;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t2_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[84].valid = 0;
    if (GiveReminder_guide_reminder_2_t2_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 44;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t2_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[85].valid = 0;
    if (GiveReminder_guide_reminder_2_t2_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 45;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t2_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[86].valid = 0;
    if (GiveReminder_guide_reminder_2_t2_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 46;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t2_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[87].valid = 0;
    if (GiveReminder_guide_reminder_2_t2_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 47;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t2_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[88].valid = 0;
    if (GiveReminder_guide_reminder_2_t3_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 48;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t3_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[89].valid = 0;
    if (GiveReminder_guide_reminder_2_t3_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 49;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t3_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[90].valid = 0;
    if (GiveReminder_guide_reminder_2_t3_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 50;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t3_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[91].valid = 0;
    if (GiveReminder_guide_reminder_2_t3_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 51;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t3_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[92].valid = 0;
    if (GiveReminder_guide_reminder_2_t3_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 52;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t3_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[93].valid = 0;
    if (GiveReminder_guide_reminder_2_t3_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 53;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t3_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[94].valid = 0;
    if (GiveReminder_guide_reminder_2_t5_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 54;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t5_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[95].valid = 0;
    if (GiveReminder_guide_reminder_2_t5_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 55;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t5_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[96].valid = 0;
    if (GiveReminder_guide_reminder_2_t5_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 56;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t5_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[97].valid = 0;
    if (GiveReminder_guide_reminder_2_t5_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 57;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t5_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[98].valid = 0;
    if (GiveReminder_guide_reminder_2_t5_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 58;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t5_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[99].valid = 0;
    if (GiveReminder_guide_reminder_2_t5_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 59;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_2_t5_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[100].valid = 0;
    if (GiveReminder_guide_reminder_1_t4_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 60;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t4_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[101].valid = 0;
    if (GiveReminder_guide_reminder_1_t4_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 61;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t4_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[102].valid = 0;
    if (GiveReminder_guide_reminder_1_t4_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 62;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t4_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[103].valid = 0;
    if (GiveReminder_guide_reminder_1_t4_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 63;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t4_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[104].valid = 0;
    if (GiveReminder_guide_reminder_1_t4_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 64;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t4_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[105].valid = 0;
    if (GiveReminder_guide_reminder_1_t4_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 65;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t4_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[106].valid = 0;
    if (GiveReminder_guide_reminder_1_t1_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 66;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t1_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[107].valid = 0;
    if (GiveReminder_guide_reminder_1_t1_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 67;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t1_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[108].valid = 0;
    if (GiveReminder_guide_reminder_1_t1_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 68;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t1_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[109].valid = 0;
    if (GiveReminder_guide_reminder_1_t1_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 69;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t1_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[110].valid = 0;
    if (GiveReminder_guide_reminder_1_t1_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 70;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t1_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[111].valid = 0;
    if (GiveReminder_guide_reminder_1_t1_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 71;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t1_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[112].valid = 0;
    if (GiveReminder_guide_reminder_1_t2_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 72;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t2_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[113].valid = 0;
    if (GiveReminder_guide_reminder_1_t2_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 73;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t2_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[114].valid = 0;
    if (GiveReminder_guide_reminder_1_t2_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 74;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t2_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[115].valid = 0;
    if (GiveReminder_guide_reminder_1_t2_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 75;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t2_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[116].valid = 0;
    if (GiveReminder_guide_reminder_1_t2_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 76;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t2_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[117].valid = 0;
    if (GiveReminder_guide_reminder_1_t2_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 77;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t2_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[118].valid = 0;
    if (GiveReminder_guide_reminder_1_t3_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 78;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t3_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[119].valid = 0;
    if (GiveReminder_guide_reminder_1_t3_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 79;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t3_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[120].valid = 0;
    if (GiveReminder_guide_reminder_1_t3_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 80;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t3_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[121].valid = 0;
    if (GiveReminder_guide_reminder_1_t3_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 81;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t3_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[122].valid = 0;
    if (GiveReminder_guide_reminder_1_t3_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 82;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t3_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[123].valid = 0;
    if (GiveReminder_guide_reminder_1_t3_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 83;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t3_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[124].valid = 0;
    if (GiveReminder_guide_reminder_1_t5_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 84;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t5_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[125].valid = 0;
    if (GiveReminder_guide_reminder_1_t5_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 85;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t5_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[126].valid = 0;
    if (GiveReminder_guide_reminder_1_t5_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 86;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t5_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[127].valid = 0;
    if (GiveReminder_guide_reminder_1_t5_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 87;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t5_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[128].valid = 0;
    if (GiveReminder_guide_reminder_1_t5_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 88;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t5_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[129].valid = 0;
    if (GiveReminder_guide_reminder_1_t5_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 89;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_guide_reminder_1_t5_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[130].valid = 0;
    if (GiveReminder_recorded_reminder_t4_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 90;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t4_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[131].valid = 0;
    if (GiveReminder_recorded_reminder_t4_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 91;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t4_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[132].valid = 0;
    if (GiveReminder_recorded_reminder_t4_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 92;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t4_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[133].valid = 0;
    if (GiveReminder_recorded_reminder_t4_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 93;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t4_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[134].valid = 0;
    if (GiveReminder_recorded_reminder_t4_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 94;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t4_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[135].valid = 0;
    if (GiveReminder_recorded_reminder_t4_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 95;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t4_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[136].valid = 0;
    if (GiveReminder_recorded_reminder_t1_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 96;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t1_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[137].valid = 0;
    if (GiveReminder_recorded_reminder_t1_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 97;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t1_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[138].valid = 0;
    if (GiveReminder_recorded_reminder_t1_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 98;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t1_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[139].valid = 0;
    if (GiveReminder_recorded_reminder_t1_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 99;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t1_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[140].valid = 0;
    if (GiveReminder_recorded_reminder_t1_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 100;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t1_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[141].valid = 0;
    if (GiveReminder_recorded_reminder_t1_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 101;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t1_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[142].valid = 0;
    if (GiveReminder_recorded_reminder_t2_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 102;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t2_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[143].valid = 0;
    if (GiveReminder_recorded_reminder_t2_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 103;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t2_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[144].valid = 0;
    if (GiveReminder_recorded_reminder_t2_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 104;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t2_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[145].valid = 0;
    if (GiveReminder_recorded_reminder_t2_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 105;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t2_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[146].valid = 0;
    if (GiveReminder_recorded_reminder_t2_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 106;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t2_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[147].valid = 0;
    if (GiveReminder_recorded_reminder_t2_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 107;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t2_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[148].valid = 0;
    if (GiveReminder_recorded_reminder_t3_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 108;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t3_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[149].valid = 0;
    if (GiveReminder_recorded_reminder_t3_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 109;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t3_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[150].valid = 0;
    if (GiveReminder_recorded_reminder_t3_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 110;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t3_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[151].valid = 0;
    if (GiveReminder_recorded_reminder_t3_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 111;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t3_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[152].valid = 0;
    if (GiveReminder_recorded_reminder_t3_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 112;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t3_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[153].valid = 0;
    if (GiveReminder_recorded_reminder_t3_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 113;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t3_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[154].valid = 0;
    if (GiveReminder_recorded_reminder_t5_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 114;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t5_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[155].valid = 0;
    if (GiveReminder_recorded_reminder_t5_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 115;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t5_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[156].valid = 0;
    if (GiveReminder_recorded_reminder_t5_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 116;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t5_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[157].valid = 0;
    if (GiveReminder_recorded_reminder_t5_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 117;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t5_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[158].valid = 0;
    if (GiveReminder_recorded_reminder_t5_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 118;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t5_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[159].valid = 0;
    if (GiveReminder_recorded_reminder_t5_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 119;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            GiveReminder_recorded_reminder_t5_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[160].valid = 0;
    if (MoveToLandmark_t4_couch_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 120;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t4_couch_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[161].valid = 0;
    if (MoveToLandmark_t4_couch_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 121;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t4_couch_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[162].valid = 0;
    if (MoveToLandmark_t4_kitchen_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 122;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t4_kitchen_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[163].valid = 0;
    if (MoveToLandmark_t4_kitchen_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 123;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t4_kitchen_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[164].valid = 0;
    if (MoveToLandmark_t1_couch_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 124;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t1_couch_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[165].valid = 0;
    if (MoveToLandmark_t1_couch_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 125;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t1_couch_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[166].valid = 0;
    if (MoveToLandmark_t1_kitchen_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 126;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t1_kitchen_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[167].valid = 0;
    if (MoveToLandmark_t1_kitchen_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 127;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t1_kitchen_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[168].valid = 0;
    if (MoveToLandmark_t2_couch_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 128;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t2_couch_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[169].valid = 0;
    if (MoveToLandmark_t2_couch_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 129;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t2_couch_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[170].valid = 0;
    if (MoveToLandmark_t2_kitchen_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 130;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t2_kitchen_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[171].valid = 0;
    if (MoveToLandmark_t2_kitchen_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 131;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t2_kitchen_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[172].valid = 0;
    if (MoveToLandmark_t3_couch_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 132;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t3_couch_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[173].valid = 0;
    if (MoveToLandmark_t3_couch_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 133;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t3_couch_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[174].valid = 0;
    if (MoveToLandmark_t3_kitchen_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 134;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t3_kitchen_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[175].valid = 0;
    if (MoveToLandmark_t3_kitchen_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 135;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t3_kitchen_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[176].valid = 0;
    if (MoveToLandmark_t5_couch_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 136;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t5_couch_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[177].valid = 0;
    if (MoveToLandmark_t5_couch_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 137;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t5_couch_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[178].valid = 0;
    if (MoveToLandmark_t5_kitchen_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 138;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t5_kitchen_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[179].valid = 0;
    if (MoveToLandmark_t5_kitchen_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 139;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MoveToLandmark_t5_kitchen_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[180].valid = 0;
    if (MakeCall_caregiver_call_t4_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 140;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t4_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[181].valid = 0;
    if (MakeCall_caregiver_call_t4_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 141;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t4_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[182].valid = 0;
    if (MakeCall_caregiver_call_t4_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 142;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t4_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[183].valid = 0;
    if (MakeCall_caregiver_call_t4_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 143;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t4_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[184].valid = 0;
    if (MakeCall_caregiver_call_t4_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 144;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t4_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[185].valid = 0;
    if (MakeCall_caregiver_call_t4_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 145;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t4_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[186].valid = 0;
    if (MakeCall_caregiver_call_t1_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 146;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t1_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[187].valid = 0;
    if (MakeCall_caregiver_call_t1_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 147;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t1_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[188].valid = 0;
    if (MakeCall_caregiver_call_t1_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 148;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t1_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[189].valid = 0;
    if (MakeCall_caregiver_call_t1_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 149;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t1_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[190].valid = 0;
    if (MakeCall_caregiver_call_t1_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 150;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t1_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[191].valid = 0;
    if (MakeCall_caregiver_call_t1_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 151;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t1_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[192].valid = 0;
    if (MakeCall_caregiver_call_t2_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 152;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t2_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[193].valid = 0;
    if (MakeCall_caregiver_call_t2_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 153;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t2_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[194].valid = 0;
    if (MakeCall_caregiver_call_t2_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 154;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t2_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[195].valid = 0;
    if (MakeCall_caregiver_call_t2_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 155;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t2_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[196].valid = 0;
    if (MakeCall_caregiver_call_t2_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 156;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t2_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[197].valid = 0;
    if (MakeCall_caregiver_call_t2_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 157;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t2_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[198].valid = 0;
    if (MakeCall_caregiver_call_t3_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 158;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t3_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[199].valid = 0;
    if (MakeCall_caregiver_call_t3_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 159;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t3_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[200].valid = 0;
    if (MakeCall_caregiver_call_t3_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 160;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t3_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[201].valid = 0;
    if (MakeCall_caregiver_call_t3_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 161;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t3_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[202].valid = 0;
    if (MakeCall_caregiver_call_t3_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 162;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t3_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[203].valid = 0;
    if (MakeCall_caregiver_call_t3_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 163;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t3_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[204].valid = 0;
    if (MakeCall_caregiver_call_t5_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 164;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t5_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[205].valid = 0;
    if (MakeCall_caregiver_call_t5_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 165;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t5_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[206].valid = 0;
    if (MakeCall_caregiver_call_t5_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 166;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t5_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[207].valid = 0;
    if (MakeCall_caregiver_call_t5_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 167;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t5_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[208].valid = 0;
    if (MakeCall_caregiver_call_t5_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 168;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t5_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[209].valid = 0;
    if (MakeCall_caregiver_call_t5_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 169;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_t5_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[210].valid = 0;
    if (MakeCall_caregiver_call_guide_t4_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 170;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t4_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[211].valid = 0;
    if (MakeCall_caregiver_call_guide_t4_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 171;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t4_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[212].valid = 0;
    if (MakeCall_caregiver_call_guide_t4_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 172;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t4_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[213].valid = 0;
    if (MakeCall_caregiver_call_guide_t4_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 173;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t4_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[214].valid = 0;
    if (MakeCall_caregiver_call_guide_t4_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 174;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t4_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[215].valid = 0;
    if (MakeCall_caregiver_call_guide_t4_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 175;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t4_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[216].valid = 0;
    if (MakeCall_caregiver_call_guide_t1_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 176;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t1_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[217].valid = 0;
    if (MakeCall_caregiver_call_guide_t1_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 177;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t1_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[218].valid = 0;
    if (MakeCall_caregiver_call_guide_t1_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 178;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t1_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[219].valid = 0;
    if (MakeCall_caregiver_call_guide_t1_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 179;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t1_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[220].valid = 0;
    if (MakeCall_caregiver_call_guide_t1_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 180;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t1_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[221].valid = 0;
    if (MakeCall_caregiver_call_guide_t1_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 181;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t1_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[222].valid = 0;
    if (MakeCall_caregiver_call_guide_t2_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 182;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t2_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[223].valid = 0;
    if (MakeCall_caregiver_call_guide_t2_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 183;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t2_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[224].valid = 0;
    if (MakeCall_caregiver_call_guide_t2_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 184;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t2_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[225].valid = 0;
    if (MakeCall_caregiver_call_guide_t2_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 185;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t2_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[226].valid = 0;
    if (MakeCall_caregiver_call_guide_t2_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 186;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t2_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[227].valid = 0;
    if (MakeCall_caregiver_call_guide_t2_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 187;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t2_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[228].valid = 0;
    if (MakeCall_caregiver_call_guide_t3_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 188;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t3_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[229].valid = 0;
    if (MakeCall_caregiver_call_guide_t3_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 189;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t3_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[230].valid = 0;
    if (MakeCall_caregiver_call_guide_t3_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 190;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t3_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[231].valid = 0;
    if (MakeCall_caregiver_call_guide_t3_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 191;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t3_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[232].valid = 0;
    if (MakeCall_caregiver_call_guide_t3_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 192;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t3_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[233].valid = 0;
    if (MakeCall_caregiver_call_guide_t3_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 193;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t3_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[234].valid = 0;
    if (MakeCall_caregiver_call_guide_t5_nathan_guide_2_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 194;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t5_nathan_guide_2_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[235].valid = 0;
    if (MakeCall_caregiver_call_guide_t5_nathan_guide_1_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 195;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t5_nathan_guide_1_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[236].valid = 0;
    if (MakeCall_caregiver_call_guide_t5_nathan_automated_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 196;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t5_nathan_automated_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[237].valid = 0;
    if (MakeCall_caregiver_call_guide_t5_nathan_recorded_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 197;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t5_nathan_recorded_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[238].valid = 0;
    if (MakeCall_caregiver_call_guide_t5_nathan_call_caregiver_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 198;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t5_nathan_call_caregiver_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[239].valid = 0;
    if (MakeCall_caregiver_call_guide_t5_nathan_call_caregiver_guide_msg::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 199;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MakeCall_caregiver_call_guide_t5_nathan_call_caregiver_guide_msg::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[240].valid = 0;
    if (MessageGivenSuccess_::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 200;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MessageGivenSuccess_::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[241].valid = 0;
    if (MedicineTakenSuccess_::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 201;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            MedicineTakenSuccess_::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[242].valid = 0;
    if (FoodEatenSuccess_::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 202;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            FoodEatenSuccess_::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[243].valid = 0;
    if (PersonAtSuccess_nathan_t4_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 203;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t4_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[244].valid = 0;
    if (PersonAtSuccess_nathan_t4_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 204;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t4_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[245].valid = 0;
    if (PersonAtSuccess_nathan_t1_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 205;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t1_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[246].valid = 0;
    if (PersonAtSuccess_nathan_t1_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 206;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t1_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[247].valid = 0;
    if (PersonAtSuccess_nathan_t2_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 207;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t2_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[248].valid = 0;
    if (PersonAtSuccess_nathan_t2_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 208;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t2_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[249].valid = 0;
    if (PersonAtSuccess_nathan_t3_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 209;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t3_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[250].valid = 0;
    if (PersonAtSuccess_nathan_t3_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 210;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t3_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[251].valid = 0;
    if (PersonAtSuccess_nathan_t5_couch::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 211;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t5_couch::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    
    new_states[252].valid = 0;
    if (PersonAtSuccess_nathan_t5_kitchen::check_preconditions(cur_state)){
            new_states[num] = cur_state;
            new_states[num].action = 212;
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            PersonAtSuccess_nathan_t5_kitchen::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    

}

} // pddl_lib