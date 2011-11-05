.PHONY: all
all: partitions.bin

partitions.bin: dtmf/dtmf.part animals/animals.part record/record.part
	python make.py

dtmf/dtmf.part:
	cd dtmf && python make.py

animals/animals.part:
	cd animals && python make.py

record/record.part:
	cd record && python make.py
