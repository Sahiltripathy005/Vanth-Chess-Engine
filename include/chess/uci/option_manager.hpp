#ifndef CHESS_UCI_OPTION_MANAGER_HPP
#define CHESS_UCI_OPTION_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace chess::uci {

enum class OptionType {
    Check,
    Spin,
    Combo,
    String,
    Button
};

struct UCIOption {
    std::string name;
    OptionType type;
    std::string default_val;
    std::string current_val;
    int min_val = 0;
    int max_val = 0;
    std::vector<std::string> combo_vars;
    std::function<void(const std::string&)> on_change;
};

class OptionManager {
public:
    static OptionManager& get();

    void register_option(const UCIOption& option);
    bool set_option(const std::string& name, const std::string& value);
    std::string get_option(const std::string& name) const;
    
    // Helper accessors
    int get_spin(const std::string& name) const;
    bool get_check(const std::string& name) const;
    std::string get_string(const std::string& name) const;

    // Print all options in UCI format
    void print_options() const;

private:
    OptionManager();
    std::map<std::string, UCIOption> options_;
};

} // namespace chess::uci

#endif // CHESS_UCI_OPTION_MANAGER_HPP
