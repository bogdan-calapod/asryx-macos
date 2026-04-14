set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

alias f := format
alias l := lint
alias b := build
alias t := test

@format:
	scripts/fmt --fix

@format-check:
	scripts/fmt --check


@test:
	just build
	ctest --preset dev

@lint:
	just configure
	scripts/lint

@build:
	cmake --fresh --preset dev # configure the build dir
	cmake --build --preset dev

@runtime-safety-check:
	cmake --fresh --preset asan
	cmake --build --preset asan
	ctest --preset asan
	cmake --fresh --preset ubsan
	cmake --build --preset ubsan
	ctest --preset ubsan


@clean:
	rm -rf build .cache .asryx-test-home .asryx-test-runtime


@install-dev-deps:
  scripts/install-dev-deps