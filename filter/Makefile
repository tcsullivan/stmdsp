all:
	@arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os --specs=nosys.specs -nostartfiles -fPIE -c test.cpp
	@arm-none-eabi-ld -shared -n -N -z max-page-size=512 -Ttext-segment=0 \
	    test.o -o test.so
	@arm-none-eabi-strip -s -S --strip-unneeded test.so
	@arm-none-eabi-objcopy --remove-section .dynsym \
						   --remove-section .dynstr \
						   --remove-section .dynamic \
						   --remove-section .hash \
						   --remove-section .ARM.exidx \
						   --remove-section .ARM.attributes \
						   --remove-section .comment \
						   test.so

