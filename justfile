alias s:= shell

@i:
	bash ./package/install

@u:
	bash ./package/uninstall


@shell:
	shellcheck -x package/install package/uninstall

