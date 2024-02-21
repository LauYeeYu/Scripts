#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

#include <cxxopts.hpp>

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

std::string GetTagOrHash() {
    auto tags = CommandOutput(
        "git for-each-ref --points-at=HEAD --count=2 --sort=version:refname --format=%\\(refname:short\\) refs/tags 2> /dev/null"
    ).Split('\n');
    if (tags.empty()) {
        return CommandOutput("git rev-parse --short HEAD 2> /dev/null").Strip();
    } else if (tags.size() == 1) {
        return tags[0];
    } else {
        return tags[0] + '+';
    }
}

int StashCount() {
    return CommandOutput("git stash list 2> /dev/null").Split('\n').size();
}

struct Status {
    std::string branch;
    int ahead = 0;
    int behind = 0;
    int untracked = 0;
    int staged = 0;
    int changed = 0;
    int conflicts = 0;
    int deleted = 0;
    int stashed = 0;
    bool clean = false;
};

Status GetStatus() {
    auto command = CommandOutput("LANG=C git status --porcelain --branch 2> /dev/null");
    if (command.ReturnCode() != 0) {
        exit(0); // not a git repository
    }
    auto status = command.Split('\n');
    Status result;
    for (const auto& line: status) {
        if (line[0] ==  '#' && line[1] == '#') {
            if (line.find("Initial commit on") != std::string::npos ||
                line.find("No commits yet on") != std::string::npos) {
                auto segments = SplitString(line, ' ');
                result.branch = segments[segments.size() - 1];
            } else if (line.find("no branch") != std::string::npos) {
                // detached HEAD
                result.branch = GetTagOrHash();
            } else {
                auto branchInfo = SplitString(
                    StripString(line.substr(2)),
                    "..."
                );
                result.branch = branchInfo[0];
                if (branchInfo.size() != 1) { // has remote branch
                    auto place = branchInfo[1].find(' ');
                    if (place != std::string::npos) { // not sync with remote branch
                    std::string syncStatusRaw = branchInfo[1].substr(place + 2, branchInfo[1].size() - place - 3);
                        auto syncStatus = SplitString(
                            syncStatusRaw, ", "
                        );
                        for (const auto& status: syncStatus) {
                            auto segments = SplitString(status, ' ');
                            if (segments[0] == "ahead") {
                                result.ahead = std::stoi(segments[1]);
                            } else if (segments[0] == "behind") {
                                result.behind = std::stoi(segments[1]);
                            }
                        }
                    }
                }
            }
        } else if (line[0] == '?' && line[1] == '?') {
            ++result.untracked;
        } else {
            if (line[1] == 'M') {
                ++result.changed;
            }
            if (line[1] == 'D') {
                ++result.deleted;
            }
            if (line[0] == 'U') {
                ++result.conflicts;
            } else if (line[0] != ' ') {
                ++result.staged;
            }
        }
    }
    result.stashed = StashCount();
    if (result.staged == 0 && result.changed == 0 && result.deleted == 0 &&
        result.untracked == 0 && result.conflicts == 0) {
        result.clean = true;
    }
    return result;
}

const char* format = R"(
default format: <branch> <ahead> <behind> <staged> <conflicts> <changed> <untracked> <stashed> <clean> <deleted>
)";

void PrintResult(const Status& status, const cxxopts::ParseResult& result);

int main(int argc, char** argv) {
    try {
        // Get options
        cxxopts::Options options("git-status", "Show brief git status");
        options.add_options()
            ("h,help", "Print usage")
            ("H,human", "Show human readable status");
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help() << format << std::endl;
            return 0;
        }

        auto status = GetStatus();
        PrintResult(status, result);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

void PrintResult(const Status& status, const cxxopts::ParseResult& result) {
    if (result.count("human")) {
        if (status.branch.empty()) {
            std::cout << "not a git repository" << std::endl;
        } else {
            std::cout << "Current branch: " << status.branch << '.' << std::endl;
            bool hasStatus = false;
            if (status.ahead > 0) {
                hasStatus = true;
                std::cout << status.ahead;
                if (status.ahead > 1) {
                    std::cout << " commits ahead";
                } else {
                    std::cout << " commit ahead";
                }
            }
            if (status.behind > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.behind;
                if (status.behind > 1) {
                    std::cout << " commits behind";
                } else {
                    std::cout << " commit behind";
                }
            }
            if (status.staged > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.staged;
                if (status.staged > 1) {
                    std::cout << " files staged";
                } else {
                    std::cout << " file staged";
                }
            }
            if (status.conflicts > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.conflicts;
                if (status.conflicts > 1) {
                    std::cout << " files in conflict";
                } else {
                    std::cout << " file in conflict";
                }
            }
            if (status.changed > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.changed;
                if (status.changed > 1) {
                    std::cout << " files changed";
                } else {
                    std::cout << " file changed";
                }
            }
            if (status.untracked > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.untracked;
                if (status.untracked > 1) {
                    std::cout << " files untracked";
                } else {
                    std::cout << " file untracked";
                }
            }
            if (status.stashed > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.stashed;
                if (status.stashed > 1) {
                    std::cout << " stashes";
                } else {
                    std::cout << " stash";
                }
            }
            if (status.clean) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << "working tree clean";
            }
            if (status.deleted > 0) {
                if (hasStatus) {
                    std::cout << ", ";
                }
                hasStatus = true;
                std::cout << status.deleted;
                if (status.deleted > 1) {
                    std::cout << " files deleted";
                } else {
                    std::cout << " file deleted";
                }
            }
            if (hasStatus) {
                std::cout << '.' << std::endl;
            }
        }
    } else {
        std::cout << status.branch << " "
            << status.ahead << " " << status.behind << " "
            << status.staged << " " << status.conflicts << " "
            << status.changed << " " << status.untracked << " "
            << status.stashed << " " << status.clean << " "
            << status.deleted;
    }
}
