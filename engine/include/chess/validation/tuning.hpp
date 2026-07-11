#ifndef CHESS_TUNING_HPP
#define CHESS_TUNING_HPP

#include "chess/search/evaluation_constants.hpp"
#include <string>
#include <vector>
#include <functional>

namespace chess::validation {

struct TuningParameter {
    std::string name;
    int* ptr;
    int default_value;
    int min_value;
    int max_value;
    int step;
};

class TuningManager {
public:
    static TuningManager& get();

    TuningManager();

    // Registers all weights (PSTs and structural weights)
    void register_parameters();

    const std::vector<TuningParameter>& get_parameters() const { return parameters_; }

    // Config file loading/saving
    bool load_weights(const std::string& filepath);
    bool save_weights(const std::string& filepath);

    // Dynamic parameter modification
    bool set_parameter(const std::string& name, int value);
    bool get_parameter(const std::string& name, int& value) const;

    // Reset all parameters to their default values
    void reset_to_defaults();

    // Tuning algorithms
    // Runs hill climbing on selected parameters using the provided evaluator
    // The evaluator takes a candidate index and runs games, returning a score (e.g. net wins or Elo change)
    void run_hill_climbing(const std::vector<std::string>& param_names,
                           int iterations,
                           const std::function<double(const std::string& param_name, int new_val)>& evaluator);

    void run_grid_search(const std::vector<std::string>& param_names,
                         const std::function<double()>& evaluator);

private:
    std::vector<TuningParameter> parameters_;

    void add_param(const std::string& name, int* ptr, int min_val, int max_val, int step);
    void add_pst(const std::string& prefix, search::PST& pst, int min_val, int max_val, int step);
};

} // namespace chess::validation

#endif // CHESS_TUNING_HPP
