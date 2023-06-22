namespace {{domain_name}} {
class {{class_name}} : public BT::SyncActionNode {
public:
    {{class_name}}(const std::string &name, const BT::NodeConfiguration &config)
            : SyncActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return { {% for param in parameters %}BT::InputPort<std::string>("{{param}}"), {% endfor %} };
    }

    BT::NodeStatus tick() override {
        std::unordered_map<std::string, std::string> param_subs;
        {% for param in parameters %}BT::Optional <std::string> {{param}} = getInput<std::string>("{{param}}");
        {% endfor -%}
        {% for param in parameters %}
        if (!{{param}}) {
            throw BT::RuntimeError("missing required input {{param}}");
        }
        param_subs["{{param}}"] = {{param}}.value();
        {%- endfor %}

        auto action_opt = parse_action(R"({{action_str}})");
        if (!action_opt.has_value()) {
            throw BT::RuntimeError("failed to parse action string for {{class_name}}");
        }
        auto action = action_opt.value();

        auto & kb = KnowledgeBase::getInstance();
        InstantiatedAction inst_action = instantiate_action(action, param_subs, kb.objects);
        bool precondition_met = kb.check_conditions(inst_action.precondtions);

        std::cout << "ACTION: " << inst_action.name << " params: ";
        for (const auto & p : inst_action.parameters){
            std::cout << p.name << " ";
        }
        std::cout << std::endl;
        if (!precondition_met){
            std::cout << "\t" << "preconditions violated" << std::endl;
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus status = tick_action(inst_action);
        if (status == BT::NodeStatus::SUCCESS){
            kb.apply_conditions(inst_action.effect);
            kb.apply_conditions(inst_action.observe);
            kb.apply_constraints();

        } else if (status == BT::NodeStatus::FAILURE){
            InstantiatedCondition cond;
            cond.op = OPERATION::NOT;
            cond.conditions.push_back(inst_action.observe);
            kb.apply_conditions(cond);
            kb.apply_constraints();

            std::cout << "\t" << "action failed" << std::endl;
        }
        return status;
    }

    BT::NodeStatus tick_action(const InstantiatedAction & action);

};
} // {{domain_name}}
