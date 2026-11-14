/*
Copyright 11/09/2025 https://github.com/su8/kurger
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;
#endif /* _WIN32 */

namespace {

constexpr unsigned long kMaxPage = 9999UL;       /* keep in sync with the GUI */
constexpr unsigned long kMaxResolution = 9999UL;
constexpr const char *kImageFormat = "png";
constexpr const char *kDevice = "png16m";

struct Options {
  unsigned long firstPage = 0;
  unsigned long lastPage = 0;
  unsigned long resolution = 0;
  std::string inputFile;
};

void usage(const char *program) {
  std::cout << "Usage: " << program
            << " from <first> to <last> resolution <dpi> file <input.pdf>\n"
            << "Example: " << program
            << " from 1 to 10 resolution 100 file \"the book.pdf\"\n";
}

bool parseUnsigned(const char *text, unsigned long maximum, unsigned long &out) {
  if (text == nullptr || *text == '\0') {
    return false;
  }

  errno = 0;
  char *end = nullptr;
  const unsigned long value = std::strtoul(text, &end, 10);
  if (errno == ERANGE || end == text || *end != '\0' || value > maximum) {
    return false;
  }

  out = value;
  return true;
}

enum class ParseStatus { Ok, ShowUsage, InvalidRange };

ParseStatus parseArgs(int argc, char *argv[], Options &options) {
  if (argc != 9) {
    return ParseStatus::ShowUsage;
  }

  if (std::strcmp(argv[1], "from") != 0 || std::strcmp(argv[3], "to") != 0 ||
      std::strcmp(argv[5], "resolution") != 0 ||
      std::strcmp(argv[7], "file") != 0) {
    return ParseStatus::ShowUsage;
  }

  if (!parseUnsigned(argv[2], kMaxPage, options.firstPage) ||
      !parseUnsigned(argv[4], kMaxPage, options.lastPage) ||
      !parseUnsigned(argv[6], kMaxResolution, options.resolution) ||
      options.firstPage < 1) {
    return ParseStatus::ShowUsage;
  }

  if (options.lastPage < options.firstPage) {
    return ParseStatus::InvalidRange;
  }

  if ((options.lastPage - options.firstPage) + 1 > kMaxPage) {
    return ParseStatus::ShowUsage;
  }

  options.inputFile = argv[8];
  return ParseStatus::Ok;
}

std::string lowercase(std::string text) {
  for (char &c : text) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return text;
}

/*
 * Ghostscript treats '%' in -sOutputFile as the start of a page-number
 * placeholder. A literal '%' in the input path has to be doubled.
 */
std::string escapeGhostscriptPattern(const std::string &text) {
  std::string escaped;
  escaped.reserve(text.size() * 2U);
  for (char c : text) {
    escaped += c;
    if (c == '%') {
      escaped += c;
    }
  }
  return escaped;
}

std::vector<std::string> ghostscriptArguments(const Options &options,
                                             const std::string &outputPattern) {
  return {
      "-dBATCH",
      "-dNOPAUSE",
      "-dQUIET",
      "-dFirstPage=" + std::to_string(options.firstPage),
      "-dLastPage=" + std::to_string(options.lastPage),
      "-sOutputFile=" + outputPattern,
      std::string("-sDEVICE=") + kDevice,
      "-r" + std::to_string(options.resolution),
      "-dGraphicsAlphaBits=4",
      "-sBandListStorage=memory",
      "-dBufferSpace=99000",
      "-dNumRenderingThreads=8",
      options.inputFile,
  };
}

#if defined(_WIN32)
/*
 * Quote a single argument following the rules used by the Microsoft C runtime
 * so that a process started with CreateProcess receives exactly this argument.
 */
std::string quoteWindowsArgument(const std::string &argument) {
  if (argument.empty()) {
    return "\"\"";
  }
  if (argument.find_first_of(" \t\n\v\"") == std::string::npos) {
    return argument;
  }

  std::string quoted = "\"";
  std::size_t backslashes = 0;
  for (char c : argument) {
    if (c == '\\') {
      ++backslashes;
      continue;
    }
    if (c == '"') {
      quoted.append(backslashes * 2U + 1U, '\\');
      quoted += '"';
    } else {
      quoted.append(backslashes, '\\');
      quoted += c;
    }
    backslashes = 0;
  }
  quoted.append(backslashes * 2U, '\\');
  quoted += '"';
  return quoted;
}
#endif /* _WIN32 */

/*
 * Run a program without going through a shell. This fixes both the broken
 * quoting of file names containing spaces and the command injection that was
 * possible through a carefully crafted PDF file name.
 */
bool runProgram(const std::string &program,
                const std::vector<std::string> &arguments, bool wait) {
#if defined(_WIN32)
  std::string commandLine = quoteWindowsArgument(program);
  for (const std::string &argument : arguments) {
    commandLine += ' ';
    commandLine += quoteWindowsArgument(argument);
  }

  std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
  mutableCommandLine.push_back('\0');

  STARTUPINFOA startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  PROCESS_INFORMATION processInfo{};

  if (CreateProcessA(nullptr, mutableCommandLine.data(), nullptr, nullptr,
                     FALSE, 0, nullptr, nullptr, &startupInfo,
                     &processInfo) == 0) {
    return false;
  }

  if (!wait) {
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
  }

  WaitForSingleObject(processInfo.hProcess, INFINITE);
  DWORD exitCode = 1;
  GetExitCodeProcess(processInfo.hProcess, &exitCode);
  CloseHandle(processInfo.hThread);
  CloseHandle(processInfo.hProcess);
  return exitCode == 0;
#else
  std::vector<char *> argv;
  argv.reserve(arguments.size() + 2U);
  argv.push_back(const_cast<char *>(program.c_str()));
  for (const std::string &argument : arguments) {
    argv.push_back(const_cast<char *>(argument.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = 0;
  if (posix_spawnp(&pid, program.c_str(), nullptr, nullptr, argv.data(),
                   environ) != 0) {
    return false;
  }
  if (!wait) {
    return true;
  }

  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    return false;
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif /* _WIN32 */
}

std::string ghostscriptProgram() {
  const char *fromEnvironment = std::getenv("KURGER_GS");
  if (fromEnvironment != nullptr && *fromEnvironment != '\0') {
    return fromEnvironment;
  }
#if defined(_WIN32)
  return "C:\\gs\\bin\\gswin64c.exe";
#else
  return "gs";
#endif /* _WIN32 */
}

bool openFileManager(const std::filesystem::path &directory) {
  const std::vector<std::string> arguments = {directory.string()};
#if defined(_WIN32)
  return runProgram("explorer", arguments, false);
#elif defined(__APPLE__)
  return runProgram("open", arguments, false);
#else
  return runProgram("xdg-open", arguments, false);
#endif
}

} /* namespace */

int main(int argc, char *argv[]) {
  if (argc == 2 &&
      (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0)) {
    usage(argv[0]);
    return EXIT_SUCCESS;
  }

  Options options;
  const ParseStatus status = parseArgs(argc, argv, options);
  if (status == ParseStatus::InvalidRange) {
    std::cerr << "From page can't be greater than To page.\n";
    return EXIT_FAILURE;
  }
  if (status != ParseStatus::Ok) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  const std::filesystem::path inputPath(options.inputFile);
  std::error_code error;

  if (!std::filesystem::exists(inputPath, error) ||
      !std::filesystem::is_regular_file(inputPath, error)) {
    std::cerr << options.inputFile << " doesn't exist or is not a file.\n";
    return EXIT_FAILURE;
  }

  if (lowercase(inputPath.extension().string()) != ".pdf") {
    std::cerr << options.inputFile << " is not a PDF file.\n";
    return EXIT_FAILURE;
  }

  const std::string baseName = inputPath.stem().string();
  const std::filesystem::path outputDirectory = options.inputFile + "_converted";

  std::filesystem::create_directories(outputDirectory, error);
  if (error || !std::filesystem::is_directory(outputDirectory, error)) {
    std::cerr << "Could not create the output directory "
              << outputDirectory.string() << ".\n";
    return EXIT_FAILURE;
  }

  const std::string outputPattern = escapeGhostscriptPattern(options.inputFile) +
                                    "_pAge_%01d." + kImageFormat;

  std::cout << "Please wait until we convert the requested images."
            << std::endl;

  if (!runProgram(ghostscriptProgram(),
                  ghostscriptArguments(options, outputPattern), true)) {
    std::cerr << "Ghostscript could not convert the file. Make sure it is "
                 "installed and that the PDF is valid.\n";
    return EXIT_FAILURE;
  }

  bool renamedEveryPage = true;
  const unsigned long pageCount = (options.lastPage - options.firstPage) + 1;

  for (unsigned long page = 0; page < pageCount; ++page) {
    const std::string source =
        options.inputFile + "_pAge_" + std::to_string(page + 1) + "." +
        kImageFormat;
    const std::string target =
        (outputDirectory /
         (baseName + "_page_" +
          std::to_string(options.firstPage + page) + "." + kImageFormat))
            .string();

    std::error_code removeError;
    std::filesystem::remove(target, removeError);

    std::error_code renameError;
    std::filesystem::rename(source, target, renameError);
    if (renameError) {
      std::cerr << "Could not rename " << source << " to " << target << ": "
                << renameError.message() << "\n";
      renamedEveryPage = false;
    }
  }

  if (!renamedEveryPage) {
    return EXIT_FAILURE;
  }

  std::cout << "Done" << std::endl;
  openFileManager(outputDirectory);
  return EXIT_SUCCESS;
}
