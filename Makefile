
submodules:
	git submodule update --init --recursive

pteditor:
	cd PTEditor && make && cd module && make && sudo insmod pteditor.ko