---
page_ref: "@ARK_PROJECT__VARIANT@/termux/termux-core-package/docs/@ARK_DOC__VERSION@/usage/utils/termux-apps-info-app-version-name.md"
---

# `termux-apps-info-app-version-name` Utils Docs

The `termux-apps-info-app-version-name` scripts/codes provide a way, if running in a Termux app, to get/unset variable values of the app version name environment variables `*_APP__APP_VERSION_NAME` for Termux app `TERMUX_APP__`, its plugin apps `TERMUX_*_APP__` and external apps `*_APP__` app scoped that exist in the `termux-apps-info.env` file, with support for validation of values.

This is only supported for Termux app version `>= 0.119.0` and plugin app versions using its updated internal [`TermuxShellEnvironment`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellEnvironment.java) class changes.

### Contents

- [Rationale and Design](#rationale-and-design)
- [Implementations](#implementations)

---

&nbsp;





## Rationale and Design

*If you do not know what `termux-apps-info-env-variable` is and why it exists, check the [Rationale](termux-apps-info-env-variable.md#rationale-and-design) section of [`termux-apps-info-env-variable` Utils Docs](termux-apps-info-env-variable.md).*

*If you do not know what `termux-scoped-env-variable` is and why it exists, or what `TERMUX*__` scoped environment variables means, check the [Rationale](termux-scoped-env-variable.md#rationale-and-design) and [Scoped Variables](termux-scoped-env-variable.md#scoped-variables) sections of [`termux-scoped-env-variable` Utils Docs](termux-scoped-env-variable.md).*

`termux-apps-info-app-version-name` exists to solve the following issues in addition to the ones solved by `termux-apps-info-env-variable` and `termux-scoped-env-variable`.

1. The primary reason is to provide a wrapper around `termux-apps-info-env-variable` which allows users to easily get Termux app version names, like for Termux app version name (`$TERMUX_APP__APP_VERSION_NAME`) and Termux:API app version name (`$TERMUX_API_APP__APP_VERSION_NAME`) by just passing their (component) name without any `--skip-sourcing*` flags or posix/extended validator arguments, and `termux-apps-info-app-version-name` will automatically handle sourcing and validation. Additionally, the call returns with exit code `0` instead of failing with exit code `81` (`C_EX__NOT_FOUND`) if app version name variable is not set, so callers don't need to handle that case either and can just test if output is set or not.

2. The versioning scheme can vary for each Termux app and its install source, and that can be handled internally without any user side changes. If users directly call `termux-apps-info-env-variable` instead, then they will need to keep updating the posix/extended validators passed for validation.

3. The `*_APP__APP_VERSION_NAME` variables are only exported for Termux app version `>= 0.119.0` and `termux-apps-info.env` file wouldn't exist either. However, on older versions the `$TERMUX_VERSION` deprecated variable is still exported for the main Termux app version name and so `termux-apps-info-env-variable` automatically fallbacks to reading `$TERMUX_VERSION` for `cn="termux-app"` scope mode if `$TERMUX_APP__APP_VERSION_NAME` is not set so that users don't have to do that themselves. Note that this may give outdated/wrong values if running in a plugin app like Termux:Float app and Termux app got updated in the background, as `TERMUX_VERSION` would be set to the Termux app version at the time the Termux:Float shell was started, and not the updated version.

If running in Termux app version `< 0.119.0` where `termux-apps-info.env` file does not exist and `*_APP__APP_VERSION_NAME` environment variables are not exported, and any calls to `termux-apps-info-app-version-name` utils will return with the exit code `0` and output variable/stdout will not be set.

---

&nbsp;





## Implementations

The following implementations are currently provided for `termux-apps-info-app-version-name` utils. Implementations for additional languages are planned for future, for now the `termux-apps-info-app-version-name` executable in `$PATH` can be used by them.

- [Executable in `$PATH` (`termux-apps-info-app-version-name`)](#termux-apps-info-app-version-name)
- [`bash` shell (`termux-apps-info-app-version-name.bash`)](#bash)
- [POSIX shells (`termux-apps-info-app-version-name.sh`)](#sh)

&nbsp;



### `termux-apps-info-app-version-name`

`termux-apps-info-app-version-name` exists in Termux default `bin` directory in `$PATH` in Termux environments and can be executed as a command with arguments.

Currently, it is a symlink to [`termux-apps-info-app-version-name.bash`](#bash), check its [Help](#help) section for more info on supported arguments and [Examples](#examples-1) section for additional examples.

**DO NOT** [`source`](https://www.gnu.org/software/bash/manual/bash.html#index-_002e) this file, as underlying implementation may not be guaranteed to be a `bash` syntax. It should be executed (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and its `stdout` captured for the result output.

The `output_mode` argument must be set to `>` so that output is returned in `stdout`, as if a variable name is passed instead, the variable will only be set in the `termux-apps-info-app-version-name` child process and not in the calling parent process. The `>` argument should be quoted with single `'` or double quotes `"` if running in a shell, otherwise it may be considered a [command redirection](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_07).

&nbsp;



#### Examples

##### `TERMUX_APP__APP_VERSION_NAME`

Get `$TERMUX_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name starting with a number, then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be printed to `stdout`.

```shell
termux-apps-info-app-version-name get-value '>' cn="termux-app" || return $?
```

&nbsp;



##### `TERMUX_API_APP__APP_VERSION_NAME`

Get `$TERMUX_API_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name starting with a number, then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be printed to `stdout`.

```shell
termux-apps-info-app-version-name get-value '>' cn="termux-api-app" || return $?
```

## &nbsp;

&nbsp;



### bash

`termux-apps-info-app-version-name.bash` is a [`bash` shell](https://www.gnu.org/software/bash/manual/bash.html) implementation for `termux-apps-info-app-version-name` and exists in Termux default `bin` directory in `$PATH` in Termux environments.

The source file for the `termux-apps-info-app-version-name.bash` script is the [`termux-apps-info-app-version-name.bash.in`] file in the `termux-core-package` repo, and it internally uses the [`termux_core__bash__termux_apps_info_app_version_name()`] function defined in the `termux-packages` repo which is replaced at build time with the `@TERMUX_CORE__BASH__TERMUX_APPS_INFO_APP_VERSION_NAME@` placeholder, see below for how.

`termux-apps-info-app-version-name.bash` script can be used at runtime by:

1. Sourcing the `termux-apps-info-app-version-name.bash` file in `$PATH` (with [`.`](https://www.gnu.org/software/bash/manual/bash.html#index-_002e) or [`source`](https://www.gnu.org/software/bash/manual/bash.html#index-source) command) by running the following commands and then calling the `termux_core__bash__termux_apps_info_app_version_name()` function that should be defined in the current shell environment. The dependency functions like [`termux_core__bash__termux_apps_info_env_variable()`] and [`termux_core__bash__termux_scoped_env_variable()`] will also get automatically defined when sourcing the `termux-apps-info-app-version-name.bash` file and the `termux-apps-info-env-variable.bash` and `termux-scoped-env-variable.bash` files will not need to be sourced.
For the `get-*` commands, the name of an environment variable in which the output should be set can be passed as the `output_mode` argument, instead of having to call the function in a subshell and capturing its `stdout` for the output.  
The `command -v` command is used to find the location of the script file instead of directly using the `.`/`source` command to prevent sourcing of a (malicious) file in the current working directory with the same name instead of the one in `$PATH`. A separate function is used to source so that arguments passed to calling script/function are not passed to the sourced script. Replace `exit` with `return` if running inside a function.
    ```shell
    source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then source "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

    source_file_from_path "termux-apps-info-app-version-name.bash" || exit $?

    termux_core__bash__termux_apps_info_app_version_name <command> output_variable <arguments...> || exit $?
    ```

2. Executing `termux-apps-info-app-version-name.bash` directly (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and capturing its `stdout`/`stderr` for the output and errors.
    ```shell
    output="$(termux-apps-info-app-version-name.bash <command> '>' <arguments...>)" || exit $?
    ```

Sourcing should be preferred, especially if multiple variable names or values need to be read. Sourcing the `termux-apps-info-app-version-name.bash` script once would be a single external call, and then any calls to the `termux_core__bash__termux_apps_info_app_version_name()` function would all be local calls, so would be faster than if `termux-apps-info-app-version-name.bash` script is executed multiple times with external calls for each variable. If efficiency is not an issue, then it doesn't matter much if script is executed or sourced, other than being able to pass a variable name as `output_mode`.

&nbsp;

If efficiency is an issue for time critical calling scripts that require low latency, the `termux_core__bash__termux_apps_info_app_version_name()` function can be imported into calling script source files so that external calls to source or execute `termux-apps-info-app-version-name.bash` is not required and the function can be called locally as it will already be defined. This can be done at build time by:

1. The [`termux_core__bash__termux_apps_info_app_version_name()`] function and its dependency functions like [`termux_core__bash__termux_apps_info_env_variable()`] and [`termux_core__bash__termux_scoped_env_variable()`] can be copied into calling script source files manually. It will have to kept in sync with upstream whenever any changes or fixes are done. The `@TERMUX_ENV__S_ROOT@` placeholder in the function should ideally be replaced at build time with the value defined in the [`properties.sh`] file of the `termux-packages` repo.
2. The `@TERMUX_CORE__BASH__TERMUX_APPS_INFO_APP_VERSION_NAME@` placeholder can be added to calling script source files as a commented line like `##### @TERMUX_CORE__BASH__TERMUX_APPS_INFO_APP_VERSION_NAME@ to be replaced at build time. #####` and then during build time, the [`termux-replace-termux-core-src-scripts`] script in the `termux-packages` repo can be executed and passed the paths to the script source files so that it replaces the placeholders with actual source content of the [`termux_core__bash__termux_apps_info_app_version_name()`] function and its dependency functions like [`termux_core__bash__termux_scoped_env_variable()`]. The required variables listed in the `termux-replace-termux-core-src-scripts` script help must be exported when executing it. For example, the `termux-core` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-core/build.sh) file used for compiling the package, passes the build time values for the required variables and the `$TERMUX_PACKAGES__REPO_DIR` environment variable for the root directory of the local `termux-packages` repo directory, which is required to find the path of the `termux-replace-termux-core-src-scripts` script on the local filesystem so that it can be executed. The [`Makefile`](https://github.com/termux/termux-core-package/blob/v0.1.0/Makefile) of the `termux-core` package then reads the variables and executes the `termux-replace-termux-core-src-scripts` script on the [`termux-apps-info-app-version-name.bash.in`] file that contains the `@TERMUX_CORE__BASH__TERMUX_APPS_INFO_APP_VERSION_NAME@` placeholder.

&nbsp;



#### Help

```
termux-apps-info-app-version-name.bash can be used to get/unset
variable values of the app version name environment variables
`*_APP__APP_VERSION_NAME` for Termux app `TERMUX_APP__`, its plugin
apps `TERMUX_*_APP__` and external apps `*_APP__` app scoped that
exist in the `termux-apps-info.env` file, with support for validation
of values.


Usage:
  termux-apps-info-app-version-name.bash <command> <args...>


Available commands:
    get-value                    Get app version name value from
                                 Termux scoped variable.
    unset-value                  Unset Termux scoped variable value
                                 for app version name.



get-value:
  termux-apps-info-app-version-name.bash get-value [<command_options>] \
    <output_mode> \
    <scoped_var_scope_mode>

Available command options:
  [ --skip-sourcing ]
                     Skip sourcing of `termux-apps-info.env` file
                     before getting value. By default, the
                     '--skip-sourcing-if-cur-app-var' flag is passed
                     to 'termux-apps-info-env-variable.bash' instead.
  [ --extended-validator=<validator> ]
                     The extended validator to pass to
                     'termux-scoped-env-variable.bash' for validation
                     of app version name value. By default, the value
                     must start with a number.



unset-value:
  termux-apps-info-app-version-name.bash unset-value \
    <scoped_var_scope_mode>
```

The `get-value` command type returns the value for the Termux scoped
environment variable generated for the app version name depending on
the variable scope passed and and the sub name as `APP_VERSION_NAME`.
The value is read from the environment if app scope of the variable
to get is the same as the app scope of the current app set in the
`$TERMUX_ENV__S_APP` environment variable, otherwise
`termux-apps-info.env` file is sourced first before reading the value
unless the `--skip-sourcing` flag is passed. If the environment
variable is not set (like in case app is not installed) or is set to a
valid app version name starting with a number (default) or matches the
custom validator passed with the `--extended-validator` argument, then
call will return with exit code `0`. If its set, but not to a valid
app version name, then the call will return with exit code
`81` (`C_EX__NOT_FOUND`).

The `unset-value` command type unsets the value of the Termux scoped
environment variable generated for the app version name by running the
`unset` command.

The `unset-value` command type is not available if executing the
`termux-apps-info-app-version-name.bash` script as that will not have
any effect on the calling process environment and is only available if
the script is sourced and the
`termux_core__bash__termux_apps_info_app_version_name` function is
called.

Check the help of `termux-apps-info-env-variable.sh` and
`termux-scoped-env-variable.sh` commands for info on the arguments for
the `get-value` and `unset-value` command types.

**See also:**
- [`termux-apps-info-app-version-name.bash.in`]
- [`termux_core__bash__termux_apps_info_app_version_name()`]

- [`termux-apps-info-env-variable`]
- [`termux-apps-info-env-variable.bash.in`]
- [`termux_core__bash__termux_apps_info_env_variable()`]
.
- [`termux-scoped-env-variable`]
- [`termux-scoped-env-variable.bash.in`]
- [`termux_core__bash__termux_scoped_env_variable()`]

&nbsp;



#### Examples

The following examples assume that `termux-apps-info-app-version-name.bash` script has been sourced in the current shell (so that/or) the `termux_core__bash__termux_apps_info_app_version_name` function is defined and can be called. If `termux-apps-info-app-version-name.bash` needs to be executed instead, then replace `termux_core__bash__termux_apps_info_app_version_name` in the following examples with `termux-apps-info-app-version-name`/`termux-apps-info-app-version-name.bash` and the second argument for `output_mode` with the value `>` so that the output is printed to `stdout`.

```shell
source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then source "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

source_file_from_path "termux-apps-info-app-version-name.bash" || exit $?
```

&nbsp;



##### `TERMUX_APP__APP_VERSION_NAME`

Get `$TERMUX_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name starting with a number, then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__APP_VERSION_NAME` variable.

```shell
termux_core__bash__termux_apps_info_app_version_name get-value TERMUX_APP__APP_VERSION_NAME cn="termux-app" || return $?
```

&nbsp;



##### `TERMUX_API_APP__APP_VERSION_NAME`

Get `$TERMUX_API_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name starting with a number, then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_API_APP__APP_VERSION_NAME` variable.

```shell
termux_core__bash__termux_apps_info_app_version_name get-value TERMUX_API_APP__APP_VERSION_NAME cn="termux-api-app" || return $?
```

&nbsp;



##### Multiple Variable Read

Unset `$TERMUX_APP__APP_VERSION_NAME` and `$TERMUX_API_APP__APP_VERSION_NAME` variables, then source the `termux-apps-info.env` file to set their latest values into the current environment, then get their variable values without sourcing the file again.

```shell
termux_core__bash__termux_apps_info_app_version_name unset-value cn="termux-app" || return $?
termux_core__bash__termux_apps_info_app_version_name unset-value cn="termux-api-app" || return $?
termux_core__bash__termux_apps_info_env_variable source-env || return $?
termux_core__bash__termux_apps_info_app_version_name get-value --skip-sourcing TERMUX_APP__APP_VERSION_NAME cn="termux-app" || return $?
termux_core__bash__termux_apps_info_app_version_name get-value --skip-sourcing TERMUX_API_APP__APP_VERSION_NAME cn="termux-api-app" || return $?
```

&nbsp;



##### External App

Get `$FOO_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name with `3` numbers separated with dots (`x.y.z`), then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$FOO_APP__APP_VERSION_NAME` variable.

```shell
termux_core__bash__termux_apps_info_app_version_name get-value --extended-validator="r+=^(()|([0-9]+\.[0-9]+\.[0-9]+))$" FOO_APP__APP_VERSION_NAME s="FOO_APP__" || return $?
```

&nbsp;

## &nbsp;

&nbsp;



### sh

`termux-apps-info-app-version-name.sh` is a [POSIX shell](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html) implementation for `termux-apps-info-app-version-name` and exists in Termux default `bin` directory in `$PATH` in Termux environments. However, the shell must support the [`local`](https://www.shellcheck.net/wiki/SC3043) keyword for variables, which normally is supported.

The source file for the `termux-apps-info-app-version-name.sh` script is the [`termux-apps-info-app-version-name.sh.in`] file in the `termux-core-package` repo, and it internally uses the [`termux_core__sh__termux_apps_info_app_version_name()`] function defined in the `termux-packages` repo which is replaced at build time with the `@TERMUX_CORE__SH__TERMUX_APPS_INFO_APP_VERSION_NAME@` placeholder, see below for how.

`termux-apps-info-app-version-name.sh` script can be used at runtime by:

1. Sourcing the `termux-apps-info-app-version-name.sh` file in `$PATH` (with [`.`](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_18_01) command) by running the following commands and then calling the `termux_core__sh__termux_apps_info_app_version_name()` function that should be defined in the current shell environment. The dependency functions like [`termux_core__sh__termux_apps_info_env_variable()`] and [`termux_core__sh__termux_scoped_env_variable()`] will also get automatically defined when sourcing the `termux-apps-info-app-version-name.sh` file and the `termux-apps-info-env-variable.sh` and `termux-scoped-env-variable.sh` files will not need to be sourced.
For the `get-*` commands, the name of an environment variable in which the output should be set can be passed as the `output_mode` argument, instead of having to call the function in a subshell and capturing its `stdout` for the output.  
The `command -v` command is used to find the location of the script file instead of directly using the `.`/`source` command to prevent sourcing of a (malicious) file in the current working directory with the same name instead of the one in `$PATH`. A separate function is used to source so that arguments passed to calling script/function are not passed to the sourced script. Passing `--sourcing-script` argument is necessary if sourcing from a `sh` shell script so that script `main` function is not run as there is no POSIX way to detect if current script is being sourced from another script. Replace `exit` with `return` if running inside a function.
    ```shell
    source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then . "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

    source_file_from_path "termux-apps-info-app-version-name.sh" --sourcing-script || exit $?

    termux_core__sh__termux_apps_info_app_version_name <command> output_variable <arguments...> || exit $?
    ```

2. Executing `termux-apps-info-app-version-name.sh` directly (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and capturing its `stdout`/`stderr` for the output and errors.
    ```shell
    output="$(termux-apps-info-app-version-name.sh <command> '>' <arguments...>)" || exit $?
    ```

Sourcing should be preferred, especially if multiple variable names or values need to be read. Sourcing the `termux-apps-info-app-version-name.sh` script once would be a single external call, and then any calls to the `termux_core__sh__termux_apps_info_app_version_name()` function would all be local calls, so would be faster than if `termux-apps-info-app-version-name.sh` script is executed multiple times with external calls for each variable. If efficiency is not an issue, then it doesn't matter much if script is executed or sourced, other than being able to pass a variable name as `output_mode`.

&nbsp;

If efficiency is an issue for time critical calling scripts that require low latency, the `termux_core__sh__termux_apps_info_app_version_name()` function can be imported into calling script source files so that external calls to source or execute `termux-apps-info-app-version-name.sh` is not required and the function can be called locally as it will already be defined. This can be done at build time by:

1. The [`termux_core__sh__termux_apps_info_app_version_name()`] function and its dependency functions like [`termux_core__sh__termux_apps_info_env_variable()`] and [`termux_core__sh__termux_scoped_env_variable()`] can be copied into calling script source files manually. It will have to kept in sync with upstream whenever any changes or fixes are done. The `@TERMUX_ENV__S_ROOT@` placeholder in the function should ideally be replaced at build time with the value defined in the [`properties.sh`] file of the `termux-packages` repo.
2. The `@TERMUX_CORE__SH__TERMUX_APPS_INFO_APP_VERSION_NAME@` placeholder can be added to calling script source files as a commented line like `##### @TERMUX_CORE__SH__TERMUX_APPS_INFO_APP_VERSION_NAME@ to be replaced at build time. #####` and then during build time, the [`termux-replace-termux-core-src-scripts`] script in the `termux-packages` repo can be executed and passed the paths to the script source files so that it replaces the placeholders with actual source content of the [`termux_core__sh__termux_apps_info_app_version_name()`] function and its dependency functions like [`termux_core__sh__termux_scoped_env_variable()`]. The required variables listed in the `termux-replace-termux-core-src-scripts` script help must be exported when executing it. For example, the `termux-core` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-core/build.sh) file used for compiling the package, passes the build time values for the required variables and the `$TERMUX_PACKAGES__REPO_DIR` environment variable for the root directory of the local `termux-packages` repo directory, which is required to find the path of the `termux-replace-termux-core-src-scripts` script on the local filesystem so that it can be executed. The [`Makefile`](https://github.com/termux/termux-core-package/blob/v0.1.0/Makefile) of the `termux-core` package then reads the variables and executes the `termux-replace-termux-core-src-scripts` script on the [`termux-apps-info-app-version-name.sh.in`] file that contains the `@TERMUX_CORE__SH__TERMUX_APPS_INFO_APP_VERSION_NAME@` placeholder.

&nbsp;



#### Help

```
termux-apps-info-app-version-name.sh can be used to get/unset
variable values of the app version name environment variables
`*_APP__APP_VERSION_NAME` for Termux app `TERMUX_APP__`, its plugin
apps `TERMUX_*_APP__` and external apps `*_APP__` app scoped that
exist in the `termux-apps-info.env` file, with support for validation
of values.


Usage:
  termux-apps-info-app-version-name.sh <command> <args...>


Available commands:
    get-value                    Get app version name value from
                                 Termux scoped variable.
    unset-value                  Unset Termux scoped variable value
                                 for app version name.



get-value:
  termux-apps-info-app-version-name.sh get-value [<command_options>] \
    <output_mode> \
    <scoped_var_scope_mode>

Available command options:
  [ --skip-sourcing ]
                     Skip sourcing of `termux-apps-info.env` file
                     before getting value. By default, the
                     '--skip-sourcing-if-cur-app-var' flag is passed
                     to 'termux-apps-info-env-variable.sh' instead.
  [ --posix-validator=<validator> ]
                     The posix validator to pass to
                     'termux-scoped-env-variable.sh' for validation
                     of app version name value. By default, the value
                     must start with a number.



unset-value:
  termux-apps-info-app-version-name.sh unset-value \
    <scoped_var_scope_mode>
```

The `get-value` command type returns the value for the Termux scoped
environment variable generated for the app version name depending on
the variable scope passed and and the sub name as `APP_VERSION_NAME`.
The value is read from the environment if app scope of the variable
to get is the same as the app scope of the current app set in the
`$TERMUX_ENV__S_APP` environment variable, otherwise
`termux-apps-info.env` file is sourced first before reading the value
unless the `--skip-sourcing` flag is passed. If the environment
variable is not set (like in case app is not installed) or is set to a
valid app version name starting with a number (default) or matches the
custom validator passed with the `--posix-validator` argument, then
call will return with exit code `0`. If its set, but not to a valid
app version name, then the call will return with exit code
`81` (`C_EX__NOT_FOUND`).

The `unset-value` command type unsets the value of the Termux scoped
environment variable generated for the app version name by running the
`unset` command.

The `unset-value` command type is not available if executing the
`termux-apps-info-app-version-name.sh` script as that will not have
any effect on the calling process environment and is only available if
the script is sourced and the
`termux_core__sh__termux_apps_info_app_version_name` function is
called.

Check the help of `termux-apps-info-env-variable.sh` and
`termux-scoped-env-variable.sh` commands for info on the arguments for
the `get-value` and `unset-value` command types.

**See also:**
- [`termux-apps-info-app-version-name.sh.in`]
- [`termux_core__sh__termux_apps_info_app_version_name()`]

- [`termux-apps-info-env-variable`]
- [`termux-apps-info-env-variable.sh.in`]
- [`termux_core__sh__termux_apps_info_env_variable()`]
.
- [`termux-scoped-env-variable`]
- [`termux-scoped-env-variable.sh.in`]
- [`termux_core__sh__termux_scoped_env_variable()`]

&nbsp;



#### Examples

The following examples assume that `termux-apps-info-app-version-name.sh` script has been sourced in the current shell (so that/or) the `termux_core__sh__termux_apps_info_app_version_name` function is defined and can be called. If `termux-apps-info-app-version-name.sh` needs to be executed instead, then replace `termux_core__sh__termux_apps_info_app_version_name` in the following examples with `termux-apps-info-app-version-name.sh` and the second argument for `output_mode` with the value `>` so that the output is printed to `stdout`.

```shell
source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then . "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

source_file_from_path "termux-apps-info-app-version-name.sh" --sourcing-script || exit $?
```

&nbsp;



##### `TERMUX_APP__APP_VERSION_NAME`

Get `$TERMUX_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name starting with a number, then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__APP_VERSION_NAME` variable.

```shell
termux_core__sh__termux_apps_info_app_version_name get-value TERMUX_APP__APP_VERSION_NAME cn="termux-app" || return $?
```

&nbsp;



##### `TERMUX_API_APP__APP_VERSION_NAME`

Get `$TERMUX_API_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name starting with a number, then call will return with exit code `0`. If its set, but not to a valid app version name, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_API_APP__APP_VERSION_NAME` variable.

```shell
termux_core__sh__termux_apps_info_app_version_name get-value TERMUX_API_APP__APP_VERSION_NAME cn="termux-api-app" || return $?
```

&nbsp;



##### Multiple Variable Read

Unset `$TERMUX_APP__APP_VERSION_NAME` and `$TERMUX_API_APP__APP_VERSION_NAME` variables, then source the `termux-apps-info.env` file to set their latest values into the current environment, then get their variable values without sourcing the file again.

```shell
termux_core__sh__termux_apps_info_app_version_name unset-value cn="termux-app" || return $?
termux_core__sh__termux_apps_info_app_version_name unset-value cn="termux-api-app" || return $?
termux_core__sh__termux_apps_info_env_variable source-env || return $?
termux_core__sh__termux_apps_info_app_version_name get-value --skip-sourcing TERMUX_APP__APP_VERSION_NAME cn="termux-app" || return $?
termux_core__sh__termux_apps_info_app_version_name get-value --skip-sourcing TERMUX_API_APP__APP_VERSION_NAME cn="termux-api-app" || return $?
```

&nbsp;



##### External App

Get `$FOO_APP__APP_VERSION_NAME` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set (like in case app is not installed) or is set to a valid app version name, then call will return with exit code `0`. If its set, but not to a valid app version name with `3` single digit numbers separated with dots (`x.y.z`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$FOO_APP__APP_VERSION_NAME` variable.

```shell
termux_core__sh__termux_apps_info_app_version_name get-value --posix-validator="p+=''|[0-9].[0-9].[0-9]" FOO_APP__APP_VERSION_NAME s="FOO_APP__" || return $?
```

---

&nbsp;





[`properties.sh`]: https://github.com/termux/termux-packages/blob/master/scripts/properties.sh
[`termux-apps-info-app-version-name.bash.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-apps-info-app-version-name.bash.in
[`termux-apps-info-app-version-name.sh.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-apps-info-app-version-name.sh.in
[`termux-apps-info-env-variable`]: termux-apps-info-env-variable.md
[`termux-apps-info-env-variable.bash.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-apps-info-env-variable.bash.in
[`termux-apps-info-env-variable.sh.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-apps-info-env-variable.sh.in
[`termux-scoped-env-variable`]: termux-scoped-env-variable.md
[`termux-scoped-env-variable.bash.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-scoped-env-variable.bash.in
[`termux-scoped-env-variable.sh.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-scoped-env-variable.sh.in
[`termux_core__bash__termux_apps_info_app_version_name()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__bash__termux_apps_info_app_version_name
[`termux_core__sh__termux_apps_info_app_version_name()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__sh__termux_apps_info_app_version_name
[`termux_core__bash__termux_apps_info_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__bash__termux_apps_info_env_variable
[`termux_core__sh__termux_apps_info_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__sh__termux_apps_info_env_variable
[`termux_core__bash__termux_scoped_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__bash__termux_scoped_env_variable
[`termux_core__sh__termux_scoped_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__sh__termux_scoped_env_variable
[`termux-replace-termux-core-src-scripts`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/build/scripts/termux-replace-termux-core-src-scripts
