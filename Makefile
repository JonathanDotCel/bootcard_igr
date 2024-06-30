all::
	$(MAKE) -C sd2psxman
	$(MAKE) -C igr_app

clean::
	$(MAKE) -C sd2psxman clean
	$(MAKE) -C igr_app clean