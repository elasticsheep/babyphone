.PHONY: all
all: babyphone.image

clean:
	rm -f babyphone.image
	rm -f dtmf/dtmf.slotfs
	rm -f animals/animals.slotfs
	rm -f record/record.slotfs

babyphone.image: dtmf/dtmf.slotfs animals/animals.slotfs record/record.slotfs
	python make.py

dtmf/dtmf.slotfs:
	cd dtmf && python make.py

animals/animals.slotfs:
	cd animals && python make.py

record/record.slotfs:
	cd record && python make.py
