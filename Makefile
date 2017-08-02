fit_eyes:
	make -C src fit_eyes
	cp -v src/fit_eyes .

clean:
	make -C src clean
	rm -f fit_eyes
