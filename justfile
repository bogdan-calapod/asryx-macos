alias s:= shell

@i:
	bash ./install

@u:
	bash ./uninstall


@shell:
	shellcheck -x install uninstall

