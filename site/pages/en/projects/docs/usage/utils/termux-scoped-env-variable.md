---
page_ref: "@ARK_PROJECT__VARIANT@/termux/termux-core-package/docs/@ARK_DOC__VERSION@/usage/utils/termux-scoped-env-variable.md"
---

# `termux-scoped-env-variable` Utils Docs

The `termux-scoped-env-variable` scripts/codes provide a way, if running in a Termux app, to get/set/unset variable names and values for `TERMUX*__` and other scoped environment variables exported by different Termux runtime components, with support for fallback values and validation of values.

### Contents

- [Rationale and Design](#rationale-and-design)
- [Scoped Variables](#scoped-variables)
- [Implementations](#implementations)

---

&nbsp;





## Rationale and Design

*If you do not know what `TERMUX*__` scoped environment variables means, check [Scoped Variables](#scoped-variables) section below.*

Since Termux app version `0.109`, a lot of effort has been spent on making the Termux project fork-friendly so that other projects can also [fork](https://github.com/termux/termux-app#forking) Termux and use it for their own requirements. Before that the Termux app and its plugin apps were using hardcoded `com.termux` package name and `/data/data/com.termux/*` paths. This was fixed by creating the [`TermuxConstants.java`](https://github.com/termux/termux-app/commit/14c49867f706c12b27d007ccede1c799041e857f) class and defining all the required constants in it so that all the Termux apps use it instead of hardcoding the values. The class was added as part of the `termux-shared` library ([1](https://github.com/termux/termux-app/wiki/Termux-Libraries), [2](https://github.com/termux/termux-app/tree/master/termux-shared), [3](https://github.com/termux/termux-app/commit/682ce083149be145512b11fe4037aa74ccbe0251)) added in Termux app version `0.109.0`. Additionally, hardcoded values in some of the package build scripts and patches in the `termux-packages` build infrastructure that still existed were also removed and instead replaced with referencing to constants defined in the [`properties.sh`] file.

Termux app version `0.119.0` introduced `TERMUX*__` scoped environment variables so that packages can rely on environment variables instead of hardcoded `/data/data/com.termux/*` paths that are added when packages are built with the `termux-packages` build infrastructure. This would allow packages to run under a Termux rootfs other than the one for which they were compiled for, or even allow packages that are not built with `termux-packages` build infrastructure to run properly, like for multiple rootfs support or if Termux app is installed in secondary users/work profiles, or on external sdcards, or if packages are running in an external app with a different app data or rootfs directory planned to be implemented future ([1](https://termux.dev/en/posts/general/2024/11/11/termux-selected-for-nlnet-ngi-mobifree-grant.html#dynamic-variables)). The `termux-scoped-env-variable` is an important part for making the Termux project fork-friendly so that packages can be used in Termux and its forks without issue, regardless of how packages are built or the Termux environment variable root scope (`TERMUX_`) being used.

`termux-scoped-env-variable` exists to solve the following issues.

1. The `TERMUX*__` scope is based on the `TERMUX_` value for the Termux environment root scope, under which all Termux variables exist. The `TERMUX_` value is defined by the `$TERMUX_ENV__S_ROOT` variable in [`properties.sh`] file of the `termux-packages` repo, and is used while building packages and is exported by the Termux app version `>= 0.119.0` and also used while generating other variable names to export. However, Termux forks may decide to change the value for `TERMUX_` root scope and may want to define variables under a root scope named after their own app name, like `FOO_`. Packages provided by Termux, or a third party (that are not built with `termux-packages` build infrastructure) cannot use the `TERMUX_` root scope when reading environment variables in such forks, as the variables wouldn't be set in Termux scope and would instead be set in the scope set by the fork, like `FOO_`. Due to this, the variable name to be read must be generated dynamically, by combining the root scope exported in the `$TERMUX_ENV__S_ROOT` environment variable by the Termux app with sub scope of respective Termux runtime component and the variable sub name to form the final the variable name, like the root scope `$TERMUX_ENV__S_ROOT=TERMUX_`, sub scope `$TERMUX_ENV__SS_TERMUX=_` for the Termux app and the variable name `PREFIX`  to form the variable name `$TERMUX__PREFIX`, or `$FOO__PREFIX` for the `FOO_` root scope. The `TERMUX_ENV__S_ROOT` is a hardcoded variable name and its own `$TERMUX_ENV__` scope is/must not be changed by Termux forks as it is used by packages to get the actual Termux scope of other variables. Dynamically generating variable names to read with `$TERMUX_ENV__` is only needed if a package provided by Termux or a third party wants to support Termux forks, otherwise they can continue to use `TERMUX_*` scoped variables.

2. Termux app version `< 0.119.0` did not export `TERMUX*__` scoped environment variables, and only primarily exported few variables like `$PREFIX` (without scope) and `$TERMUX_VERSION`, and relied on replacing placeholders in package sources during build time with hardcoded values. Since newer package versions may be installed on such older versions where variables are not exported, package sources still need to have placeholders as fallback if newer scoped environment variables are not set.

3. The Termux variables that are exported by Termux runtime components should be validated before they are used as they may not be set on older app versions or they may have been modified by user programs. Validation of environment variable values (or even placeholders replaced during build time) generally require regex or pattern matching to ensure values are in expected format, or otherwise program should abort with an error or fallback to a default value.

To solve the above issues involves a lot of code for every variable that needs to be read. Moreover, such variables may be read in different programming languages and so this logic to solve all the issues needs to either be implemented in each language, or the [`$PREFIX/bin/termux-scoped-env-variable`](#termux-scoped-env-variable) script available via the `$PATH` variable that is exported by the Termux app can be executed by programs regardless of their source language. Check [Implementations](#implementations) section for the implementations in different programming languages that are currently provided.

Another major issue is for shell scripts where exported environment variables are directly referenced where ever needed. Since the root scope may be different than the default `TERMUX_` value or the variable may not be set and a placeholder or default fallback may need to be used, `TERMUX*__` scoped variables cannot be referenced directly. `bash` (via `${!variable_name_to_expand}` indirect expansion), posix shells (via `eval`) and other shells do allow expanding variables based on dynamic names, but such expansions would have to be done where ever the variable is referenced, like with `PREFIX_VARNAME="${TERMUX_ENV__S_ROOT}_PREFIX"; PREFIX="${!PREFIX_VARNAME}"`, but this wouldn't work for Termux app version `< 0.119.0`, where `$TERMUX_ENV__S_ROOT` isn't exported. Scripts could also have placeholders for root scope that are replaced during build time, like `PREFIX="$@TERMUX_ENV__S_ROOT@_PREFIX"`, but not all packages are built with `termux-packages` build infrastructure, and such placeholders would break shell syntax checkers and highlighters and make code harder to read. The ideal way is to copy the required variables from the `FOO_` scoped variables into the `TERMUX*__` scoped variables with the same variable sub name at the start of the script or before usage by calling `termux-scoped-env-variable`, and then using the `TERMUX*__` scoped variables afterwards where ever they need to be referenced. For example, `$FOO__PREFIX` can be copied to `$TERMUX__PREFIX` via `termux-scoped-env-variable` at the start of a script and `$TERMUX__PREFIX` can be used later on. Additionally, the `@TERMUX__PREFIX@` placeholder could be replaced during build time that's passed to `termux-scoped-env-variable` and used as a fallback value for setting `$TERMUX__PREFIX`.

&nbsp;

If a package uses `TERMUX*__` scoped path based variables (like `$TERMUX__ROOTFS`, `$TERMUX__HOME`, `$TERMUX__PREFIX`) and uses `termux-scoped-env-variable` or a similar custom implementation, then following would apply. By "not built with `termux-packages` build infrastructure" mean packages download from upstream releases or built (on device) with `make`, etc.

- If running in Termux app version `< 0.119.0` where environment variables are not exported:  
    - If a package is built with `termux-packages` build infrastructure, then placeholder path values that are replaced during build time would be used.  
    - If a package is not built with `termux-packages` build infrastructure, then hardcoded `/data/data/com.termux/*` paths would have to be used. **The package will not work on forked Termux apps with a different package name** as hardcoded paths will be for the upstream Termux app and will not be accessible by the forked Termux app.  
- If running in Termux app version `>= 0.119.0` where environment variables are exported:  
    - If a package is built with `termux-packages` build infrastructure, then environment variables would be used if set, with a fallback to placeholder path values that are replaced during build time.  
    - If a package is not built with `termux-packages` build infrastructure, then environment variables would be used if set (assuming package reads them), with a fallback to hardcoded `/data/data/com.termux/*` paths instead of build time placeholder.  

Note that:
- If a custom implementation is used by a package to read Termux variables instead of `termux-scoped-env-variable`, and package is not being built with `termux-packages` build infrastructure (where root scope is patched during build), then package will not for Termux forks that use a different root scope than `TERMUX_`. So custom implementations should ensure that variable names being read are created using `$TERMUX_ENV__S_ROOT`.
- If a package is not built with the `termux-packages` build infrastructure, then it will not work in Termux app forks that have not merged the changes done in Termux app version `>= 0.119.0` for exporting path environment variables into their own apps.

---

&nbsp;





## Scoped Variables

Termux variables are named under different scopes for Termux app version `>= 0.119.0` depending on which Termux runtime components they are exported for or exported by. Scope means the variable name prefix, like `TERMUX*__`, to signify that the variable is from a distinct component. The double underscores suffix `__` after each scope name signifies a distinct scope, as a single underscore `_` is usually used to separate parts of a variable name itself. Think of scopes as `json` or `yaml` parent keys with their nested sub keys.

A scoped variable has `3` parts in the format `<root_scope><sub_scope><sub_name>`. The `root_scope` and `sub_scope` are combined to form a scope. The main scopes of Termux variables are defined in the `$TERMUX_ENV__*` variables of the [`properties.sh`] file of the `termux-packages` repo.
- `root_scope`: The root scope of the variable under which all variables are exported for the environment of a framework. The Termux root scope is defined by the `$TERMUX_ENV__S_ROOT` variable, which defaults to `TERMUX_`. The variables exported for Android have the `ANDROID_` root scope. The variables exported for external apps bound to Termux have their own root scope, normally generated from their app name.
- `sub_scope`: The sub scope of the variable under which all variables are exported for a specific component of a framework. The Termux sub scope for primary variables or variables for currently running Termux config is defined by the `$TERMUX_ENV__SS_TERMUX` variable, which defaults to `_`, which combined with the root scope forms `TERMUX__`. The Termux app sub scope is defined by the `$TERMUX_ENV__SS_TERMUX_APP` variable, which defaults to `APP__`, which combined with the root scope forms `TERMUX_APP__`. The variables exported for other components have their own sub scope.
- `sub_name`: The sub name of the variable under the root and sub scope, like the `ROOTFS` sub name under the `TERMUX_` root scope and `_` sub scope forms `TERMUX__ROOTFS`.

Check [initial implementation of `TERMUX__` scoped environment](https://github.com/termux/termux-app/commit/afc06cfd0a9855e8443d0b62cd57d8fa9e4547a1), and the [`shell.command.environment`](https://github.com/termux/termux-app/tree/v0.119.0/termux-shared/src/main/java/com/termux/shared/shell/command/environment) and [`termux.shell.command.environment`](https://github.com/termux/termux-app/tree/v0.119.0/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment) packages of `termux-shared` library for implementation of environment exported by the Termux app. Some environment variables may be read and exported by native packages.

Some of the Termux environment variable scopes are listed below.

- **`TERMUX__`:** Primary variables or variables for currently running Termux config. For example, the `$TERMUX__PREFIX` variable is for path to Termux prefix directory. Check [`TermuxShellEnvironment`](https://github.com/termux/termux-app/blob/v0.119.0/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellEnvironment.java), [`AndroidShellEnvironment`](https://github.com/termux/termux-app/blob/v0.119.0/termux-shared/src/main/java/com/termux/shared/shell/command/environment/AndroidShellEnvironment.java) and [`UnixShellEnvironment`](https://github.com/termux/termux-app/blob/v0.119.0/termux-shared/src/main/java/com/termux/shared/shell/command/environment/UnixShellEnvironment.java) for the implementation.
- **`TERMUX_APP__`:** Variables for the [Termux app](https://github.com/termux/termux-app). For example, the `$TERMUX_APP__APP_VERSION_NAME` variable is for Termux app version name. Check [`TermuxAppShellEnvironment`](https://github.com/termux/termux-app/blob/v0.119.0/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxAppShellEnvironment.java) for the implementation.
- **`TERMUX_API_APP__`:** Variables for the [Termux:API app](https://github.com/termux/termux-api). For example, the `$TERMUX_API_APP__APP_VERSION_NAME` variable is for Termux:API app version name.
- **`TERMUX_EXEC__`:** Variables for the [`termux-exec` library](https://github.com/termux/termux-exec). For example, the `$TERMUX_EXEC__INTERCEPT_EXECVE` variable is for whether to intercept `execve()` wrapper calls. Check [`termux_env.h`](https://github.com/termux/termux-exec/blob/v2.0.0/src/termux/termux_env.h) for the implementation.
- **`TERMUX_SHELL_CMD__`:** Variables for the currently running Termux shell command. For example, the `$TERMUX_SHELL_CMD__SHELL_ID` variable is for the unique id of the currently running shell. Check [`TermuxShellCommandShellEnvironment`](https://github.com/termux/termux-app/blob/v0.119.0/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellCommandShellEnvironment.java) and [`ShellCommandShellEnvironment`](https://github.com/termux/termux-app/blob/v0.119.0/termux-shared/src/main/java/com/termux/shared/shell/command/environment/ShellCommandShellEnvironment.java) for the implementation.

&nbsp;

The scope name, followed by double underscores `__` is necessary for the following reasons.

1. The `TERMUX*__` scopes help the Termux devs and users know which component a variable belongs to. Putting all the variables under the `TERMUX_` scope (instead of `TERMUX*__`) makes it harder to name and separate variables of different components from each other, especially as more and more variables and components keep getting added as the Termux project develops.

2. Different components may have variable names that fully or partially conflict with variable names of other components. For example, take the legacy (now deprecated) `$TERMUX_VERSION` variable for the Termux app version name, what does it mean just by looking at it? Version of what component of Termux? However, the new `$TERMUX_APP__APP_VERSION_NAME` and `$TERMUX_API_APP__APP_VERSION_NAME` variable names make it pretty clear that they refer to their specific apps, and are for the app [version name](https://developer.android.com/guide/topics/manifest/manifest-element.html#vname) and not their [version code](https://developer.android.com/guide/topics/manifest/manifest-element.html#vcode).

3. The distinct `TERMUX*__` scope makes it easier to provide separate documentation and anchor fragment links for each scope.

4. It also helps to filter in or out variables of scopes that are not going to be used. For example, a user may only need to use or show variables for `TERMUX_APP__`, that is not going to possible if all the Termux variables are under the `TERMUX_` scope without knowing every variable that belongs to the Termux app and then having to keep updating the list as more variables are added or renamed.

**Users and third party packages must not modify the default values of variables exported by Termux app and other runtime components that are meant to be read-only as that can lead to undefined or dangerous behaviour.**

---

&nbsp;





## Implementations

The following implementations are currently provided for `termux-scoped-env-variable` utils. Implementations for additional languages are planned for future, for now the `termux-scoped-env-variable` executable in `$PATH` can be used by them.

- [Executable in `$PATH` (`termux-scoped-env-variable`)](#termux-scoped-env-variable)
- [`bash` shell (`termux-scoped-env-variable.bash`)](#bash)
- [POSIX shells (`termux-scoped-env-variable.sh`)](#sh)
- [c/c++](#cc)

&nbsp;



### `termux-scoped-env-variable`

`termux-scoped-env-variable` exists in Termux default `bin` directory in `$PATH` in Termux environments and can be executed as a command with arguments.

Currently, it is a symlink to [`termux-scoped-env-variable.bash`](#bash), check its [Help](#help) section for more info on supported arguments and [Examples](#examples-1) section for additional examples.

**DO NOT** [`source`](https://www.gnu.org/software/bash/manual/bash.html#index-_002e) this file, as underlying implementation may not be guaranteed to be a `bash` syntax. It should be executed (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and its `stdout` captured for the result output.

The `output_mode` argument must be set to `>` so that output is returned in `stdout`, as if a variable name is passed instead, the variable will only be set in the `termux-scoped-env-variable` child process and not in the calling parent process. The `>` argument should be quoted with single `'` or double quotes `"` if running in a shell, otherwise it may be considered a [command redirection](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_07).

&nbsp;



#### Examples

##### `TERMUX__ROOTFS`

Get `$TERMUX__ROOTFS` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files` for the upstream Termux app. If no value matches a valid absolute path (including rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be printed to `stdout`.

```shell
termux-scoped-env-variable get-value '>' cn="termux" "ROOTFS" r+='^((/)|((/[^/]+)+))$' "@TERMUX__ROOTFS@" "/data/data/com.termux/files" || return $?
```

##### `TERMUX__PREFIX`

Get `$TERMUX__PREFIX` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files/usr` for the upstream Termux app. If no value matches a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be printed to `stdout`.

```shell
termux-scoped-env-variable get-value '>' cn="termux" "PREFIX" r+='^(/[^/]+)+$' "@TERMUX__PREFIX@" "/data/data/com.termux/files/usr" || return $?
```

Use sub scope mode (`ss=`) instead of component name scope mode (`cn=`).

```shell
termux-scoped-env-variable get-value '>' ss="_" "PREFIX" r+='^(/[^/]+)+$' "@TERMUX__PREFIX@" "/data/data/com.termux/files/usr" || return $?
```

Get `$TERMUX__PREFIX` variable name by generating it from the `$TERMUX_ENV__S_ROOT` environment variable, with a fallback to the build time placeholder and then to the hardcoded root scope value `TERMUX_` for the upstream Termux app. The output will be printed to `stdout`.

```shell
termux-scoped-env-variable get-name '>' cn="termux" "PREFIX" || return $?
```

## &nbsp;

&nbsp;



### bash

`termux-scoped-env-variable.bash` is a [`bash` shell](https://www.gnu.org/software/bash/manual/bash.html) implementation for `termux-scoped-env-variable` and exists in Termux default `bin` directory in `$PATH` in Termux environments.

The source file for the `termux-scoped-env-variable.bash` script is the [`termux-scoped-env-variable.bash.in`] file in the `termux-core-package` repo, and it internally uses the [`termux_core__bash__termux_scoped_env_variable()`] function defined in the `termux-packages` repo which is replaced at build time with the `@TERMUX_CORE__BASH__TERMUX_SCOPED_ENV_VARIABLE@` placeholder, see below for how.

`termux-scoped-env-variable.bash` script can be used at runtime by:

1. Sourcing the `termux-scoped-env-variable.bash` file in `$PATH` (with [`.`](https://www.gnu.org/software/bash/manual/bash.html#index-_002e) or [`source`](https://www.gnu.org/software/bash/manual/bash.html#index-source) command) by running the following commands and then calling the `termux_core__bash__termux_scoped_env_variable()` function that should be defined in the current shell environment.
For the `get-*` commands, the name of an environment variable in which the output should be set can be passed as the `output_mode` argument, instead of having to call the function in a subshell and capturing its `stdout` for the output.  
The `command -v` command is used to find the location of the script file instead of directly using the `.`/`source` command to prevent sourcing of a (malicious) file in the current working directory with the same name instead of the one in `$PATH`. A separate function is used to source so that arguments passed to calling script/function are not passed to the sourced script. Replace `exit` with `return` if running inside a function.
    ```shell
    source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then source "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

    source_file_from_path "termux-scoped-env-variable.bash" || exit $?

    termux_core__bash__termux_scoped_env_variable <command> output_variable <arguments...> || exit $?
    ```

2. Executing `termux-scoped-env-variable.bash` directly (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and capturing its `stdout`/`stderr` for the output and errors.
    ```shell
    output="$(termux-scoped-env-variable.bash <command> '>' <arguments...>)" || exit $?
    ```

Sourcing should be preferred, especially if multiple variable names or values need to be read. Sourcing the `termux-scoped-env-variable.bash` script once would be a single external call, and then any calls to the `termux_core__bash__termux_scoped_env_variable()` function would all be local calls, so would be faster than if `termux-scoped-env-variable.bash` script is executed multiple times with external calls for each variable. If efficiency is not an issue, then it doesn't matter much if script is executed or sourced, other than being able to pass a variable name as `output_mode`.

&nbsp;

If efficiency is an issue for time critical calling scripts that require low latency, the `termux_core__bash__termux_scoped_env_variable()` function can be imported into calling script source files so that external calls to source or execute `termux-scoped-env-variable.bash` is not required and the function can be called locally as it will already be defined. This can be done at build time by:

1. The [`termux_core__bash__termux_scoped_env_variable()`] function can be copied into calling script source files manually. It will have to kept in sync with upstream whenever any changes or fixes are done. The `@TERMUX_ENV__S_ROOT@` placeholder in the function should ideally be replaced at build time with the value defined in the [`properties.sh`] file of the `termux-packages` repo.
2. The `@TERMUX_CORE__BASH__TERMUX_SCOPED_ENV_VARIABLE@` placeholder can be added to calling script source files as a commented line like `##### @TERMUX_CORE__BASH__TERMUX_SCOPED_ENV_VARIABLE@ to be replaced at build time. #####` and then during build time, the [`termux-replace-termux-core-src-scripts`] script in the `termux-packages` repo can be executed and passed the paths to the script source files so that it replaces the placeholders with actual source content of the [`termux_core__bash__termux_scoped_env_variable()`] function. The required variables listed in the `termux-replace-termux-core-src-scripts` script help must be exported when executing it. For example, the `termux-core` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-core/build.sh) file used for compiling the package, passes the build time values for the required variables and the `$TERMUX_PACKAGES__REPO_DIR` environment variable for the root directory of the local `termux-packages` repo directory, which is required to find the path of the `termux-replace-termux-core-src-scripts` script on the local filesystem so that it can be executed. The [`Makefile`](https://github.com/termux/termux-core-package/blob/v0.1.0/Makefile) of the `termux-core` package then reads the variables and executes the `termux-replace-termux-core-src-scripts` script on the [`termux-scoped-env-variable.bash.in`] file that contains the `@TERMUX_CORE__BASH__TERMUX_SCOPED_ENV_VARIABLE@` placeholder.

&nbsp;



#### Help

```
termux-scoped-env-variable.bash can be used to get/set/unset variable
names and values for `TERMUX*__` and other scoped environment
variables exported by different Termux runtime components, with
support for fallback values and validation of values.


Usage:
  termux-scoped-env-variable.bash <command> <args...>


Available commands:
    get-name                     Get Termux scoped variable name.
    get-value                    Get Termux scoped variable value.
    set-value                    Set Termux scoped variable value.
    unset-value                  Unset Termux scoped variable value.



get-name:
  termux-scoped-env-variable.bash get-name \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name>



get-value:
  termux-scoped-env-variable.bash get-value \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name> \
    <extended_validator> [<default_values...>]



set-value:
  termux-scoped-env-variable.bash set-value \
    <scoped_var_scope_mode> <scoped_var_sub_name> \
    <value_to_set>



unset-value:
  termux-scoped-env-variable.bash unset-value \
    <scoped_var_scope_mode> <scoped_var_sub_name>
```

Available arguments:
- `output_mode`: The output mode for the command.
  If set to a valid environment variable name (like `TERMUX__VAR`) or
  a bash array variable (like `TERMUX__ARRAY[0]`), then output will be
  set to the variable.
  Otherwise, it must be set to `>` so that output is printed to
  `stdout`, or set to `-` so that output is not printed at all and
  only exit code is returned.
- `scoped_var_scope_mode`: The scope mode of the scoped environment
  variable name to get/set/unset.
- `scoped_var_sub_name`: The sub name of the scoped environment
  variable name to get/set/unset after the scope name.
- `extended_validator`: The extended validator for whether a
  particular variable value is valid and should be returned or not.
- `default_values`: One or more fallback default values. This may
  optionally be a placeholder value that gets replaced during build
  time.
- `value_to_set`: The value to set to the scoped environment variable.



The `get-name` command type returns the name of the Termux scoped
environment variable generated depending on the variable scope and sub
name passed.

The `get-value` command type returns the value for the Termux scoped
environment variable generated depending on the variable scope and sub
name passed. First its value is read from the environment, followed by
reading the optional values passed as `default_values`, and whichever
value first matches the `extended_validator` is returned and if no
value matches, then the call will return with exit code
`81` (`C_EX__NOT_FOUND`). If the `scoped_var_scope_mode` argument is
not set, then the environment variable is not read and only any
default values passed are checked.

The `set-value` command type sets the value of the Termux scoped
environment variable generated to the value passed as `value_to_set`.

The `unset-value` command type unsets the value of the Termux scoped
environment variable generated by running the `unset` command.

The `set-value` and `unset-value` command types are not available if
executing the `termux-scoped-env-variable.bash` script as they will
not have any effect on the calling process environment and are only
available if the script is sourced and the
`termux_core__bash__termux_scoped_env_variable` function is called.



The scoped environment variable name to get/set/unset is generated
based on the `scoped_var_scope_mode` and `scoped_var_sub_name`
arguments.
The `scoped_var_scope_mode` argument must be equal to one of
the following formats:
- `s=<scoped_var_scope_name>`: The variable name is generated as per
  `${scoped_var_scope_name}${scoped_var_sub_name}`. The root scope
  is assumed to be part of the `scoped_var_scope_name` argument and
  Termux root scope is not prefixed to it. This can be used if
  `scoped_var_scope_name` was already generated with the Termux root
  scope or if getting a variable for an external app under a
  different root scope.
- `ss=<scoped_var_sub_scope_name>`: The variable name is generated as per
  `${root_scope_name}${scoped_var_sub_scope_name}${scoped_var_sub_name}`,
  where the `root_scope_name` is the value set in the
  `$TERMUX_ENV__S_ROOT` environment variable, with a fallback to the
  build time placeholder`@TERMUX_ENV__S_ROOT@`, and then to the
  hardcoded root scope value `TERMUX_` for the upstream Termux app.
- `cn=<scoped_var_component_name>`: The variable name is generated as
    per the component name passed from the list below, where the value
    in the bracket defines the sub scope value used for variable name
    generation.
    - `cn=termux`            (`ss="_"`)
    - `cn=termux-app`        (`ss="APP__"`)
    - `cn=termux-api-app`    (`ss="API_APP__"`)
    - `cn=termux-float-app`  (`ss="FLOAT_APP__"`)
    - `cn=termux-gui-app`    (`ss="GUI_APP__"`)
    - `cn=termux-tasker-app` (`ss="TASKER_APP__"`)
    - `cn=termux-widget-app` (`ss="WIDGET_APP__"`)
    - `cn=termux-x11-app`    (`ss="X11_APP__"`)
    - `cn=termux-core`       (`ss="CORE__"`)
    - `cn=termux-exec`       (`ss="EXEC__"`)

Optionally, only the `scoped_var_scope_mode` argument may be
passed with the `scoped_var_sub_name` argument as an empty string.
This can be used for the `get-name` command and a
`ss=<scoped_var_sub_scope_name>` formatted argument to get the
full scope name with the Termux root scope prefixed to the sub scope.



The `extended_validator` argument must be equal to one of the
following formats. The `value` refers to the values that are checked
for a match, like defined by the Termux scoped environment variable
or the `default_values`.
- `?`: Matches `value` if it is set and not empty.
- `*`: Matches `value` if it is set, or defined but empty.
- `r+=<regex>`: Matches `value` if it matches the `<regex>`.
- `r-=<regex>`: Matches `value` if it does not match the `<regex>`.
- `c+=<command>`: Matches `value` if `<command>` passed the `value` as
                  an argument exits with exit code `0`.
- `c-=<command>`: Matches `value` if `<command>` passed the `value` as
                  an argument does not exit with exit code `0`.

For the `r+=<regex>` and `r-=<regex>` formats, the leading `r+=` or
`r-=` characters are removed from `extended_validator` and
remaining part is matched against the `value` with the bash `=~` if
conditional. The `<regex>` argument must be a valid POSIX extended
regular expression (`REG_EXTENDED`).
For example, the argument `r+='^((/)|((/[^/]+)+))$'` would match a
`value` that is a valid absolute path (including rootfs `/`).
For more info, check:
- https://www.gnu.org/software/bash/manual/html_node/Conditional-Constructs.html#index-_005b_005b
- https://www.man7.org/linux/man-pages/man3/regex.3.html
- https://www.man7.org/linux/man-pages/man7/regex.7.html

For the `c+=<command>` and `c-=<command>` formats, the leading `c+=`
or `c-=` characters are removed from `extended_validator` and
remaining part is passed as an argument to `<command>` and executed.
The `<command>` string must be set to an executable in `$PATH`,
or a path to an executable, or a function name in the current
execution environment, optionally followed with arguments.
This is checked with the `command -v` command and if that fails, then
the first argument is checked to see if its an absolute or relative
path to an executable (`+x`) file. For more info, check:
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/command.html

The `value` is passed as the last argument to `<command>`, or the
first if `<command>` itself does not have any argument.
Any arguments in `<command>` itself should be quoted with single or
double quotes if they have whitespace (IFS) characters, like
`c+='value_validator "some arg"`.
The `stdout` and `stderr` streams are not redirected to
`/dev/null` when executing the command, and must be manually done
in the function if required or the quiet `-q` options be used for
commands like `grep`, otherwise the `stdout` of the command will
pollute the normal result output for the matched value if it is to be
printed to `stdout`.
For example, the arguments `c+='value_validator'` and
`c+='value_validator some_arg'` would match the `value` if the
commands `value_validator "value"` and
`value_validator some_arg "value"` respectively would exit with exit
code `0`.
Note that if `termux-scoped-env-variable.bash` is being
executed, instead of the 
`termux_core__bash__termux_scoped_env_variable` local function
being called, any function in the `<command>` argument must first be
exported with the `export -f <function>` command to be defined in
the executed child process, assuming if executing from a `bash` shell.
For more info, check:
- https://www.gnu.org/software/bash/manual/bash.html#Shell-Functions
- https://www.gnu.org/software/bash/manual/bash.html#index-export



**See also:**
- [`termux-scoped-env-variable.bash.in`]
- [`termux_core__bash__termux_scoped_env_variable()`]

&nbsp;



#### Examples

The following examples assume that `termux-scoped-env-variable.bash` script has been sourced in the current shell (so that/or) the `termux_core__bash__termux_scoped_env_variable` function is defined and can be called. If `termux-scoped-env-variable.bash` needs to be executed instead, then replace `termux_core__bash__termux_scoped_env_variable` in the following examples with `termux-scoped-env-variable`/`termux-scoped-env-variable.bash` and the second argument for `output_mode` with the value `>` so that the output is printed to `stdout`.

```shell
source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then source "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

source_file_from_path "termux-scoped-env-variable.bash" || exit $?
```

&nbsp;



##### `TERMUX__ROOTFS`

Get `$TERMUX__ROOTFS` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files` for the upstream Termux app. If no value matches a valid absolute path (including rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX__ROOTFS` variable.

```shell
termux_core__bash__termux_scoped_env_variable get-value "TERMUX__ROOTFS" cn="termux" "ROOTFS" r+='^((/)|((/[^/]+)+))$' "@TERMUX__ROOTFS@" "/data/data/com.termux/files" || return $?
```

##### `TERMUX__HOME`

Get `$TERMUX__HOME` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files/home` for the upstream Termux app. If no value matches a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX__HOME` variable.

```shell
termux_core__bash__termux_scoped_env_variable get-value "TERMUX__HOME" cn="termux" "HOME" r+='^(/[^/]+)+$' "@TERMUX__HOME@" "/data/data/com.termux/files/home" || return $?
```

##### `TERMUX__PREFIX`

Get `$TERMUX__PREFIX` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files/usr` for the upstream Termux app. If no value matches a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX__PREFIX` variable.

```shell
termux_core__bash__termux_scoped_env_variable get-value "TERMUX__PREFIX" cn="termux" "PREFIX" r+='^(/[^/]+)+$' "@TERMUX__PREFIX@" "/data/data/com.termux/files/usr" || return $?
```

Get `$TERMUX__PREFIX` variable name by generating it from the `$TERMUX_ENV__S_ROOT` environment variable, with a fallback to the build time placeholder and then to the hardcoded root scope value `TERMUX_` for the upstream Termux app. The output will be set to the `$TERMUX__PREFIX___N` variable.

```shell
termux_core__bash__termux_scoped_env_variable get-name "TERMUX__PREFIX___N" cn="termux" "PREFIX" || return $?
```

&nbsp;



##### `TERMUX__PACKAGE_NAME`

*`TERMUX__PACKAGE_NAME` refers to the package name for the main Termux app or its plugin app that the current shell is running in.*

Get `$TERMUX__PACKAGE_NAME` variable value by checking its environment variable. If no value matches a valid app package name or an empty string (in case variable not exported on older Termux app versions), then call will return with exit code `81` (`C_EX__NOT_FOUND`). If a valid value is found, it is set to the `$TERMUX__PACKAGE_NAME` variable.

The regex does not validate the max length `255` as `bash` `=~` extended regex (`REG_EXTENDED`) conditional does not support lookaround. Check `TERMUX_REGEX__APP_PACKAGE_NAME` in [`properties.sh`] file for more info on what is a valid app package name.

```shell
termux_core__bash__termux_scoped_env_variable get-value "TERMUX__PACKAGE_NAME" cn="termux" "PACKAGE_NAME" r+="^(()|([a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+))$" || return $?
```

If max length `255` needs to be validated as well, the `c+=` format can be used to pass the custom function `is_valid_android_app_package_name()` below, which uses `grep -P` perl regular expressions positive lookahead with `(?=.{0,255}$)` to validate the length. The quiet `-q` option is also passed to `grep` so that it does not log to `stdout` as that will pollute the normal result output for the matched value if it is to be printed to `stdout`. Note that if `termux-scoped-env-variable.bash` is being executed, instead of the `termux_core__bash__termux_scoped_env_variable` local function being called, the `is_valid_android_app_package_name()` function must first be exported with the [`export -f <function>`](https://www.gnu.org/software/bash/manual/bash.html#index-export) command to be defined in the executed child process, assuming if executing from a `bash` shell.

```shell
is_valid_android_app_package_name() { printf "%s\n" "${1:-}" | grep -qP '^(?=.{0,255}$)(()|([a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+))$'; }

termux_core__bash__termux_scoped_env_variable get-value "TERMUX__PACKAGE_NAME" cn="termux" "PACKAGE_NAME" c+='is_valid_android_app_package_name' || return $?
```

If `grep -P` perl regular expressions is not available, a manual length can also be done without any external calls to `grep`.

```shell
is_valid_android_app_package_name() { [[ "${1:-}" =~ ^(()|([a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+))$ ]] && [ "${#1}" -le 255 ]; }

termux_core__bash__termux_scoped_env_variable get-value "TERMUX__PACKAGE_NAME" cn="termux" "PACKAGE_NAME" c+='is_valid_android_app_package_name' || return $?
```

&nbsp;



##### `TERMUX_APP__PACKAGE_NAME`

*`TERMUX_APP__PACKAGE_NAME` refers to the package name for the main Termux app.*

Get `$TERMUX_APP__PACKAGE_NAME` variable value by first checking its environment variable, with a fallback to the build time placeholder and then to the hardcoded package name value `com.termux` for the upstream Termux app. If the environment variable or build time placeholder value do not match a valid app package name, then the hardcoded fallback value is returned. The output will be set to the `$TERMUX_APP__PACKAGE_NAME` variable.

The regex does not validate the max length `255` as `bash` `=~` extended regex (`REG_EXTENDED`) conditional does not support lookaround. If max length `255` needs to be validated as well, use a custom function as detailed for the [`TERMUX__PACKAGE_NAME`](#TERMUX__PACKAGE_NAME) variable. Check `TERMUX_REGEX__APP_PACKAGE_NAME` in [`properties.sh`] file for more info on what is a valid app package name.

```shell
termux_core__bash__termux_scoped_env_variable get-value "TERMUX_APP__PACKAGE_NAME" cn="termux-app" "PACKAGE_NAME" r+="^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$" "@TERMUX_APP__PACKAGE_NAME@" "com.termux" || return $?
```

&nbsp;



##### `TERMUX__USER_ID`

Get `$TERMUX__USER_ID` variable value by first checking its environment variable, with a fallback to the hardcoded user id value `0` for the default user id for Android. If the environment variable does not match a valid user id, then the hardcoded fallback value is returned. The output will be printed to `stdout`.

```shell
termux_core__bash__termux_scoped_env_variable get-value '>' cn="termux" "USER_ID" r+="^(([0-9])|([1-9][0-9]{0,2}))$" 0 || return $?
```

&nbsp;



##### `TERMUX_EXEC__PROC_SELF_EXE`

Get `$TERMUX_EXEC__PROC_SELF_EXE` variable value by checking its environment variable. If the environment variable is unset (in case variable is not exported on older `termux-exec` versions or if `system_linker_exec` is not enabled), then an empty string will be returned. If the environment variable is set, but does not match a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_EXEC__PROC_SELF_EXE` variable.

```shell
termux_core__bash__termux_scoped_env_variable get-value "TERMUX_EXEC__PROC_SELF_EXE" cn="termux-exec" "PROC_SELF_EXE" r+='^(()|((/[^/]+)+))$' "" || return $?
```

## &nbsp;

&nbsp;



### sh

`termux-scoped-env-variable.sh` is a [POSIX shell](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html) implementation for `termux-scoped-env-variable` and exists in Termux default `bin` directory in `$PATH` in Termux environments. However, the shell must support the [`local`](https://www.shellcheck.net/wiki/SC3043) keyword for variables, which normally is supported.

The source file for the `termux-scoped-env-variable.sh` script is the [`termux-scoped-env-variable.sh.in`] file in the `termux-core-package` repo, and it internally uses the [`termux_core__sh__termux_scoped_env_variable()`] function defined in the `termux-packages` repo which is replaced at build time with the `@TERMUX_CORE__SH__TERMUX_SCOPED_ENV_VARIABLE@` placeholder, see below for how.

`termux-scoped-env-variable.sh` script can be used at runtime by:

1. Sourcing the `termux-scoped-env-variable.sh` file in `$PATH` (with [`.`](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_18_01) command) by running the following commands and then calling the `termux_core__sh__termux_scoped_env_variable()` function that should be defined in the current shell environment.
For the `get-*` commands, the name of an environment variable in which the output should be set can be passed as the `output_mode` argument, instead of having to call the function in a subshell and capturing its `stdout` for the output.  
The `command -v` command is used to find the location of the script file instead of directly using the `.`/`source` command to prevent sourcing of a (malicious) file in the current working directory with the same name instead of the one in `$PATH`. A separate function is used to source so that arguments passed to calling script/function are not passed to the sourced script. Passing `--sourcing-script` argument is necessary if sourcing from a `sh` shell script so that script `main` function is not run as there is no POSIX way to detect if current script is being sourced from another script. Replace `exit` with `return` if running inside a function.
    ```shell
    source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then . "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

    source_file_from_path "termux-scoped-env-variable.sh" --sourcing-script || exit $?

    termux_core__sh__termux_scoped_env_variable <command> output_variable <arguments...> || exit $?
    ```

2. Executing `termux-scoped-env-variable.sh` directly (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and capturing its `stdout`/`stderr` for the output and errors.
    ```shell
    output="$(termux-scoped-env-variable.sh <command> '>' <arguments...>)" || exit $?
    ```

Sourcing should be preferred, especially if multiple variable names or values need to be read. Sourcing the `termux-scoped-env-variable.sh` script once would be a single external call, and then any calls to the `termux_core__sh__termux_scoped_env_variable()` function would all be local calls, so would be faster than if `termux-scoped-env-variable.sh` script is executed multiple times with external calls for each variable. If efficiency is not an issue, then it doesn't matter much if script is executed or sourced, other than being able to pass a variable name as `output_mode`.

&nbsp;

If efficiency is an issue for time critical calling scripts that require low latency, the `termux_core__sh__termux_scoped_env_variable()` function can be imported into calling script source files so that external calls to source or execute `termux-scoped-env-variable.sh` is not required and the function can be called locally as it will already be defined. This can be done at build time by:

1. The [`termux_core__sh__termux_scoped_env_variable()`] function can be copied into calling script source files manually. It will have to kept in sync with upstream whenever any changes or fixes are done. The `@TERMUX_ENV__S_ROOT@` placeholder in the function should ideally be replaced at build time with the value defined in the [`properties.sh`] file of the `termux-packages` repo.
2. The `@TERMUX_CORE__SH__TERMUX_SCOPED_ENV_VARIABLE@` placeholder can be added to calling script source files as a commented line like `##### @TERMUX_CORE__SH__TERMUX_SCOPED_ENV_VARIABLE@ to be replaced at build time. #####` and then during build time, the [`termux-replace-termux-core-src-scripts`] script in the `termux-packages` repo can be executed and passed the paths to the script source files so that it replaces the placeholders with actual source content of the [`termux_core__sh__termux_scoped_env_variable()`] function. The required variables listed in the `termux-replace-termux-core-src-scripts` script help must be exported when executing it. For example, the `termux-core` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-core/build.sh) file used for compiling the package, passes the build time values for the required variables and the `$TERMUX_PACKAGES__REPO_DIR` environment variable for the root directory of the local `termux-packages` repo directory, which is required to find the path of the `termux-replace-termux-core-src-scripts` script on the local filesystem so that it can be executed. The [`Makefile`](https://github.com/termux/termux-core-package/blob/v0.1.0/Makefile) of the `termux-core` package then reads the variables and executes the `termux-replace-termux-core-src-scripts` script on the [`termux-scoped-env-variable.sh.in`] file that contains the `@TERMUX_CORE__SH__TERMUX_SCOPED_ENV_VARIABLE@` placeholder.

&nbsp;



#### Help

```
termux-scoped-env-variable.sh can be used to get/set/unset variable
names and values for `TERMUX*__` and other scoped environment
variables exported by different Termux runtime components, with
support for fallback values and validation of values.


Usage:
  termux-scoped-env-variable.sh <command> <args...>


Available commands:
    get-name                     Get Termux scoped variable name.
    get-value                    Get Termux scoped variable value.
    set-value                    Set Termux scoped variable value.
    unset-value                  Unset Termux scoped variable value.



get-name:
  termux-scoped-env-variable.sh get-name \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name>



get-value:
  termux-scoped-env-variable.sh get-value \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name> \
    <posix_validator> [<default_values...>]



set-value:
  termux-scoped-env-variable.sh set-value \
    <scoped_var_scope_mode> <scoped_var_sub_name> \
    <value_to_set>



unset-value:
  termux-scoped-env-variable.sh unset-value \
    <scoped_var_scope_mode> <scoped_var_sub_name>
```

Available arguments:
- `output_mode`: The output mode for the command.
  If set to a valid environment variable name (like `TERMUX__VAR`),
  then output will be set to the variable.
  Otherwise, it must be set to `>` so that output is printed to
  `stdout`, or set to `-` so that output is not printed at all and
  only exit code is returned.
- `scoped_var_scope_mode`: The scope mode of the scoped environment
  variable name to get/set/unset.
- `scoped_var_sub_name`: The sub name of the scoped environment
  variable name to get/set/unset after the scope name.
- `posix_validator`: The posix validator for whether a
  particular variable value is valid and should be returned or not.
- `default_values`: One or more fallback default values. This may
  optionally be a placeholder value that gets replaced during build
  time.
- `value_to_set`: The value to set to the scoped environment variable.



The `get-name` command type returns the name of the Termux scoped
environment variable generated depending on the variable scope and sub
name passed.

The `get-value` command type returns the value for the Termux scoped
environment variable generated depending on the variable scope and sub
name passed. First its value is read from the environment, followed by
reading the optional values passed as `default_values`, and whichever
value first matches the `posix_validator` is returned and if no
value matches, then the call will return with exit code
`81` (`C_EX__NOT_FOUND`). If the `scoped_var_scope_mode` argument is
not set, then the environment variable is not read and only any
default values passed are checked.

The `set-value` command type sets the value of the Termux scoped
environment variable generated to the value passed as `value_to_set`.

The `unset-value` command type unsets the value of the Termux scoped
environment variable generated by running the `unset` command.

The `set-value` and `unset-value` command types are not available if
executing the `termux-scoped-env-variable.sh` script as they will
not have any effect on the calling process environment and are only
available if the script is sourced and the
`termux_core__sh__termux_scoped_env_variable` function is called.



The scoped environment variable name to get/set/unset is generated
based on the `scoped_var_scope_mode` and `scoped_var_sub_name`
arguments.
The `scoped_var_scope_mode` argument must be equal to one of
the following formats:
- `s=<scoped_var_scope_name>`: The variable name is generated as per
  `${scoped_var_scope_name}${scoped_var_sub_name}`. The root scope
  is assumed to be part of the `scoped_var_scope_name` argument and
  Termux root scope is not prefixed to it. This can be used if
  `scoped_var_scope_name` was already generated with the Termux root
  scope or if getting a variable for an external app under a
  different root scope.
- `ss=<scoped_var_sub_scope_name>`: The variable name is generated as per
  `${root_scope_name}${scoped_var_sub_scope_name}${scoped_var_sub_name}`,
  where the `root_scope_name` is the value set in the
  `$TERMUX_ENV__S_ROOT` environment variable, with a fallback to the
  build time placeholder`@TERMUX_ENV__S_ROOT@`, and then to the
  hardcoded root scope value `TERMUX_` for the upstream Termux app.
- `cn=<scoped_var_component_name>`: The variable name is generated as
    per the component name passed from the list below, where the value
    in the bracket defines the sub scope value used for variable name
    generation.
    - `cn=termux`            (`ss="_"`)
    - `cn=termux-app`        (`ss="APP__"`)
    - `cn=termux-api-app`    (`ss="API_APP__"`)
    - `cn=termux-float-app`  (`ss="FLOAT_APP__"`)
    - `cn=termux-gui-app`    (`ss="GUI_APP__"`)
    - `cn=termux-tasker-app` (`ss="TASKER_APP__"`)
    - `cn=termux-widget-app` (`ss="WIDGET_APP__"`)
    - `cn=termux-x11-app`    (`ss="X11_APP__"`)
    - `cn=termux-core`       (`ss="CORE__"`)
    - `cn=termux-exec`       (`ss="EXEC__"`)

Optionally, only the `scoped_var_scope_mode` argument may be
passed with the `scoped_var_sub_name` argument as an empty string.
This can be used for the `get-name` command and a
`ss=<scoped_var_sub_scope_name>` formatted argument to get the
full scope name with the Termux root scope prefixed to the sub scope.



The `posix_validator` argument must be equal to one of the
following formats. The `value` refers to the values that are checked
for a match, like defined by the Termux scoped environment variable
or the `default_values`.
- `?`: Matches `value` if it is set and not empty.
- `*`: Matches `value` if it is set, or defined but empty.
- `p+=<pattern>`: Matches `value` if it matches the `<pattern>`.
- `p-=<pattern>`: Matches `value` if it does not match the `<pattern>`.
- `c+=<command>`: Matches `value` if `<command>` passed the `value` as
                  an argument exits with exit code `0`.
- `c-=<command>`: Matches `value` if `<command>` passed the `value` as
                  an argument does not exit with exit code `0`.

For the `p+=pattern` and `p-=pattern` formats, the leading `p+=` or
`p-=` characters are removed from `posix_validator`
and remaining part is used as is as the pattern for a shell `case`
construct that is dynamically generated using the `eval` command to
match against the `value`. Since `eval` command is used, **DO NOT**
pass untrusted code as `posix_validator` and only a valid
`case` construct pattern must be passed. For example, the argument
`p+='/*'` would match a `value` that is a valid absolute path
(including rootfs `/`). For more info, check:
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_09_04_05
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_13


For the `c+=<command>` and `c-=<command>` formats, the leading `c+=`
or `c-=` characters are removed from `posix_validator` and
remaining part is passed as an argument to `<command>` and executed.
The `<command>` string must be set to an executable in `$PATH`,
or a path to an executable, or a function name in the current
execution environment, optionally followed with arguments.
This is checked with the `command -v` command and if that fails, then
the first argument is checked to see if its an absolute or relative
path to an executable (`+x`) file. For more info, check:
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/command.html

The `value` is passed as the last argument to `<command>`, or the
first if `<command>` itself does not have any argument.
Arguments in `<command>` should be separated with a space ` `
character but themselves must not contain any whitespace (IFS)
characters as that will result in them splitting into
multiple arguments, since `<command>` string is executed without
quotes like `$command`. Any quotes surrounding the arguments will be
treated literally.
The `stdout` and `stderr` streams are not redirected to
`/dev/null` when executing the command, and must be manually done
in the function if required or the quiet `-q` options be used for
commands like `grep`, otherwise the `stdout` of the command will
pollute the normal result output for the matched value if it is to be
printed to `stdout`.
For example, the arguments `c+='value_validator'` and
`c+='value_validator some_arg'` would match the `value` if the
commands `value_validator "value"` and
`value_validator some_arg "value"` respectively would exit with exit
code `0`.
Note that if `termux-scoped-env-variable.sh` is being
executed, instead of the
`termux_core__sh__termux_scoped_env_variable` local function
being called, any function in the `<command>` argument will not be
defined in the executed child process, as POSIX shells do not
support exporting functions, like `bash` does with the
`export -f <function>` command. If a function needs to be passed,
the `termux_core__sh__termux_scoped_env_variable` function should
be sourced and executed in the same shell environment/process as
the validator function.
For more info, check:
- https://www.gnu.org/software/bash/manual/bash.html#Shell-Functions
- https://www.gnu.org/software/bash/manual/bash.html#index-export



**See also:**
- [`termux-scoped-env-variable.sh.in`]
- [`termux_core__sh__termux_scoped_env_variable()`]

&nbsp;



#### Examples

The following examples assume that `termux-scoped-env-variable.sh` script has been sourced in the current shell (so that/or) the `termux_core__sh__termux_scoped_env_variable` function is defined and can be called. If `termux-scoped-env-variable.sh` needs to be executed instead, then replace `termux_core__sh__termux_scoped_env_variable` in the following examples with `termux-scoped-env-variable.sh` and the second argument for `output_mode` with the value `>` so that the output is printed to `stdout`.

```shell
source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then . "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

source_file_from_path "termux-scoped-env-variable.sh" --sourcing-script || exit $?
```

&nbsp;



##### `TERMUX__ROOTFS`

Get `$TERMUX__ROOTFS` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files` for the upstream Termux app. If no value matches a valid absolute path (including rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX__ROOTFS` variable.

```shell
termux_core__sh__termux_scoped_env_variable get-value "TERMUX__ROOTFS" ss="_" "ROOTFS" p+='/|/*[!/]' "@TERMUX__ROOTFS@" "/data/data/com.termux/files" || return $?
```

##### `TERMUX__HOME`

Get `$TERMUX__HOME` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files/home` for the upstream Termux app. If no value matches a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX__HOME` variable.

```shell
termux_core__sh__termux_scoped_env_variable get-value "TERMUX__HOME" ss="_" "HOME" p+='/*[!/]' "@TERMUX__HOME@" "/data/data/com.termux/files/home" || return $?
```

##### `TERMUX__PREFIX`

Get `$TERMUX__PREFIX` variable value by first checking environment variable, with a fallback to the build time placeholder and then to the hardcoded path value `/data/data/com.termux/files/usr` for the upstream Termux app. If no value matches a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX__PREFIX` variable.

```shell
termux_core__sh__termux_scoped_env_variable get-value "TERMUX__PREFIX" ss="_" "PREFIX" p+='/*[!/]' "@TERMUX__PREFIX@" "/data/data/com.termux/files/usr" || return $?
```

Get `$TERMUX__PREFIX` variable name by generating it from the `$TERMUX_ENV__S_ROOT` environment variable, with a fallback to the build time placeholder and then to the hardcoded root scope value `TERMUX_` for the upstream Termux app . The output will be set to the `$TERMUX__PREFIX___N` variable.

```shell
termux_core__sh__termux_scoped_env_variable get-name "TERMUX__PREFIX___N" ss="_" "PREFIX" || return $?
```

&nbsp;



##### `TERMUX__PACKAGE_NAME`

*`TERMUX__PACKAGE_NAME` refers to the package name for the main Termux app or its plugin app that the current shell is running in.*

Get `$TERMUX__PACKAGE_NAME` variable value by checking its environment variable. If the environment variable matches a string that contains a character not in the range `a-zA-Z0-9_.` and is not an empty string (in case variable not exported on older Termux app versions), then call will return with exit code `81` (`C_EX__NOT_FOUND`). If a valid value is found, it is set to the `$TERMUX__PACKAGE_NAME` variable.

The valid regex to match an android app package name is `^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$`, but this cannot be implemented in a single `case` construct pattern, and neither can the `case` construct pattern validate the max length `255`. Check `TERMUX_REGEX__APP_PACKAGE_NAME` in [`properties.sh`] file for more info on what is a valid app package name.

```shell
termux_core__sh__termux_scoped_env_variable get-value "TERMUX__PACKAGE_NAME" ss="_" "PACKAGE_NAME" p-="*[!a-zA-Z0-9_.]*" || return $?
```

If full validation of android app package name is required, including max length `255`, at the cost of external calls to `grep`, the `c+=` format can be used to pass the custom function `is_valid_android_app_package_name()` below, which uses `grep -E` extended regular expressions to validate the package name characters and has a manual length check. The quiet `-q` option is also passed to `grep` so that it does not log to `stdout` as that will pollute the normal result output for the matched value if it is to be printed to `stdout`. Note that if `termux-scoped-env-variable.sh` is being executed, instead of the `termux_core__sh__termux_scoped_env_variable` local function being called, the `is_valid_android_app_package_name()` function will not be defined in the executed child process, as POSIX shells do not support exporting functions, like `bash` does with the [`export -f <function>`](https://www.gnu.org/software/bash/manual/bash.html#index-export) command.

```shell
is_valid_android_app_package_name() { printf "%s\n" "${1:-}" | grep -qE '^(()|([a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+))$' && [ "${#1}" -le 255 ]; }

termux_core__sh__termux_scoped_env_variable get-value "TERMUX__PACKAGE_NAME" ss="_" "PACKAGE_NAME" c+='is_valid_android_app_package_name' || return $?
```

&nbsp;



##### `TERMUX_APP__PACKAGE_NAME`

*`TERMUX_APP__PACKAGE_NAME` refers to the package name for the main Termux app.*

Get `$TERMUX_APP__PACKAGE_NAME` variable value by first checking its environment variable, with a fallback to the build time placeholder and then to the hardcoded package name value `com.termux` for the upstream Termux app. If the environment variable or build time placeholder value matches a string that is empty or contain a character not in the range `a-zA-Z0-9_.`, then the hardcoded fallback value is returned. The output will be set to the `$TERMUX_APP__PACKAGE_NAME` variable.

The valid regex to match an android app package name is `^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$`, but this cannot be implemented in a single `case` construct pattern, and neither can the `case` construct pattern validate the max length `255`. If full validation of android app package name is required, including max length `255`, use a custom function as detailed for the [`TERMUX__PACKAGE_NAME`](#TERMUX__PACKAGE_NAME) variable. Check `TERMUX_REGEX__APP_PACKAGE_NAME` in [`properties.sh`] file for more info on what is a valid app package name.

```shell
termux_core__sh__termux_scoped_env_variable get-value "TERMUX_APP__PACKAGE_NAME" cn="termux-app" "PACKAGE_NAME" p-="''|*[!a-zA-Z0-9_.]*" "@TERMUX_APP__PACKAGE_NAME@" "com.termux" || return $?
```

&nbsp;



##### `TERMUX__USER_ID`

Get `$TERMUX__USER_ID` variable value by first checking its environment variable, with a fallback to the hardcoded user id value `0` for the default user id for Android. If the environment variable does not match a valid user id, then the hardcoded fallback value is returned. The output will be printed to `stdout`.

```shell
termux_core__sh__termux_scoped_env_variable get-value '>' ss="_" "USER_ID" p-="''|*[!0-9]*|0[0-9]*|????*" 0 || return $?
```

&nbsp;



##### `TERMUX_EXEC__PROC_SELF_EXE`

Get `$TERMUX_EXEC__PROC_SELF_EXE` variable value by checking its environment variable. If the environment variable is unset (in case variable is not exported on older `termux-exec` versions or if `system_linker_exec` is not enabled), then an empty string will be returned. If the environment variable is set, but does not match a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_EXEC__PROC_SELF_EXE` variable.

```shell
termux_core__sh__termux_scoped_env_variable get-value "TERMUX_EXEC__PROC_SELF_EXE" cn="termux-exec" "PROC_SELF_EXE" p+="''|/*[!/]" || return $?
```

## &nbsp;

&nbsp;



### c/c++

Currently, a sample implementation is not provided for c/c++ for reading `TERMUX*__` scoped environment variables. However. the [`termux-exec`](https://github.com/termux/termux-exec) package can be used as a reference to implement it. For example, the `$TERMUX__PREFIX` variable is read with the following way.

1. The `termux-exec` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-exec/build.sh) file used for compiling the package, passes the build time values for the `$TERMUX_ENV__S_TERMUX` and `$TERMUX__PREFIX` variables to `make` via the `$TERMUX_PKG_EXTRA_MAKE_ARGS` variable.

2. The `Makefile` [reads](https://github.com/termux/termux-exec/blob/v2.0.0/Makefile#L1) the `make` variables, with fallback default values in case not using the `termux-packages` build infrastructure to compile the package, and then the `Makefile` [passes](https://github.com/termux/termux-exec/blob/v2.0.0/Makefile#L97) the `$TERMUX_ENV__S_TERMUX` and `$TERMUX__PREFIX` values to the compiler pre-processor as macros.

3. The `termux_env.h` file uses the `TERMUX_ENV__S_TERMUX` macro value to [create](https://github.com/termux/termux-exec/blob/v2.0.0/src/termux/termux_env.h#102) the `ENV__TERMUX__PREFIX` macro value for the actual environment variable name for `$TERMUX__PREFIX`. Since `termux-exec` is compiled with `termux-packages` build infrastructure and is not a 3rd party package, it does not need to use the `$TERMUX_ENV__S_ROOT` environment variable to create the `$TERMUX__PREFIX` environment variable name dynamically at runtime, and can use the build time value instead.

4. The `termux-exec` library at runtime uses the `get_termux_prefix_dir_from_env_or_default()` function ([1](https://github.com/termux/termux-exec/blob/v2.0.0/src/termux/termux_files.h#197), [2](https://github.com/termux/termux-exec/blob/v2.0.0/src/termux/termux_files.c#L230)) in `termux_files.h` and implemented by `termux_files.c` to get the Termux prefix directory. It first calls the `get_termux_prefix_dir_from_env()` function ([1](https://github.com/termux/termux-exec/blob/v2.0.0/src/termux/termux_files.h#184), [2](https://github.com/termux/termux-exec/blob/v2.0.0/src/termux/termux_files.c#L224)) to read the value of the `$TERMUX__PREFIX` environment variable that was created at build time, and if the environment variable is not set or is invalid, the `TERMUX__PREFIX` build time macro/default value passed by the `Makefile` is used. The environment variable or built time value must be an absolute path that is [`< TERMUX__PREFIX_DIR_MAX_LEN (90)`](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#file-path-limits), otherwise it is not used.

---

&nbsp;





[`properties.sh`]: https://github.com/termux/termux-packages/blob/master/scripts/properties.sh
[`termux-scoped-env-variable.bash.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-scoped-env-variable.bash.in
[`termux-scoped-env-variable.sh.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-scoped-env-variable.sh.in
[`termux_core__bash__termux_scoped_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__bash__termux_scoped_env_variable
[`termux_core__sh__termux_scoped_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__sh__termux_scoped_env_variable
[`termux-replace-termux-core-src-scripts`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/build/scripts/termux-replace-termux-core-src-scripts
