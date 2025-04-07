PREFIX      = /usr/local # the prefix to install to
BIN_PREFIX  = $(addsuffix /bin/, $(PREFIX))
MAN1_PREFIX = $(addsuffix /share/man/man1/, $(PREFIX))
BASH_PREFIX = $(addsuffix /share/bash-completion/completions/, $(PREFIX))
ZSH_PREFIX  = $(addsuffix /share/zsh/site-functions/_, $(PREFIX))

SRC_TARGETS             = makepkg-recursively git-status
SRC_BIN_FILES           = $(addprefix bin/, $(SRC_TARGETS))
SRC_INTERMEDIATE_FILES  = $(addprefix src/, $(SRC_TARGETS))
BIN                     = $(shell ls bin)
BIN_TARGETS             = $(addprefix $(BIN_PREFIX), $(BIN))
MAN1                    = $(shell cd man && find . -name '*.1')
MAN1_TARGETS            = $(addprefix $(MAN1_PREFIX), $(MAN1))
BASH_COMPLETION         = $(shell ls completion/bash)
BASH_COMPLETION_TARGETS = $(addprefix $(BASH_PREFIX), $(BASH_COMPLETION))
ZSH_COMPLETION          = $(shell ls completion/zsh)
ZSH_COMPLETION_TARGETS  = $(addprefix $(ZSH_PREFIX), $(ZSH_COMPLETION))

CXXFLAGS += -O2 -std=c++17 -MMD

.PHONY: all
all: srcs

.PHONY: install
install: bins man1s bash-completions zsh-completions

.PHONY: srcs bins man1s bash-completions zsh-completions
srcs: $(SRC_BIN_FILES)
bins: $(BIN_TARGETS)
man1s: $(MAN1_TARGETS)
bash-completions: $(BASH_COMPLETION_TARGETS)
zsh-completions: $(ZSH_COMPLETION_TARGETS)

$(BIN_PREFIX)%: bin/%
	install -Dm755 $< $@

$(MAN1_PREFIX)%: man/%
	install -Dm644 $< $@

$(BASH_PREFIX)%: completion/bash/%
	install -Dm644 $< $@

$(ZSH_PREFIX)%: completion/zsh/%
	install -Dm644 $< $@

bin/makepkg-recursively: src/makepkg-recursively
	cp $< $@

bin/git-status: src/git-status
	cp $< $@

.PHONY: clean
clean:
	rm -f $(SRC_BIN_FILES) $(SRC_INTERMEDIATE_FILES) src/*.d
