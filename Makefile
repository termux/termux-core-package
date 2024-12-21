export TERMUX_CORE_PKG__VERSION ?= 0.1.0
export TERMUX_CORE_PKG__ARCH
export TERMUX_CORE__INSTALL_PREFIX

export TERMUX__NAME ?= Termux# Default value: `Termux`
export TERMUX__LNAME ?= termux# Default value: `termux`

export TERMUX__REPOS_HOST_ORG_NAME ?= termux# Default value: `termux`
export TERMUX__REPOS_HOST_ORG_URL ?= https://github.com/$(TERMUX__REPOS_HOST_ORG_NAME)# Default value: `https://github.com/termux`

export TERMUX_APP__PACKAGE_NAME ?= com.termux# Default value: `com.termux`
export TERMUX_APP__DATA_DIR ?= /data/data/$(TERMUX_APP__PACKAGE_NAME)# Default value: `/data/data/com.termux`

export TERMUX__ROOTFS ?= $(TERMUX_APP__DATA_DIR)/files# Default value: `/data/data/com.termux/files`
export TERMUX__HOME ?= $(TERMUX__ROOTFS)/home# Default value: `/data/data/com.termux/files/home`
export TERMUX__PREFIX ?= $(TERMUX__ROOTFS)/usr# Default value: `/data/data/com.termux/files/usr`

export TERMUX_ENV__S_ROOT ?= TERMUX_# Default value: `TERMUX_`
export TERMUX_ENV__SS_TERMUX ?= _# Default value: `_`
export TERMUX_ENV__S_TERMUX ?= $(TERMUX_ENV__S_ROOT)$(TERMUX_ENV__SS_TERMUX)# Default value: `TERMUX__`
export TERMUX_ENV__SS_TERMUX_APP ?= APP__# Default value: `APP__`
export TERMUX_ENV__S_TERMUX_APP ?= $(TERMUX_ENV__S_ROOT)$(TERMUX_ENV__SS_TERMUX_APP)# Default value: `TERMUX_APP__`
export TERMUX_ENV__SS_TERMUX_CORE__TESTS ?= CORE__TESTS__# Default value: `CORE__TESTS__`
export TERMUX_ENV__S_TERMUX_CORE__TESTS ?= $(TERMUX_ENV__S_ROOT)$(TERMUX_ENV__SS_TERMUX_CORE__TESTS)# Default value: `TERMUX_CORE__TESTS__`

export TERMUX_PACKAGES__REPO_NAME ?= termux-packages# Default value: `termux-packages`
export TERMUX_PACKAGES__REPO_URL ?= $(TERMUX__REPOS_HOST_ORG_URL)/$(TERMUX_PACKAGES__REPO_NAME)# Default value: `https://github.com/termux/termux-packages`
export TERMUX_PACKAGES__REPO_DIR

export TERMUX_CORE_PKG__REPO_NAME ?= termux-core-package# Default value: `termux-core-package`
export TERMUX_CORE_PKG__REPO_URL ?= $(TERMUX__REPOS_HOST_ORG_URL)/$(TERMUX_CORE_PKG__REPO_NAME)# Default value: `https://github.com/termux/termux-core-package`
export TERMUX_CORE__TERMUX_REPLACE_TERMUX_CORE_SRC_SCRIPTS_FILE := $(TERMUX_PACKAGES__REPO_DIR)/packages/termux-core/build/scripts/termux-replace-termux-core-src-scripts



# Check if TERMUX_PACKAGES__REPO_DIR and TERMUX_CORE__TERMUX_REPLACE_TERMUX_CORE_SRC_SCRIPTS_FILE are set and exist
ifeq ($(TERMUX_PACKAGES__REPO_DIR),)
    $(error The TERMUX_PACKAGES__REPO_DIR variable is not set)
else ifneq ($(patsubst /%,,$(TERMUX_PACKAGES__REPO_DIR)),)
    $(error The TERMUX_PACKAGES__REPO_DIR variable '$(TERMUX_PACKAGES__REPO_DIR)' is not set to an absolute path)
else ifneq ($(shell test -d "$(TERMUX_PACKAGES__REPO_DIR)" && echo 1 || echo 0), 1)
    $(error The termux-pacakges repo directory does not exist at TERMUX_PACKAGES__REPO_DIR '$(patsubst /%,,$(TERMUX_PACKAGES__REPO_DIR))' path)
