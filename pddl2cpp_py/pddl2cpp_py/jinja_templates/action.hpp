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

//        BT::Optional <std::string> objects_str_opt = getInput<std::string>("objects");
//        if (!objects_str_opt) {
//            throw BT::RuntimeError("missing required input objects");
//        }
//        std::string objects_str = objects_opt.value();
//        std::vector<InstantiatedParameter> objects;
//        std::stringsream ss(objects_str);
//        while(!ss.eof()){
//            std::string name;
//            std::string type;
//            name << ss;
//            type << ss;
//            InstantiatedParameter param = {name, type};
//            objects.push_back(param);
//        }

        auto action_opt = parse_action(R"({{action_str}})");
        if (!action_opt.has_value()) {
            throw BT::RuntimeError("failed to parse action string for {{class_name}}");
        }
        auto action = action_opt.value();

        auto & kb = KnowledgeBase::getInstance();
        InstantiatedAction inst_action = instantiate_action(action, param_subs, kb.objects);
        bool precondition_met = kb.check_conditions(inst_action.precondtions);
        if (!precondition_met){
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus status = tick_action(inst_action);
        if (status == BT::NodeStatus::SUCCESS){
            kb.apply_conditions(inst_action.effect);
            kb.apply_conditions(inst_action.observe);
        } else if (status == BT::NodeStatus::FAILURE){
            InstantiatedCondition cond;
            cond.op = OPERATION::NOT;
            cond.conditions.push_back(inst_action.observe);
            kb.apply_conditions(cond);
        }
        return status;
    }

    BT::NodeStatus tick_action(const InstantiatedAction & action);

};