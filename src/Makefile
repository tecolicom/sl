SHELL := /bin/bash

NCURSES_CFLAGS := $(shell ncurses6-config --cflags 2>/dev/null)
NCURSES_LIBS   := $(or $(shell ncurses6-config --libs 2>/dev/null),-lcurses)

NAME    := sl
GIT     := git -C ..
CURRENT := $(shell git -C .. describe --tags --abbrev=0 2>/dev/null)
YEAR    := $(shell date +%Y)
DATE    := $(shell date +%Y-%m-%d)
MAKE    := $(MAKE) --no-print-directory
CONFIRM ?= 1

.ONESHELL:
.PHONY: all build check check-changes release clean

VER := $(CURRENT)

MTOYODA := ../mtoyoda

all: sl-2026 sl-2023 sl-2010 sl-1992 sl-1985

sl-1985: sl-1985.c
	$(CC) -std=c89 -w \
		-Wno-incompatible-function-pointer-types \
		-Wno-incompatible-pointer-types \
		$(NCURSES_CFLAGS) \
		-include term.h -DCM=cursor_address \
		-o $@ $< $(NCURSES_LIBS)

# Coupler cars for sl-2026.
# List as NAME=1 (default on) or NAME=0 (default off).
# This auto-generates -DCAR_NAME=x flags and cars/name.c source list.
# Each car can be overridden at runtime via SL_CAR_NAME env var.
SL2026_CARS      := SWEEP=1 NULL=0 STREAK=0
SL2026_CAR_FLAGS := $(addprefix -DCAR_,$(SL2026_CARS))
SL2026_CAR_NAMES := $(foreach c,$(SL2026_CARS),$(firstword $(subst =, ,$(c))))
SL2026_CAR_SRCS  := cars/couplers.c \
	$(shell echo '$(SL2026_CAR_NAMES)' | tr A-Z a-z | sed 's/[^ ]*/cars\/&.c/g')
SL2026_ANIM_SRCS := art/arts.c art/sl.c art/clawd.c art/invader.c art/mtoyoda/sl.c art/swim.c art/bike.c

sl-2026: sl-2026.c $(SL2026_CAR_SRCS) $(SL2026_ANIM_SRCS) sl.h cars/coupler.h art/art.h art/mtoyoda/sl.h
	$(CC) $(NCURSES_CFLAGS) $(SL2026_CAR_FLAGS) -o $@ sl-2026.c $(SL2026_CAR_SRCS) $(SL2026_ANIM_SRCS) $(NCURSES_LIBS) -lm

sl-1992: $(MTOYODA)/sl.c
	$(CC) -O -Wall $(NCURSES_CFLAGS) -o $@ $< $(NCURSES_LIBS)

%: %.c
	$(CC) $(NCURSES_CFLAGS) -o $@ $< $(NCURSES_LIBS)

build: sl.1
	@echo "Version: $(VER)"
	grep '^\.TH' sl.1

sl.1: README.md deflist.lua
	pandoc -s -f markdown-smart -t man -L deflist.lua \
		-M title=SL -M section=1 -M date="$(YEAR)" -M header="sl" \
		-V footer="$(NAME) $(VER)" \
		README.md -o $@
	sed -i '' '/^\.IP$$/{N;/\.EX/s/^\.IP/.IP "" 2/;}' $@

check:
	@$(GIT) diff --quiet -- ':!src/$(NAME).1' ':!src/$(NAME)' ':!src/Changes' \
	 && $(GIT) diff --cached --quiet -- ':!src/$(NAME).1' ':!src/$(NAME)' ':!src/Changes' \
		|| { echo "ERROR: uncommitted changes"; exit 1; }
	echo "Working tree is clean"

check-changes:
	@awk '/\{\{.NEXT\}\}/{f=1; next} f && /^[[:space:]]+[^[:space:]]/{ok=1; exit} f && /^[^[:space:]]/{exit 1} END{exit(ok ? 0 : 1)}' Changes \
		|| { echo "ERROR: No entries under {{\$$NEXT}} in Changes"; exit 1; }
	echo "Changes OK"

release:
	@set -e
	test -n "$(VERSION)" || { echo "ERROR: VERSION required: make release VERSION=v2026"; exit 1; }
	$(GIT) diff --quiet -- ':!src/$(NAME).1' ':!src/$(NAME)' ':!src/Changes' \
	 && $(GIT) diff --cached --quiet -- ':!src/$(NAME).1' ':!src/$(NAME)' ':!src/Changes' \
		|| { echo "ERROR: uncommitted changes"; exit 1; }
	echo "$(VERSION)" | grep -qE '^v[0-9]' \
		|| { echo "ERROR: VERSION must start with v"; exit 1; }
	! $(GIT) tag -l | grep -qx "$(VERSION)" \
		|| { echo "ERROR: tag $(VERSION) already exists"; exit 1; }
	if grep -q '^$(VERSION) ' Changes; then
		echo "Changes already has $(VERSION) entry, skipping rewrite"
	else
		awk '/\{\{.NEXT\}\}/{f=1; next} f && /^[[:space:]]+[^[:space:]]/{ok=1; exit} f && /^[^[:space:]]/{exit 1} END{exit(ok ? 0 : 1)}' Changes \
			|| { echo "ERROR: No entries under {{\$$NEXT}} in Changes"; exit 1; }
		sed -i '' 's/{{\$$NEXT}}/{{\$$NEXT}}\'$$'\n''\'$$'\n''$(VERSION) $(DATE)/' Changes
	fi
	echo "Releasing $(VERSION) (current: $(CURRENT))"
	sed -i '' 's/^VERSION=.*/VERSION=$(VERSION)/' sl
	rm -f sl.1
	$(MAKE) build VER=$(VERSION)
	$(GIT) add -u
	msg="Checking in changes prior to tagging of version $(VERSION)."$$'\n\n'"Changelog diff is:"$$'\n\n'"$$($(GIT) --no-pager diff --cached -- src/Changes)"
	echo "$$msg"
	echo "---"
	$(GIT) --no-pager diff --cached --stat
	if [ -n "$(CONFIRM)" ]; then read -p "Commit and tag $(VERSION)? [y/N] " ans; else ans=y; fi
	[ "$$ans" = y ] || { echo "Aborted"; exit 1; }
	$(GIT) commit -m "$$msg"
	$(GIT) tag $(VERSION)
	$(GIT) push origin main $(VERSION)
	echo "Released $(VERSION)"

clean:
	rm -f sl-{2026,2023,2010,1992,1985}
