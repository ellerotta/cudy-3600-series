#
# ninja is a fancy installer tool used by systemd.
#

ninja:
	tar -xvzf v1.9.0.tar.gz
	cd ninja-1.9.0/ ; ./configure.py --bootstrap
	ln -s ninja-1.9.0/ninja ninja

clean:
	rm -rf ninja-1.9.0/
	rm -f ninja

