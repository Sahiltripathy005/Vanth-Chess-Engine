#include "chess/uci/option_manager.hpp"
#include "chess/search/transposition_table.hpp"
#include <iostream>
#include <sstream>

namespace chess::uci {

OptionManager& OptionManager::get() {
    static OptionManager instance;
    return instance;
}

OptionManager::OptionManager() {
    // Register standard UCI options
    register_option({
        "Hash", OptionType::Spin, "16", "16", 1, 2048, {},
        [](const std::string& val) {
            try {
                int size = std::stoi(val);
                search::g_tt.resize(size);
            } catch (...) {}
        }
    });

    register_option({
        "Threads", OptionType::Spin, "1", "1", 1, 64, {},
        [](const std::string& val) {
            (void)val;
        }
    });

    register_option({
        "MultiPV", OptionType::Spin, "1", "1", 1, 256, {},
        [](const std::string& val) {
            (void)val;
        }
    });

    register_option({
        "OwnBook", OptionType::Check, "true", "true", 0, 0, {},
        [](const std::string& val) {
            (void)val;
        }
    });

    register_option({
        "Ponder", OptionType::Check, "false", "false", 0, 0, {},
        [](const std::string& val) {
            (void)val;
        }
    });

    register_option({
        "Move Overhead", OptionType::Spin, "10", "10", 0, 5000, {},
        [](const std::string& val) {
            (void)val;
        }
    });

    register_option({
        "Skill Level", OptionType::Spin, "20", "20", 0, 20, {},
        [](const std::string& val) {
            (void)val;
        }
    });

    register_option({
        "Contempt", OptionType::Spin, "0", "0", -100, 100, {},
        [](const std::string& val) {
            (void)val;
        }
    });
}

void OptionManager::register_option(const UCIOption& option) {
    options_[option.name] = option;
}

bool OptionManager::set_option(const std::string& name, const std::string& value) {
    auto it = options_.find(name);
    if (it == options_.end()) return false;

    // Validate value based on type
    if (it->second.type == OptionType::Spin) {
        try {
            int val = std::stoi(value);
            if (val < it->second.min_val || val > it->second.max_val) return false;
        } catch (...) {
            return false;
        }
    } else if (it->second.type == OptionType::Check) {
        if (value != "true" && value != "false") return false;
    }

    it->second.current_val = value;
    if (it->second.on_change) {
        it->second.on_change(value);
    }
    return true;
}

std::string OptionManager::get_option(const std::string& name) const {
    auto it = options_.find(name);
    if (it != options_.end()) {
        return it->second.current_val;
    }
    return "";
}

int OptionManager::get_spin(const std::string& name) const {
    std::string val = get_option(name);
    if (val.empty()) return 0;
    try {
        return std::stoi(val);
    } catch (...) {
        return 0;
    }
}

bool OptionManager::get_check(const std::string& name) const {
    return get_option(name) == "true";
}

std::string OptionManager::get_string(const std::string& name) const {
    return get_option(name);
}

void OptionManager::print_options() const {
    for (const auto& [name, opt] : options_) {
        std::cout << "option name " << name << " type ";
        switch (opt.type) {
            case OptionType::Check: std::cout << "check default " << opt.default_val; break;
            case OptionType::Spin: 
                std::cout << "spin default " << opt.default_val 
                          << " min " << opt.min_val << " max " << opt.max_val; 
                break;
            case OptionType::Combo:
                std::cout << "combo default " << opt.default_val;
                for (const auto& v : opt.combo_vars) {
                    std::cout << " var " << v;
                }
                break;
            case OptionType::String: std::cout << "string default " << opt.default_val; break;
            case OptionType::Button: std::cout << "button"; break;
        }
        std::cout << "\n";
    }
}

} // namespace chess::uci
