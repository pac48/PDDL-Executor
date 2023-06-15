class {{class_name}} : public BT::SyncActionNode {
public:
    {{class_name}}(const std::string &name, const BT::NodeConfiguration &config)
            : SyncActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return { {% for param in parameters %}BT::InputPort<std::string>("{{param}}"), {% endfor %} };
    }

    BT::NodeStatus tick() override {
        {% for param in parameters %}BT::Optional <std::string> {{param}} = getInput<std::string>("{{param}}");
        {% endfor -%}
        {% for param in parameters %}
        if (!{{param}}) {
            throw BT::RuntimeError("missing required input {{param}}");
        }
        {%- endfor %}

        return tick_action({% for param in parameters %}{{param}}.value() {%- if loop.index < loop.length %}, {% endif -%}{% endfor %});
    }

    BT::NodeStatus tick_action({% for param in parameters %}const std::string & {{param}} {%- if loop.index < loop.length %}, {% endif -%}{% endfor %});

};