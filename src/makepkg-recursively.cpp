#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

std::vector<std::string> SplitString(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::string::size_type start = 0;
    std::string::size_type end = input.find(delimiter);
    while (end != std::string::npos) {
        result.emplace_back(input.substr(start, end - start));
        start = end + 1;
        end = input.find(delimiter, start);
    }
    if (start != input.size()) {
        result.emplace_back(input.substr(start));
    }
    return result;
}

std::vector<std::string> SplitString(const std::string& input, const std::string& delimiter) {
    std::vector<std::string> result;
    std::string::size_type start = 0;
    std::string::size_type end = input.find(delimiter);
    while (end != std::string::npos) {
        result.emplace_back(input.substr(start, end - start));
        start = end + delimiter.size();
        end = input.find(delimiter, start);
    }
    if (start != input.size()) {
        result.emplace_back(input.substr(start));
    }
    return result;
}

std::string StripString(const std::string& input) {
    std::string::size_type start = 0;
    std::string::size_type end = input.size() - 1;
    while (start < input.size() && std::isspace(input[start])) {
        ++start;
    }
    while (end > start && std::isspace(input[end])) {
        --end;
    }
    return input.substr(start, end - start + 1);
}

class CommandOutput {
public:
    CommandOutput() = delete;

    CommandOutput(const char* command) {
        pipe = popen(command, "r");
        char buffer[128];
        if (pipe == nullptr) {
            std::cerr << "popen failed" << std::endl;
            exit(1);
        }
        while (fgets(buffer, 128, pipe) != nullptr) {
            output += buffer;
        }
    }

    ~CommandOutput() = default;

    int ReturnCode() {
        if (returned) {
            return returnCode;
        } else {
            returnCode = pclose(pipe);
            returned = true;
            return returnCode;
        }
    }

    std::string ToString() const {
        return output;
    }

    std::vector<std::string> Split(char delimiter) {
        return SplitString(output, delimiter);
    }

    std::string Strip() {
        return StripString(output);
    }

private:
    std::string output;
    FILE* pipe;
    bool returned = false;
    int returnCode;
};

bool SelectResult(std::string line, std::vector<bool>& selectionList) {
    auto strings = SplitString(line, ' ');
    for (auto& segment : strings) {
        if (segment.empty()) continue;
        try {
            if (segment.find('-') != std::string::npos) { // a-b
                auto numbers = SplitString(segment, '-');
                if (numbers.size() != 2) return false;
                auto from = std::stoi(numbers[0]);
                auto to = std::stoi(numbers[1]);
                if (from > to || from < 1 || to > selectionList.size()) return false;
                for (auto i = from - 1; i < to; ++i) {
                    selectionList[i] = true;
                }
            } else if (segment[0] == '^') {
                auto exclude = std::stoi(segment.substr(1));
                if (exclude < 1 || exclude > selectionList.size()) return false;
                selectionList[exclude - 1] = false;
            } else {
                auto selection = std::stoi(segment);
                if (selection < 1 || selection > selectionList.size()) return false;
                selectionList[selection - 1] = true;
            }
        } catch (...) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> GetAllPkgbuilds() {
    auto list = CommandOutput("find . -name PKGBUILD 2>/dev/null | xargs dirname 2> /dev/null | sort").Split('\n');
    for (auto& item : list) {
        item = item.substr(2);
    }
    return list;
}

std::pair<std::vector<std::string>, bool> SelectPkgbuilds(std::vector<std::string> pkgbuilds) {
    if (pkgbuilds.empty()) {
        std::cout << "==> No PKGBUILDs found" << std::endl;
        return {{}, false};
    }
    std::vector<bool> selection(pkgbuilds.size());
    for (size_t i = 0; i < pkgbuilds.size(); ++i) {
        selection[i] = false;
    }

    // print pkgbuilds
    std::cout << "==> found " << pkgbuilds.size() << " PKGBUILDs in:" << std::endl;
    for (size_t i = 0; i < pkgbuilds.size(); ++i) {
        std::cout << '\t' << i + 1 << '\t' <<pkgbuilds[i] << std::endl;
    }
    std::cout << "==> Choose the package(s) you want to install: (for example: \"1 2 3\", \"1-3\", \"^4\", use space to seperate)"
              << std::endl << "==>";
    std::string line;
    if (std::getline(std::cin, line)) {
        if (SelectResult(line, selection)) {
            std::cout << "==> Selected packages:" << std::endl;
            for (size_t i = 0; i < pkgbuilds.size(); ++i) {
                if (selection[i]) {
                    std::cout << '\t' << i + 1 << '\t' << pkgbuilds[i] << std::endl;
                }
            }
            std::cout << "==> Continue? (Y/n)" << std::endl << "==>";
            std::getline(std::cin, line);
            if (line.empty() || line[0] == 'Y' || line[0] == 'y') {
                std::vector<std::string> selected;
                for (size_t i = 0; i < pkgbuilds.size(); ++i) {
                    if (selection[i]) {
                        selected.emplace_back(pkgbuilds[i]);
                    }
                }
                return {selected, true};
            } else {
                return {{}, false};
            }
        } else {
            std::cout << "==> Invalid input" << std::endl;
            return {{}, false};
        }
    } else {
        return {{}, false};
    }
}

int main(int argc, char** argv) {
    auto pkgbuilds = GetAllPkgbuilds();
    auto [result, ok]= SelectPkgbuilds(pkgbuilds);
    if (!ok) {
        return 1;
    }
    std::string arguments;
    for (int i = 1; i < argc; ++i) {
        arguments += " \'";
        arguments += argv[i];
        arguments += '\'';
    }
    for (auto& pkgbuild : result) {
        std::cout << "==> Installing " << pkgbuild << std::endl;
        auto command = "cd " + pkgbuild + " && makepkg" + arguments;
        system(command.c_str());
    }
    return 0;
}
