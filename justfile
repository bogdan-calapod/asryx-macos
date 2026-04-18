set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

alias f := format
alias fc := format-check
alias l := lint
alias b := build
alias t := test
alias sc := shellcheck
alias r := runtime-safety-check
alias c := clean


@format:
	scripts/fmt --fix

@format-check:
	scripts/fmt --check


@test:
	just build
	ctest --preset release

@lint:
	cmake --fresh --preset release
	scripts/lint

@shellcheck:
	scripts/shellcheck

@build:
	cmake --fresh --preset release
	cmake --build --preset release

@runtime-safety-check:
	cmake --fresh --preset asan
	cmake --build --preset asan
	ctest --preset asan
	cmake --fresh --preset ubsan
	cmake --build --preset ubsan
	ctest --preset ubsan


@clean:
	rm -rf build .cache .asryx-test-home .asryx-test-runtime
