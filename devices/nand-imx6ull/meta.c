

ssize_t meta_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	const nand_t *nand = nand_get(minor);

	nand_read(nand, paddr, NULL, aux);
}


ssize_t meta_write(unsigned int minor, addr_t offs, const void *buff, size_t len);
{
	const nand_t *nand = nand_get(minor);

	nand_write(nand, paddr, NULL, aux);
}


__attribute__((constructor)) static void data_register(void)
{
	static const dev_handler_t h[] = {
		.read = meta_read,
		.write = meta_write,
		.sync = meta_sync,
		.map = meta_map,
		.done = meta_done,
		.init = meta_init,
	};

	devs_register(DEV_NAND_META, NAND_MAX_CNT, &h);
}
