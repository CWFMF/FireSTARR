/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_ARGUMENT_PARSER_H
#define FS_ARGUMENT_PARSER_H
#include "../stdafx.h"
#include "Settings.h"
namespace fs::settings
{
struct Usage
{
  string description{};
  string positional_arg_summary{};
};
enum class PositionalArgumentsRequired
{
  Required,
  NotRequired
};
class ArgumentParser
{
private:
  vector<string> positional_args_{};
  size_t cur_positional_{0};
  PositionalArgumentsRequired require_positional_{};
  bool help_requested_{false};

public:
  virtual ~ArgumentParser() = default;
  ArgumentParser(
    const vector<Usage> usages,
    const vector<string> arguments,
    const PositionalArgumentsRequired require_positional
  );
  ArgumentParser(
    const vector<Usage> usages,
    const vector<string> arguments,
    const string binary_directory,
    const string binary_name,
    const PositionalArgumentsRequired require_positional
  );
  ArgumentParser(
    const Usage usage,
    const int argc,
    const char* const argv[],
    const PositionalArgumentsRequired require_positional = PositionalArgumentsRequired::Required
  );
  ArgumentParser(
    const vector<Usage> usages,
    const int argc,
    const char* const argv[],
    const PositionalArgumentsRequired require_positional = PositionalArgumentsRequired::Required
  );
  // HACK: copying is breaking current argument tracking
  ArgumentParser(const ArgumentParser& rhs) = delete;
  ArgumentParser(ArgumentParser& rhs) = delete;
  ArgumentParser& operator=(const ArgumentParser& rhs) = delete;
  ArgumentParser& operator=(ArgumentParser& rhs) = delete;
  /**
   * \brief Parse arguments that were given to constructor
   * \return string Positional arguments
   */
  virtual Settings& parse_args();

protected:
  ArgumentParser(
    const vector<Usage> usages,
    const vector<string> arguments,
    const pair<string, string> binary,
    const PositionalArgumentsRequired require_positional
  )
    : ArgumentParser(usages, arguments, binary.first, binary.second, require_positional)
  { }
  /**
   * \brief If any more positional arguments are available
   */
  bool has_positional() const;
  /**
   * \brief Get next positional argument
   */
  string get_positional();
  void done_positional();

public:
  /**
   * \brief Indicate positional arguments should all be used by now and error if not
   */
  string cur_arg();
  string get_args();
  string get_arg() noexcept;
  bool help_requested() { return help_requested_; }
  void mark_parsed(const string arg);
  bool was_parsed(const string arg);
  string format_args();
  void show_args();
  void log_args();
  void show_usage_and_exit(int exit_code);
  void show_usage_and_exit();
  void show_help_and_exit();
  size_t argc() const { return arguments_.size(); }

protected:
  size_t cur_arg_{0};
  size_t skipped_args_{0};
  vector<string> arguments_{};
  vector<string> arguments_expanded_{};
  string binary_directory_{};
  string binary_name_{};
  vector<string>& args_expanded();
};
class SettingsArgumentParser : public ArgumentParser
{
public:
  using ArgumentParser::ArgumentParser;
  Settings& parse_args() override;
};
class MainArgumentParser : public SettingsArgumentParser
{
public:
  MainArgumentParser(const int argc, const char* const argv[]);
  Settings& parse_args() override;
};
}
#endif
