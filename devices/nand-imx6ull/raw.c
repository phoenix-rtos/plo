

ssize_t raw_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	const nand_t *nand = nand_get(minor);

	nand_read(nand, paddr, data, NULL);
}


ssize_t raw_write(unsigned int minor, addr_t offs, const void *buff, size_t len);
{
	const nand_t *nand = nand_get(minor);

	nand_write(nand, paddr, data, NULL);
}


__attribute__((constructor)) static void data_register(void)
{
	static const dev_handler_t h[] = {
		.read = raw_read,
		.write = raw_write,
		.sync = raw_sync,
		.map = raw_map,
		.done = raw_done,
		.init = raw_init,
	};

	devs_register(DEV_NAND_RAW, NAND_MAX_CNT, &h);
}
