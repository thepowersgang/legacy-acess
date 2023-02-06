# Project: Acess


.PHONY : all clean acessos

all: acessos


clean:
	$(MAKE) -C CommonCode clean
	$(MAKE) -C AcessOS clean

acessos:
	@$(MAKE) -C CommonCode
	@$(MAKE) -C AcessOS

