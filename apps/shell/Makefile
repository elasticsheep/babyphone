all: hw0_teensy2 hw1_atmega328

clean: hw0_teensy2-clean hw1_atmega328-clean

program: hw1_atmega328-program

hw0_teensy2:
	make -f hw0_teensy2.makefile

hw0_teensy2-clean:
	make -f hw0_teensy2.makefile clean

hw1_atmega328:
	make -f hw1_atmega328.makefile

hw1_atmega328-clean:
	make -f hw1_atmega328.makefile clean

hw1_atmega328-program:
	make -f hw1_atmega328.makefile program