else ifneq ($(shell test -f "$(TERMUX_CORE__TERMUX_REPLACE_TERMUX_CORE_SRC_SCRIPTS_FILE)" && echo 1 || echo 0), 1)
    $(error The 'replace-termux-core-src-scripts' file does not exist at TERMUX_CORE__TERMUX_REPLACE_TERMUX_CORE_SRC_SCRIPTS_FILE '$(TERMUX_CORE__TERMUX_REPLACE_TERMUX_CORE_SRC_SCRIPTS_FILE)' path)
endif



# If architecture not set, find it for the compiler based on which
# predefined architecture macro is defined. The `shell` function
# replaces newlines with a space and a literal space cannot be entered
# in a makefile as its used as a splitter, hence $(SPACE) variable is
# created and used for matching.
ifeq ($(TERMUX_CORE_PKG__ARCH),)
	export PREDEFINED_MACROS := $(shell $(CC) -x c /dev/null -dM -E)
	EMPTY :=
	SPACE := $(EMPTY) $(EMPTY)
	ifneq (,$(findstring $(SPACE)#define __i686__ 1$(SPACE),$(SPACE)$(PREDEFINED_MACROS)$(SPACE)))
		TERMUX_CORE_PKG__ARCH := i686
	else ifneq (,$(findstring $(SPACE)#define __x86_64__ 1$(SPACE),$(SPACE)$(PREDEFINED_MACROS)$(SPACE)))
		TERMUX_CORE_PKG__ARCH := x86_64
	else ifneq (,$(findstring $(SPACE)#define __aarch64__ 1$(SPACE),$(SPACE)$(PREDEFINED_MACROS)$(SPACE)))
		TERMUX_CORE_PKG__ARCH := aarch64
	else ifneq (,$(findstring $(SPACE)#define __arm__ 1$(SPACE),$(SPACE)$(PREDEFINED_MACROS)$(SPACE)))
		TERMUX_CORE_PKG__ARCH := arm
	else
        $(error Unsupported package arch)
	endif
endif


ifeq ($(DESTDIR)$(PREFIX),)
	TERMUX_CORE__INSTALL_PREFIX := $(TERMUX__PREFIX)
else
	TERMUX_CORE__INSTALL_PREFIX := $(DESTDIR)$(PREFIX)
endif


# Escape `&`, `\` and `/` characters to be used as replacement string
# in sed `s/regexp/replacement/` expression.
# The `\` character needs to be escaped twice since used in command
# string with double quoted arguments, which are then passed to `sh`
# as a single quoted argument.
# - https://stackoverflow.com/a/29613573/14686958
TERMUX_PACKAGES__REPO_URL__ESCAPED := $(shell printf '%s' "$(TERMUX_PACKAGES__REPO_URL)" | sed -e '$$!{:a;N;$$!ba;}; s/[\]/\\\\&/g; s/[&/]/\\&/g; s/\n/\\&/g')
TERMUX_CORE_PKG__REPO_URL__ESCAPED := $(shell printf '%s' "$(TERMUX_CORE_PKG__REPO_URL)" | sed -e '$$!{:a;N;$$!ba;}; s/[\]/\\\\&/g; s/[&/]/\\&/g; s/\n/\\&/g')


BUILD_DIR := build
PREFIX_BUILD_DIR := $(BUILD_DIR)/usr
BIN_BUILD_DIR := $(PREFIX_BUILD_DIR)/bin
TESTS_BUILD_DIR := $(PREFIX_BUILD_DIR)/libexec/installed-tests/termux-core


TERMUX_CONSTANTS_SED_ARGS := \
	-e "s%[@]TERMUX_CORE_PKG__VERSION[@]%$(TERMUX_CORE_PKG__VERSION)%g" \
	-e "s%[@]TERMUX_CORE_PKG__ARCH[@]%$(TERMUX_CORE_PKG__ARCH)%g" \
	-e "s%[@]TERMUX__LNAME[@]%$(TERMUX__LNAME)%g" \
	-e "s%[@]TERMUX_APP__PACKAGE_NAME[@]%$(TERMUX_APP__PACKAGE_NAME)%g" \
	-e "s%[@]TERMUX_APP__DATA_DIR[@]%$(TERMUX_APP__DATA_DIR)%g" \
	-e "s%[@]TERMUX__ROOTFS[@]%$(TERMUX__ROOTFS)%g" \
	-e "s%[@]TERMUX__HOME[@]%$(TERMUX__HOME)%g" \
	-e "s%[@]TERMUX__PREFIX[@]%$(TERMUX__PREFIX)%g" \
	-e "s%[@]TERMUX_ENV__S_ROOT[@]%$(TERMUX_ENV__S_ROOT)%g" \
	-e "s%[@]TERMUX_ENV__S_TERMUX[@]%$(TERMUX_ENV__S_TERMUX)%g" \
	-e "s%[@]TERMUX_ENV__S_TERMUX_APP[@]%$(TERMUX_ENV__S_TERMUX_APP)%g" \
	-e "s%[@]TERMUX_ENV__S_TERMUX_CORE__TESTS[@]%$(TERMUX_ENV__S_TERMUX_CORE__TESTS)%g" \
	-e "s%[@]TERMUX_PACKAGES__REPO_URL[@]%$(TERMUX_PACKAGES__REPO_URL__ESCAPED)%g" \
	-e "s%[@]TERMUX_CORE_PKG__REPO_URL[@]%$(TERMUX_CORE_PKG__REPO_URL__ESCAPED)%g"

define replace-termux-constants
	sed $(TERMUX_CONSTANTS_SED_ARGS) "$1.in" > "$2/$$(basename "$1" | sed "s/\.in$$//")"
endef


all:
	@echo "Building src/scripts/*"
	@mkdir -p $(BIN_BUILD_DIR)
	find src/scripts -maxdepth 1 -type f -name "*.in" -exec sh -c \
		'sed $(TERMUX_CONSTANTS_SED_ARGS) "$$1" > $(BIN_BUILD_DIR)/"$$(basename "$$1" | sed "s/\.in$$//")"' sh "{}" \;
	find $(BIN_BUILD_DIR) -maxdepth 1 -type f -exec chmod u+x "{}" \;
	find src/scripts -maxdepth 1 -type l -exec cp -a "{}" $(BIN_BUILD_DIR)/ \;
	"$(TERMUX_CORE__TERMUX_REPLACE_TERMUX_CORE_SRC_SCRIPTS_FILE)" $(BIN_BUILD_DIR)/*


	@mkdir -p $(TESTS_BUILD_DIR)

	@echo "Building tests/termux-core-tests"
	$(call replace-termux-constants,tests/termux-core-tests,$(TESTS_BUILD_DIR))
	chmod u+x $(TESTS_BUILD_DIR)/termux-core-tests


	@echo "Building termux-core-package.json"
	$(call replace-termux-constants,termux-core-package.json,$(BUILD_DIR))



clean:
	rm -rf $(BUILD_DIR)

install:
	@echo "Installing in $(TERMUX_CORE__INSTALL_PREFIX)"

	install -d $(TERMUX_CORE__INSTALL_PREFIX)/bin
	find $(BIN_BUILD_DIR) -maxdepth 1 \( -type f -o -type l \) -exec cp -a "{}" $(TERMUX_CORE__INSTALL_PREFIX)/bin/ \;

	install -d $(TERMUX_CORE__INSTALL_PREFIX)/libexec/installed-tests/termux-core
	find $(TESTS_BUILD_DIR) -maxdepth 1 -type f -exec install "{}" -t $(TERMUX_CORE__INSTALL_PREFIX)/libexec/installed-tests/termux-core/ \;

uninstall:
	@echo "Uninstalling from $(TERMUX_CORE__INSTALL_PREFIX)"

	find src/scripts -maxdepth 1 \( -type f -o -type l \) -exec sh -c \
		'rm -f "$(TERMUX_CORE__INSTALL_PREFIX)/bin/$$(basename "$$1" | sed "s/\.in$$//")"' sh "{}" \;

	rm -rf $(TERMUX_CORE__INSTALL_PREFIX)/libexec/installed-tests/termux-core



deb: all
	termux-create-package $(BUILD_DIR)/termux-core-package.json



test: all
	$(MAKE) TERMUX_CORE__INSTALL_PREFIX=install install

	@echo "Running tests"
	install/libexec/installed-tests/termux-core/termux-core-tests --tests-path="install/libexec/installed-tests/termux-core" -vvv all

test-runtime: all
	$(MAKE) TERMUX_CORE__INSTALL_PREFIX=install install

	@echo "Running runtime tests"
	install/libexec/installed-tests/termux-core/termux-core-tests --tests-path="install/libexec/installed-tests/termux-core" -vvv runtime



.PHONY: all clean install uninstall deb test test-runtime
