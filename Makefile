CFLAGS    = -MMD -MP
INCLUDES  = -Iapp_cpu1_bsp/ps7_cortexa9_1/include -I./printf -I.
LDFLAGS   = -Wl,-T -Wl,lscript.ld -Lapp_cpu1_bsp/ps7_cortexa9_1/lib
LIBRARIES = -Wl,--start-group,-lxil,-lgcc,-lc,--end-group

CC        = arm-xilinx-eabi-gcc
RM        = rm -rf

app_cpu1.elf: app_cpu1.o printf.o
	@echo ">> Linking $@"
	@$(CC) $(LDFLAGS) $(LIBRARIES) -o $@ $^

app_cpu1.o: app_cpu1.c
	@echo ">> Compiling $<"
	@$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

printf.o: printf.c
	@echo ">> Compiling $<"
	@$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

clean:
	@$(RM) *.elf *.o


#arm-xilinx-linux-gnueabi-gcc -fPIC -static remap.c -o remap
