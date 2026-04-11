#include "platform/process.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iterator>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace platform {

bool command_exists(const std::string& name)
{
  const char* path_env = std::getenv("PATH");
  if (!path_env) {
    return false;
  }
  std::string path_str(path_env);
  std::stringstream ss(path_str);
  std::string item;
  while (std::getline(ss, item, ':')) {
    std::filesystem::path p = std::filesystem::path(item) / name;
    if (access(p.c_str(), X_OK) == 0) {
      if (std::filesystem::is_regular_file(p)) {
        return true;
      }
    }
  }
  return false;
}

pid_t spawn_process_background(const std::vector<std::string>& argv,
                               const std::string& redirect_file)
{
  if (argv.empty()) {
    return -1;
  }
  pid_t pid = fork();
  if (pid == -1) {
    return -1;
  }
  if (pid == 0) {
    // Child
    if (!redirect_file.empty()) {
      int fd = open(redirect_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd != -1) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
    }
    else {
      int dev_null = open("/dev/null", O_WRONLY);
      if (dev_null != -1) {
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        close(dev_null);
      }
    }

    std::vector<std::string> argv_copy = argv;
    std::vector<char*> c_argv;
    c_argv.reserve(argv.size() + 1);
    std::transform(argv_copy.begin(), argv_copy.end(), std::back_inserter(c_argv),
                   // cppcheck-suppress constParameterReference
                   [](std::string& arg) { return arg.data(); });
    c_argv.push_back(nullptr);

    execvp(c_argv[0], c_argv.data());
    exit(127);
  }
  return pid;
}

int wait_process(pid_t pid)
{
  if (pid <= 0) {
    return -1;
  }
  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    return -1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return -1;
}

bool run_process_blocking(const std::vector<std::string>& argv)
{
  pid_t pid = spawn_process_background(argv);
  if (pid == -1) {
    return false;
  }
  return wait_process(pid) == 0;
}

bool run_process_with_stdin(const std::vector<std::string>& argv, const std::string& input)
{
  if (argv.empty()) {
    return false;
  }
  std::array<int, 2> pipefd{};
  if (pipe(pipefd.data()) == -1) {
    return false;
  }
  pid_t pid = fork();
  if (pid == -1) {
    close(pipefd[0]);
    close(pipefd[1]);
    return false;
  }
  if (pid == 0) {
    // Child
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    int dev_null = open("/dev/null", O_WRONLY);
    if (dev_null != -1) {
      dup2(dev_null, STDOUT_FILENO);
      dup2(dev_null, STDERR_FILENO);
      close(dev_null);
    }

    std::vector<std::string> argv_copy = argv;
    std::vector<char*> c_argv;
    c_argv.reserve(argv.size() + 1);
    std::transform(argv_copy.begin(), argv_copy.end(), std::back_inserter(c_argv),
                   // cppcheck-suppress constParameterReference
                   [](std::string& arg) { return arg.data(); });
    c_argv.push_back(nullptr);

    execvp(c_argv[0], c_argv.data());
    exit(127);
  }

  // Parent
  close(pipefd[0]);
  size_t written = 0;
  while (written < input.size()) {
    ssize_t res = write(pipefd[1], input.data() + written, input.size() - written);
    if (res <= 0) {
      break;
    }
    written += static_cast<size_t>(res);
  }
  close(pipefd[1]);

  int status = 0;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool is_process_running(pid_t pid)
{
  if (pid <= 0) {
    return false;
  }
  return kill(pid, 0) == 0;
}

bool stop_process(pid_t pid, int sig)
{
  if (pid <= 0) {
    return false;
  }
  return kill(pid, sig) == 0;
}

} // namespace platform
