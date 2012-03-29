
export STATIC=0

all:
	@mkdir -p bin
	@make -C src
	@mv src/edacc_jobserver bin/edacc_jobserver

clean:
	@make -C src clean
