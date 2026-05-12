__asm(".global __use_no_semihosting");

void _sys_exit(int return_code) {
	(void)return_code;
}

void _ttywrch(int ch) {
	(void)ch;
}
