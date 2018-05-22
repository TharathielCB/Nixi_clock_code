# Platformio make commands
all:
	platformio -f -c nvim run

upload:
	platformio -f -c nvim run --target upload

clean:
	platformio -f -c nvim run --target clean

program:
	platformio -f -c nvim run --target program

uploadfs:
	platformio -f -c nvim run --target uploadfs

update:
	platformio -f -c nvim update

