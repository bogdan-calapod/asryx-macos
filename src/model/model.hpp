#ifndef ASRYX_MODEL_MODEL_HPP
#define ASRYX_MODEL_MODEL_HPP

#include <string>
#include <vector>

namespace model {

const std::vector<std::string>& get_supported_models();
std::string get_model_path(const std::string& name);
bool is_model_installed(const std::string& name);
void list_models();
void install_model(const std::string& name);
void use_model(const std::string& name);
void uninstall_model(const std::string& name);

} // namespace model

#endif // ASRYX_MODEL_MODEL_HPP
