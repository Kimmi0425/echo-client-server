.PHONY : tc ts uc us clean install uninstall android-install android-uninstall

all: tc ts uc us

tc:
	cd tc; make; cd ..

ts:
	cd ts; make; cd ..

clean:
	cd tc; make clean; cd ..
	cd ts; make clean; cd ..
