set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

alias f := format
alias l := lint
alias b := build
alias t := test
alias g := gate

@format:
	scripts/fmt --fix

@format-check:
	scripts/fmt --check

@configure:
	cmake --fresh --preset dev

@build:
	just configure
	cmake --build --preset dev

@test:
	just build
	ctest --preset dev

@lint:
	just configure
	scripts/check

@sanitize:
	cmake --fresh --preset asan
	cmake --build --preset asan
	ctest --preset asan
	cmake --fresh --preset ubsan
	cmake --build --preset ubsan
	ctest --preset ubsan

@gate:
	just format-check
	just build
	just lint
	just test

@hard-gate:
	just gate
	just sanitize

@clean:
	rm -rf build .cache .asryx-test-home .asryx-test-runtime
