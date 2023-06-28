#include <cstddef>
#include <bits/functional_hash.h>

#pragma once

namespace pddl_lib {
    struct KBState {
        unsigned char data[{{size_kb_data}}];
    };
} // pddl_lib

namespace std {
    template<>
    struct hash<pddl_lib::KBState> {
        std::size_t operator()(const pddl_lib::KBState &obj) const {
            std::size_t hashValue = 0;
            std::size_t* data = (std::size_t*) obj.data;
            constexpr std::size_t prime = 1099511628211;  // A large prime number

            for (size_t i = 0; i < {{size_kb_data}}/64; ++i) {
                hashValue = (hashValue * prime) ^ data[i];
            }

            return hashValue;
        }
    };
}

namespace pddl_lib {

{% for action in actions %}
struct {{action.name}} {
    bool check_preconditions(const KBState & state) {
        return {{action.pre}}
    }
    void apply_effect(KBState & state) {
    {{action.effect}}
    }
};
{% endfor %}

} // pddl_lib