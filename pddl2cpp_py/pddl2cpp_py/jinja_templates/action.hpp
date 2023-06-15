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

        return tick_action(instantiate_action(action, param_subs));
    }

    BT::NodeStatus tick_action(const InstantiatedAction & action);

};