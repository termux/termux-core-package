---
page_ref: "@ARK_PROJECT__VARIANT@/termux/termux-core-package/docs/@ARK_DOC__VERSION@/developer/test/index.md"
---

# termux-core-package Test Docs

<!-- @ARK_DOCS__HEADER_PLACEHOLDER@ -->

The [`termux-core`](https://github.com/termux/termux-core-package) can be tested with [`termux-core-tests`](https://github.com/termux/termux-core-package/blob/master/tests/termux-core-tests.in).

To show help, run `"${TERMUX__PREFIX:-$PREFIX}/libexec/installed-tests/termux-core/termux-core-tests" --help`.

To run all tests, run `"${TERMUX__PREFIX:-$PREFIX}/libexec/installed-tests/termux-core/termux-core-tests" -vv all`. You can optionally run only `runtime` tests as well.
- The `runtime` tests test the runtime execution of utils provided by `termux-core` package.

Under normal circumstances with Termux app in foreground, tests take `~5min` to run depending on device.

---

&nbsp;





## Help

```
termux-core-tests is a script that run tests for the termux-core.


Usage:
    termux-core-tests [command_options] <command>

Available commands:
    runtime                   Run runtime on-device tests.
    all                       Run all tests.

Available command_options:
    [ -h | --help ]           Display this help screen.
    [ --version ]             Display version.
    [ -q | --quiet ]          Set log level to 'OFF'.
    [ -v | -vv | -vvv | -vvvvv ]
                              Set log level to 'DEBUG', 'VERBOSE',
                              'VVERBOSE' and 'VVVERBOSE'.
    [ --no-clean ]            Do not clean test files on failure.
    [ --tests-filter=<filter> ]
                              Regex to filter which tests to run.
    [ --tests-path=<path> ]   The path to installed-tests directory.
```

---

&nbsp;
