---
page_ref: "@ARK_PROJECT__VARIANT@/termux/termux-core-package/docs/@ARK_DOC__VERSION@/usage/utils/termux-apps-info-env-variable.md"
---

# `termux-apps-info-env-variable` Utils Docs

The `termux-apps-info-env-variable` scripts/codes provide a way, if running in a Termux app, to source the `termux-apps-info.env` file into the current environment or get variable values of Termux app `TERMUX_APP__`, its plugin apps `TERMUX_*_APP__` and external apps `*_APP__` app scoped environment variables that exist in the `termux-apps-info.env` file, with support for fallback values and validation of values.

This is only supported for Termux app version `>= 0.119.0` and plugin app versions using its updated internal [`TermuxShellEnvironment`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellEnvironment.java) class changes.

### Contents

- [Rationale and Design](#rationale-and-design)
- [Implementations](#implementations)

---

&nbsp;





## Rationale and Design

*If you do not know what `termux-scoped-env-variable` is and why it exists, or what `TERMUX*__` scoped environment variables means, check the [Rationale](termux-scoped-env-variable.md#rationale-and-design) and [Scoped Variables](termux-scoped-env-variable.md#scoped-variables) sections of [`termux-scoped-env-variable` Utils Docs](termux-scoped-env-variable.md).*

`termux-apps-info-env-variable` exists to solve the following issues in addition to the ones solved by `termux-scoped-env-variable`.

1. A lot of Termux internal packages rely on variables specific to Termux apps, and so do lot of user programs. For examples, specific packages may read the Termux app version name (`$TERMUX_APP__APP_VERSION_NAME`) and Termux:API app version name (`$TERMUX_API_APP__APP_VERSION_NAME`) to see if app is installed and to run version specific logic, or read other variables get app data directory path or APK file path. The issue is that if environment variables are exported at shell creation time, they only represent the state of the apps at that point in time and any changes in the apps will result in the variables getting invalid. For example, if Termux:API app is not installed when Termux app creates a shell, its environment variables wouldn't be set. If user then installs the Termux:API app, packages in older shells will not know that it got installed, and only new shells will know. Moreover, if the Termux:API app was already installed at shell creation time and its variables were exported, it could either be updated or uninstalled afterwards, and its environment variables in the current shell would still refer to the old state. This will cause problems in running code dependent on app states.

2. So there needs to be way a for the current shell to get variable values representing the current state of apps. However, there are two main consideration for the mechanism that should be used to get the variable values. Firstly, latency must be very low, as there are time sensitive programs like [`termux-api-package`](https://github.com/termux/termux-api-package) scripts that cannot waste time just getting the variable values. Secondly, it should be easily callable from shells like `bash` and POSIX `sh` shells, and other binaries. A filesystem socket server running in the Termux app (like for [`termux-am-socket`](https://github.com/termux/termux-am-socket)) that clients could connect to to get the required variable values cannot be used as that has a high latency and first a server must be connected to, then a command sent and then additional I/O and time needs to spent to receive the variable values from the server. Additionally, a single command will only be able to get a single value as the result, unless additional parsing is done on output, which isn't always possible depending on calling shell/binary and parsing may require additional external calls, increasing latency yet again. Instead, an `.env` file ([1](https://github.com/ko1nksm/shdotenv#env-file-syntax), [2](https://github.com/ko1nksm/shdotenv/blob/main/docs/specification.md)) based approach is better, where all the required variables related to apps are written to an `.env` file by the app hosting the current shell whenever any of the watched app is installed, updated or uninstalled. Whenever a variable value needs to be read, the file can be first sourced into the environment of the current shell so that latest value gets loaded, and then the value can be read. Programs that are not running in a shell (like `c`/`c++` code) cannot (easily) source a file, so additionally a `.json` file can also be written to which can be parsed, and an executable is `$PATH` can also be provided that can be executed to get the variable values, which internally first sources the `.env` file.

3. Each Termux app that can host a shell, like the main Termux app and Termux:Float app, must manage its own `.env` and `.json` files. If only the main Termux app managed a single file for all apps, then if the main Termux app was not running and a plugin app like Termux:Float was hosting a shell, then the file would not be getting updated and anyone reading it would get old and invalid values. The contents of files maintained by different apps could vary too, like if some external apps are bound to main Termux app, its variables would get added to the file managed by Termux app, but not the one managed by Termux:Float as it wouldn't have info about the external apps bound to main Termux app, at least without using some Termux app API to get them.

4. Since each Termux app needs to manage its own environment files, each app will need to export the paths for its own `.env` and `.json` files in environment variables which should be used for sourcing/reading, namely `$TERMUX_CORE__APPS_INFO_ENV_FILE` and `$TERMUX_CORE__APPS_INFO_JSON_FILE` respectively, check below for more info. Additionally, the environment variable names would need to be scoped to Termux and could have a different root scope than the default `TERMUX_` value defined by the `$TERMUX_ENV__S_ROOT` variable, and so the name of the variable to read for the paths would need to be dynamically generated before sourcing. For example, if running under a forked Foo app with `FOO_` root scope set in `$TERMUX_ENV__S_ROOT`, then it will have its `.env` file path set in `FOO_CORE__APPS_INFO_ENV_FILE` variable instead for the file managed by Foo app. If under the Foo app shell, a user wants to call Termux packages with `root`/`su`, they could switch to a Termux environment by sourcing `$TERMUX__PREFIX/etc/termux/termux.env` file with `TERMUX_` root scope set in `$TERMUX_ENV__S_ROOT`, and starting a Termux shell. Under this Termux environment, the `$TERMUX_CORE__APPS_INFO_ENV_FILE` variable would refer to the different file managed by Termux app instead of the one managed by the Foo app with `$FOO_CORE__APPS_INFO_ENV_FILE`. If the forked app did not change the root scope and kept `TERMUX_` as root scope, then just multiple environments wouldn't be possible and even `TERMUX_APP__` variables would refer to values for the Foo app.

5. If a program wants to read the environment variables of the same app that is currently hosting the shell, then it may not need to source/read the `.env` and `.json` files as all or most variables for the current app would already be exported and can be read directly from the process environment, as they would not change as long as app is alive or not updated, and sourcing/reading the files would just waste time.

6. The variables for the apps exported in the environment files would be scoped to specific Termux apps or external apps, and so [`termux-scoped-env-variable`] or a similar logic will need to be used to read the variables after sourcing the `.env` file to the current environment, and optionally validation done as well on the values read.

7. If the `.env` file was already sourced earlier, and updated value needs to be read by sourcing the `.env` file again, then the variable to be read would need to be unset before sourcing the `.env` file as an updated version of the file may not have exported it, like if its respective app got uninstalled after the last sourcing. If its not unset beforehand, then the stale value would still be kept in the environment after the sourcing.

8. Multiple variables may need to be read for one ore more apps by a script, and sourcing should normally only be done once if variables are read in sequence in a short amount of time, so an option should be available to just source the `.env` file. Any variables that need to be read would have to be manually unset by users with the `termux-scoped-env-variable unset-value` command before sourcing the file.

9. If variables of all Termux and plugin apps are exported for every shell created by the app, then it clutters the shell environment and increases the space used for storing the environment in memory and time taken to copy it when processes are forked. The increased space usage will affect the max size of arguments that can be passed to commands as per `ARG_MAX` limits, which can result in `Argument list too long` errors. If all the non-current app variables exist in a file, then they can be sourced/read only when needed to reduce/prevent such issues.

To solve the above issues, each Termux app version `>= 0.119.0` and plugin app versions using its updated internal [`TermuxShellEnvironment`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellEnvironment.java) class changes creates the `termux-apps-info.env` and `termux-apps-info.json` files at app startup with variables for the Termux app, all official Termux plugins apps, any apps with the same `sharedUserId` and external apps bound to the Termux app plugin service. Additionally, the files are updated whenever any of those packages are installed, updated or uninstalled. The paths to the `termux-apps-info.env` and `termux-apps-info.json` files are exported in the `$TERMUX_CORE__APPS_INFO_ENV_FILE` and `$TERMUX_CORE__APPS_INFO_JSON_FILE` environment variables respectively. Since the variables are scoped, they can have a different root scope than the default `TERMUX_` value defined by the `$TERMUX_ENV__S_ROOT` variable, and so the name of the variable to read for the path must be dynamically generated before sourcing. The path to the files will be under the Termux apps directory ([`$TERMUX__APPS_DIR_BY_IDENTIFIER`](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-apps-directory)) for the Termux app or plugin that hosts the shell, which defaults to `/data/data/com.termux/termux/apps/i`.

If running in Termux app version `< 0.119.0` where environment variables are not exported, then `termux-apps-info.env` file wouldn't exist and any calls to `termux-apps-info-env-variable` utils would return with the exit code `81` (`C_EX__NOT_FOUND`) as the required environment variables would not be set, unless default fallback values are passed.

---

&nbsp;





## Implementations

The following implementations are currently provided for `termux-apps-info-env-variable` utils. Implementations for additional languages are planned for future, for now the `termux-apps-info-env-variable` executable in `$PATH` can be used by them.

- [Executable in `$PATH` (`termux-apps-info-env-variable`)](#termux-apps-info-env-variable)
- [`bash` shell (`termux-apps-info-env-variable.bash`)](#bash)
- [POSIX shells (`termux-apps-info-env-variable.sh`)](#sh)

&nbsp;



### `termux-apps-info-env-variable`

`termux-apps-info-env-variable` exists in Termux default `bin` directory in `$PATH` in Termux environments and can be executed as a command with arguments.

Currently, it is a symlink to [`termux-apps-info-env-variable.bash`](#bash), check its [Help](#help) section for more info on supported arguments and [Examples](#examples-1) section for additional examples.

**DO NOT** [`source`](https://www.gnu.org/software/bash/manual/bash.html#index-_002e) this file, as underlying implementation may not be guaranteed to be a `bash` syntax. It should be executed (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and its `stdout` captured for the result output.

The `output_mode` argument must be set to `>` so that output is returned in `stdout`, as if a variable name is passed instead, the variable will only be set in the `termux-apps-info-env-variable` child process and not in the calling parent process. The `>` argument should be quoted with single `'` or double quotes `"` if running in a shell, otherwise it may be considered a [command redirection](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_07).

&nbsp;



#### Examples

##### `TERMUX_APP__DATA_DIR`

Get `$TERMUX_APP__DATA_DIR` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be printed to `stdout`.

```shell
termux-apps-info-env-variable get-value '>' cn="termux-app" "DATA_DIR" r+="^(/[^/]+)+$" || return $?
```

## &nbsp;

&nbsp;



### bash

`termux-apps-info-env-variable.bash` is a [`bash` shell](https://www.gnu.org/software/bash/manual/bash.html) implementation for `termux-apps-info-env-variable` and exists in Termux default `bin` directory in `$PATH` in Termux environments.

The source file for the `termux-apps-info-env-variable.bash` script is the [`termux-apps-info-env-variable.bash.in`] file in the `termux-core-package` repo, and it internally uses the [`termux_core__bash__termux_apps_info_env_variable()`] function defined in the `termux-packages` repo which is replaced at build time with the `@TERMUX_CORE__BASH__TERMUX_APPS_INFO_ENV_VARIABLE@` placeholder, see below for how.

`termux-apps-info-env-variable.bash` script can be used at runtime by:

1. Sourcing the `termux-apps-info-env-variable.bash` file in `$PATH` (with [`.`](https://www.gnu.org/software/bash/manual/bash.html#index-_002e) or [`source`](https://www.gnu.org/software/bash/manual/bash.html#index-source) command) by running the following commands and then calling the `termux_core__bash__termux_apps_info_env_variable()` function that should be defined in the current shell environment. The dependency functions like [`termux_core__bash__termux_scoped_env_variable()`] will also get automatically defined when sourcing the `termux-apps-info-env-variable.bash` file and the `termux-scoped-env-variable.bash` file will not need to be sourced.
For the `get-*` commands, the name of an environment variable in which the output should be set can be passed as the `output_mode` argument, instead of having to call the function in a subshell and capturing its `stdout` for the output.  
The `command -v` command is used to find the location of the script file instead of directly using the `.`/`source` command to prevent sourcing of a (malicious) file in the current working directory with the same name instead of the one in `$PATH`. A separate function is used to source so that arguments passed to calling script/function are not passed to the sourced script. Replace `exit` with `return` if running inside a function.
    ```shell
    source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then source "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

    source_file_from_path "termux-apps-info-env-variable.bash" || exit $?

    termux_core__bash__termux_apps_info_env_variable <command> output_variable <arguments...> || exit $?
    ```

2. Executing `termux-apps-info-env-variable.bash` directly (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and capturing its `stdout`/`stderr` for the output and errors.
    ```shell
    output="$(termux-apps-info-env-variable.bash <command> '>' <arguments...>)" || exit $?
    ```

Sourcing should be preferred, especially if multiple variable names or values need to be read. Sourcing the `termux-apps-info-env-variable.bash` script once would be a single external call, and then any calls to the `termux_core__bash__termux_apps_info_env_variable()` function would all be local calls, so would be faster than if `termux-apps-info-env-variable.bash` script is executed multiple times with external calls for each variable. If efficiency is not an issue, then it doesn't matter much if script is executed or sourced, other than being able to pass a variable name as `output_mode`.

&nbsp;

If efficiency is an issue for time critical calling scripts that require low latency, the `termux_core__bash__termux_apps_info_env_variable()` function can be imported into calling script source files so that external calls to source or execute `termux-apps-info-env-variable.bash` is not required and the function can be called locally as it will already be defined. This can be done at build time by:

1. The [`termux_core__bash__termux_apps_info_env_variable()`] function and its dependency functions like [`termux_core__bash__termux_scoped_env_variable()`] can be copied into calling script source files manually. It will have to kept in sync with upstream whenever any changes or fixes are done. The `@TERMUX_ENV__S_ROOT@` placeholder in the function should ideally be replaced at build time with the value defined in the [`properties.sh`] file of the `termux-packages` repo.
2. The `@TERMUX_CORE__BASH__TERMUX_APPS_INFO_ENV_VARIABLE@` placeholder can be added to calling script source files as a commented line like `##### @TERMUX_CORE__BASH__TERMUX_APPS_INFO_ENV_VARIABLE@ to be replaced at build time. #####` and then during build time, the [`termux-replace-termux-core-src-scripts`] script in the `termux-packages` repo can be executed and passed the paths to the script source files so that it replaces the placeholders with actual source content of the [`termux_core__bash__termux_apps_info_env_variable()`] function and its dependency functions like [`termux_core__bash__termux_scoped_env_variable()`]. The required variables listed in the `termux-replace-termux-core-src-scripts` script help must be exported when executing it. For example, the `termux-core` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-core/build.sh) file used for compiling the package, passes the build time values for the required variables and the `$TERMUX_PACKAGES__REPO_DIR` environment variable for the root directory of the local `termux-packages` repo directory, which is required to find the path of the `termux-replace-termux-core-src-scripts` script on the local filesystem so that it can be executed. The [`Makefile`](https://github.com/termux/termux-core-package/blob/v0.1.0/Makefile) of the `termux-core` package then reads the variables and executes the `termux-replace-termux-core-src-scripts` script on the [`termux-apps-info-env-variable.bash.in`] file that contains the `@TERMUX_CORE__BASH__TERMUX_APPS_INFO_ENV_VARIABLE@` placeholder.

&nbsp;



#### Help

```
termux-apps-info-env-variable.bash can be used to source the
`termux-apps-info.env` file into the current environment or get
variable values of Termux app `TERMUX_APP__`, its plugin apps
`TERMUX_*_APP__` and external apps `*_APP__` app scoped environment
variables that exist in the `termux-apps-info.env` file, with
support for fallback values and validation of values.


Usage:
  termux-apps-info-env-variable.bash <command> <args...>


Available commands:
    source-env                   Source the apps info environment.
    get-value                    Get Termux scoped variable value.



source-env:
  termux-apps-info-env-variable.bash source-env \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name>



get-value:
  termux-apps-info-env-variable.bash get-value [<command_options>] \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name> \
    <extended_validator> [<default_values...>]

Available command options:
  [ --skip-sourcing ]
                     Skip sourcing of `termux-apps-info.env` file
                     before getting value.
  [ --skip-sourcing-if-cur-app-var ]
                     Skip sourcing of `termux-apps-info.env` file
                     before getting value if app scope of the variable
                     to get is the same as the app scope of the
                     current app set in the `$TERMUX_ENV__S_APP`
                     environment variable.
```

The `source-env` command type sources the `termux-apps-info.env` into
the current environment.

The `get-value` command type returns the value for the Termux scoped
environment variable generated depending on the variable scope and sub
name passed. First its value is read from the environment after
optionally sourcing the `termux-apps-info.env`  file depending on
`--skip-sourcing*` flags, followed by reading the optional values
passed as `default_values`, and whichever value first matches the
`extended_validator` is returned and if no value matches, then the
call will return with exit code `81` (`C_EX__NOT_FOUND`).
The `scoped_var_scope_mode` argument must be set, and for scope mode
(`s=`) and sub scope mode (`ss=`), the values must end with `_APP__`,
and for the component name mode (`cn=`), the value must end with
`-app`.

The `source-env` command type is not available if executing the
`termux-apps-info-env-variable.bash` script as that will not have any
effect on the calling process environment and is only available if
the script is sourced and the
`termux_core__bash__termux_apps_info_env_variable` function is called.

Check the help of `termux-scoped-env-variable.bash` command for info
on the arguments for the `get-value` command type.

**See also:**
- [`termux-apps-info-env-variable.bash.in`]
- [`termux_core__bash__termux_apps_info_env_variable()`]
.
- [`termux-scoped-env-variable`]
- [`termux-scoped-env-variable.bash.in`]
- [`termux_core__bash__termux_scoped_env_variable()`]

&nbsp;



#### Examples

The following examples assume that `termux-apps-info-env-variable.bash` script has been sourced in the current shell (so that/or) the `termux_core__bash__termux_apps_info_env_variable` function is defined and can be called. If `termux-apps-info-env-variable.bash` needs to be executed instead, then replace `termux_core__bash__termux_apps_info_env_variable` in the following examples with `termux-apps-info-env-variable`/`termux-apps-info-env-variable.bash` and the second argument for `output_mode` with the value `>` so that the output is printed to `stdout`.

```shell
source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then source "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

source_file_from_path "termux-apps-info-env-variable.bash" || exit $?
```

&nbsp;



##### Source Environment

Source the `termux-apps-info.env` file into the current environment.

```shell
termux_core__bash__termux_apps_info_env_variable source-env || return $?
```

&nbsp;



##### `TERMUX_APP__DATA_DIR`

Get `$TERMUX_APP__DATA_DIR` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__DATA_DIR` variable.

```shell
termux_core__bash__termux_apps_info_env_variable get-value TERMUX_APP__DATA_DIR cn="termux-app" "DATA_DIR" r+="^(/[^/]+)+$" || return $?
```

Get `$TERMUX_APP__DATA_DIR` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__DATA_DIR` variable.

```shell
termux_core__bash__termux_apps_info_env_variable get-value --skip-sourcing-if-cur-app-var TERMUX_APP__DATA_DIR cn="termux-app" "DATA_DIR" r+="^(/[^/]+)+$" || return $?
```

&nbsp;



##### `TERMUX_APP__TARGET_SDK`

Get `$TERMUX_APP__TARGET_SDK` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not an unsigned integer in the range `0-10000` (inclusive), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__TARGET_SDK` variable.

```shell
is_valid_target_sdk() { case "${1:-}" in ''|*[!0-9]*|0[0-9]*) return 1;; esac; [ "$1" -le 10000 ]; }

termux_core__bash__termux_apps_info_env_variable get-value TERMUX_APP__TARGET_SDK cn="termux-app" "TARGET_SDK" c+='is_valid_target_sdk' || return $?
```

&nbsp;



##### Multiple Variable Read

Unset `$TERMUX_APP__APK_FILE` and `$TERMUX_APP__DATA_DIR` variables, then source the `termux-apps-info.env` file to set their latest values into the current environment, then get their variable values without sourcing the file again.

```shell
termux_core__bash__termux_scoped_env_variable unset-value cn="termux-app" "APK_FILE" || return $?
termux_core__bash__termux_scoped_env_variable unset-value cn="termux-app" "DATA_DIR" || return $?
termux_core__bash__termux_apps_info_env_variable source-env || return $?
termux_core__bash__termux_apps_info_env_variable get-value --skip-sourcing TERMUX_APP__APK_FILE cn="termux-app" "APK_FILE" r+="^(/[^/]+)+$" || return $?
termux_core__bash__termux_apps_info_env_variable get-value --skip-sourcing TERMUX_APP__DATA_DIR cn="termux-app" "DATA_DIR" r+="^(/[^/]+)+$" || return $?
```

&nbsp;



##### External App

Get `$FOO_APP__UID` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not an unsigned integer, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$FOO_APP__UID` variable.

```shell
termux_core__bash__termux_apps_info_env_variable get-value FOO_APP__UID s="FOO_APP__" "UID" r+="^[0-9]+$" || return $?
```

## &nbsp;

&nbsp;



### sh

`termux-apps-info-env-variable.sh` is a [POSIX shell](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html) implementation for `termux-apps-info-env-variable` and exists in Termux default `bin` directory in `$PATH` in Termux environments. However, the shell must support the [`local`](https://www.shellcheck.net/wiki/SC3043) keyword for variables, which normally is supported.

The source file for the `termux-apps-info-env-variable.sh` script is the [`termux-apps-info-env-variable.sh.in`] file in the `termux-core-package` repo, and it internally uses the [`termux_core__sh__termux_apps_info_env_variable()`] function defined in the `termux-packages` repo which is replaced at build time with the `@TERMUX_CORE__SH__TERMUX_APPS_INFO_ENV_VARIABLE@` placeholder, see below for how.

`termux-apps-info-env-variable.sh` script can be used at runtime by:

1. Sourcing the `termux-apps-info-env-variable.sh` file in `$PATH` (with [`.`](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_18_01) command) by running the following commands and then calling the `termux_core__sh__termux_apps_info_env_variable()` function that should be defined in the current shell environment. The dependency functions like [`termux_core__sh__termux_scoped_env_variable()`] will also get automatically defined when sourcing the `termux-apps-info-env-variable.sh` file and the `termux-scoped-env-variable.sh` file will not need to be sourced.
For the `get-*` commands, the name of an environment variable in which the output should be set can be passed as the `output_mode` argument, instead of having to call the function in a subshell and capturing its `stdout` for the output.  
The `command -v` command is used to find the location of the script file instead of directly using the `.`/`source` command to prevent sourcing of a (malicious) file in the current working directory with the same name instead of the one in `$PATH`. A separate function is used to source so that arguments passed to calling script/function are not passed to the sourced script. Passing `--sourcing-script` argument is necessary if sourcing from a `sh` shell script so that script `main` function is not run as there is no POSIX way to detect if current script is being sourced from another script. Replace `exit` with `return` if running inside a function.
    ```shell
    source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then . "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

    source_file_from_path "termux-apps-info-env-variable.sh" --sourcing-script || exit $?

    termux_core__sh__termux_apps_info_env_variable <command> output_variable <arguments...> || exit $?
    ```

2. Executing `termux-apps-info-env-variable.sh` directly (with [`exec()`](https://en.wikipedia.org/wiki/Exec_(system_call))) and capturing its `stdout`/`stderr` for the output and errors.
    ```shell
    output="$(termux-apps-info-env-variable.sh <command> '>' <arguments...>)" || exit $?
    ```

Sourcing should be preferred, especially if multiple variable names or values need to be read. Sourcing the `termux-apps-info-env-variable.sh` script once would be a single external call, and then any calls to the `termux_core__sh__termux_apps_info_env_variable()` function would all be local calls, so would be faster than if `termux-apps-info-env-variable.sh` script is executed multiple times with external calls for each variable. If efficiency is not an issue, then it doesn't matter much if script is executed or sourced, other than being able to pass a variable name as `output_mode`.

&nbsp;

If efficiency is an issue for time critical calling scripts that require low latency, the `termux_core__sh__termux_apps_info_env_variable()` function can be imported into calling script source files so that external calls to source or execute `termux-apps-info-env-variable.sh` is not required and the function can be called locally as it will already be defined. This can be done at build time by:

1. The [`termux_core__sh__termux_apps_info_env_variable()`] function and its dependency functions like [`termux_core__sh__termux_scoped_env_variable()`] can be copied into calling script source files manually. It will have to kept in sync with upstream whenever any changes or fixes are done. The `@TERMUX_ENV__S_ROOT@` placeholder in the function should ideally be replaced at build time with the value defined in the [`properties.sh`] file of the `termux-packages` repo.
2. The `@TERMUX_CORE__SH__TERMUX_APPS_INFO_ENV_VARIABLE@` placeholder can be added to calling script source files as a commented line like `##### @TERMUX_CORE__SH__TERMUX_APPS_INFO_ENV_VARIABLE@ to be replaced at build time. #####` and then during build time, the [`termux-replace-termux-core-src-scripts`] script in the `termux-packages` repo can be executed and passed the paths to the script source files so that it replaces the placeholders with actual source content of the [`termux_core__sh__termux_apps_info_env_variable()`] function and its dependency functions like [`termux_core__sh__termux_scoped_env_variable()`]. The required variables listed in the `termux-replace-termux-core-src-scripts` script help must be exported when executing it. For example, the `termux-core` package [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-core/build.sh) file used for compiling the package, passes the build time values for the required variables and the `$TERMUX_PACKAGES__REPO_DIR` environment variable for the root directory of the local `termux-packages` repo directory, which is required to find the path of the `termux-replace-termux-core-src-scripts` script on the local filesystem so that it can be executed. The [`Makefile`](https://github.com/termux/termux-core-package/blob/v0.1.0/Makefile) of the `termux-core` package then reads the variables and executes the `termux-replace-termux-core-src-scripts` script on the [`termux-apps-info-env-variable.sh.in`] file that contains the `@TERMUX_CORE__SH__TERMUX_APPS_INFO_ENV_VARIABLE@` placeholder.

&nbsp;



#### Help

```
termux-apps-info-env-variable.sh can be used to source the
`termux-apps-info.env` file into the current environment or get
variable values of Termux app `TERMUX_APP__`, its plugin apps
`TERMUX_*_APP__` and external apps `*_APP__` app scoped environment
variables that exist in the `termux-apps-info.env` file, with
support for fallback values and validation of values.


Usage:
  termux-apps-info-env-variable.sh <command> <args...>


Available commands:
    source-env                   Source the apps info environment.
    get-value                    Get Termux scoped variable value.



source-env:
  termux-apps-info-env-variable.sh source-env \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name>



get-value:
  termux-apps-info-env-variable.sh get-value [<command_options>] \
    <output_mode> \
    <scoped_var_scope_mode> <scoped_var_sub_name> \
    <posix_validator> [<default_values...>]

Available command options:
  [ --skip-sourcing ]
                     Skip sourcing of `termux-apps-info.env` file
                     before getting value.
  [ --skip-sourcing-if-cur-app-var ]
                     Skip sourcing of `termux-apps-info.env` file
                     before getting value if app scope of the variable
                     to get is the same as the app scope of the
                     current app set in the `$TERMUX_ENV__S_APP`
                     environment variable.
```

The `source-env` command type sources the `termux-apps-info.env` into
the current environment.

The `get-value` command type returns the value for the Termux scoped
environment variable generated depending on the variable scope and sub
name passed. First its value is read from the environment after
optionally sourcing the `termux-apps-info.env`  file depending on
`--skip-sourcing*` flags, followed by reading the optional values
passed as `default_values`, and whichever value first matches the
`posix_validator` is returned and if no value matches, then the
call will return with exit code `81` (`C_EX__NOT_FOUND`).
The `scoped_var_scope_mode` argument must be set, and for scope mode
(`s=`) and sub scope mode (`ss=`), the values must end with `_APP__`,
and for the component name mode (`cn=`), the value must end with
`-app`.

The `source-env` command type is not available if executing the
`termux-apps-info-env-variable.sh` script as that will not have any
effect on the calling process environment and is only available if
the script is sourced and the
`termux_core__sh__termux_apps_info_env_variable` function is called.

Check the help of `termux-scoped-env-variable.sh` command for info
on the arguments for the `get-value` command type.

**See also:**
- [`termux-apps-info-env-variable.sh.in`]
- [`termux_core__sh__termux_apps_info_env_variable()`]
.
- [`termux-scoped-env-variable`]
- [`termux-scoped-env-variable.sh.in`]
- [`termux_core__sh__termux_scoped_env_variable()`]

&nbsp;



#### Examples

The following examples assume that `termux-apps-info-env-variable.sh` script has been sourced in the current shell (so that/or) the `termux_core__sh__termux_apps_info_env_variable` function is defined and can be called. If `termux-apps-info-env-variable.sh` needs to be executed instead, then replace `termux_core__sh__termux_apps_info_env_variable` in the following examples with `termux-apps-info-env-variable.sh` and the second argument for `output_mode` with the value `>` so that the output is printed to `stdout`.

```shell
source_file_from_path() { local source_file="${1:-}"; [ $# -gt 0 ] && shift 1; local source_path; if source_path="$(command -v "$source_file")" && [ -n "$source_path" ]; then . "$source_path" || return $?; else echo "Failed to find the '$source_file' file to source." 1>&2; return 1; fi; }

source_file_from_path "termux-apps-info-env-variable.sh" --sourcing-script || exit $?
```

&nbsp;



##### Source Environment

Source the `termux-apps-info.env` file into the current environment.

```shell
termux_core__sh__termux_apps_info_env_variable source-env || return $?
```

&nbsp;



##### `TERMUX_APP__DATA_DIR`

Get `$TERMUX_APP__DATA_DIR` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__DATA_DIR` variable.

```shell
termux_core__sh__termux_apps_info_env_variable get-value TERMUX_APP__DATA_DIR cn="termux-app" "DATA_DIR" p+="/*[!/]" || return $?
```

Get `$TERMUX_APP__DATA_DIR` variable value from the current environment if running under Termux app, otherwise from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not a valid absolute path (excluding rootfs `/`), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__DATA_DIR` variable.

```shell
termux_core__sh__termux_apps_info_env_variable get-value --skip-sourcing-if-cur-app-var TERMUX_APP__DATA_DIR cn="termux-app" "DATA_DIR" p+="/*[!/]" || return $?
```

&nbsp;



##### `TERMUX_APP__TARGET_SDK`

Get `$TERMUX_APP__TARGET_SDK` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not an unsigned integer in the range `0-10000` (inclusive), then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$TERMUX_APP__TARGET_SDK` variable.

```shell
is_valid_target_sdk() { case "${1:-}" in ''|*[!0-9]*|0[0-9]*) return 1;; esac; [ "$1" -le 10000 ]; }

termux_core__sh__termux_apps_info_env_variable get-value TERMUX_APP__TARGET_SDK cn="termux-app" "TARGET_SDK" c+='is_valid_target_sdk' || return $?
```

&nbsp;



##### Multiple Variable Read

Unset `$TERMUX_APP__APK_FILE` and `$TERMUX_APP__DATA_DIR` variables, then source the `termux-apps-info.env` file to set their latest values into the current environment, then get their variable values without sourcing the file again.

```shell
termux_core__sh__termux_scoped_env_variable unset-value cn="termux-app" "APK_FILE" || return $?
termux_core__sh__termux_scoped_env_variable unset-value cn="termux-app" "DATA_DIR" || return $?
termux_core__sh__termux_apps_info_env_variable source-env || return $?
termux_core__sh__termux_apps_info_env_variable get-value --skip-sourcing TERMUX_APP__APK_FILE cn="termux-app" "APK_FILE" p+="/*[!/]" || return $?
termux_core__sh__termux_apps_info_env_variable get-value --skip-sourcing TERMUX_APP__DATA_DIR cn="termux-app" "DATA_DIR" p+="/*[!/]" || return $?
```

&nbsp;



##### External App

Get `$FOO_APP__UID` variable value from the `termux-apps-info.env` file after sourcing it. If the environment variable is not set or is not an unsigned integer, then call will return with exit code `81` (`C_EX__NOT_FOUND`). The output will be set to the `$FOO_APP__UID` variable.

```shell
termux_core__sh__termux_apps_info_env_variable get-value FOO_APP__UID s="FOO_APP__" "UID" p-="''|*[!0-9]*|0[0-9]*" || return $?
```

---

&nbsp;





[`properties.sh`]: https://github.com/termux/termux-packages/blob/master/scripts/properties.sh
[`termux-apps-info-env-variable.bash.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-apps-info-env-variable.bash.in
[`termux-apps-info-env-variable.sh.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-apps-info-env-variable.sh.in
[`termux-scoped-env-variable`]: termux-scoped-env-variable.md
[`termux-scoped-env-variable.bash.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-scoped-env-variable.bash.in
[`termux-scoped-env-variable.sh.in`]: https://github.com/termux/termux-core-package/blob/master/src/scripts/termux-scoped-env-variable.sh.in
[`termux_core__bash__termux_apps_info_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__bash__termux_apps_info_env_variable
[`termux_core__sh__termux_apps_info_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__sh__termux_apps_info_env_variable
[`termux_core__bash__termux_scoped_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__bash__termux_scoped_env_variable
[`termux_core__sh__termux_scoped_env_variable()`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/src/scripts/termux_core__sh__termux_scoped_env_variable
[`termux-replace-termux-core-src-scripts`]: https://github.com/termux/termux-packages/blob/master/packages/termux-core/build/scripts/termux-replace-termux-core-src-scripts
