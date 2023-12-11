PREFIX = /usr/local # the prefix to install to
BIN_PREFIX = $(PREFIX)/bin/
MAN1_PREFIX = $(PREFIX)/share/man/man1/
BASH_PREFIX = $(PREFIX)/share/bash-completion/completions/
ZSH_PREFIX = $(PREFIX)/share/zsh/site-functions/_

BIN = $(shell ls bin)
BIN_TARGETS = $(addprefix $(BIN_PREFIX), $(BIN))
MAN1 = $(shell cd man && find -name '*.1')
MAN1_TARGETS = $(addprefix $(MAN1_PREFIX), $(MAN1))
BASH_COMPLETION = $(shell ls completion/bash)
BASH_COMPLETION_TARGETS = $(addprefix $(BASH_PREFIX), $(BASH_COMPLETION))
ZSH_COMPLETION = $(shell ls completion/zsh)
ZSH_COMPLETION_TARGETS = $(addprefix $(ZSH_PREFIX), $(ZSH_COMPLETION))

.PHONY: install
install: bins man1s bash-completions zsh-completions

.PHONY: bins man1s bash-completions zsh-completions
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
