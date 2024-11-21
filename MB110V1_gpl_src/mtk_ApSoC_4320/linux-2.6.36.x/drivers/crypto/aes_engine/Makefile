
# Make sure that ARCH, KERNEL_DIR and CROSS_COMPILE are set

ifndef KERNEL_DIR
  ERROR_KERNEL_DIR_NOT_SET:=YES
endif


ifndef ARCH
  ERROR_ARCH_NOT_SET:=YES
endif

PWD :=drivers/crypto/aes_engine
PATH_BUILD=$(PWD)

# Include Paths
INCLUDE_FLAGS_ALL:= \
  $(INCLUDE_APP)


# Compiler Flags
WARNING_FLAGS=
DEBUG_FLAGS=-O2
CODE_COVERAGE_FLAGS=
PROFILING_FLAGS=
BACKWARD_COMPATIBILITY_FLAGS=

EXTRA_CFLAGS += $(BACKWARD_COMPATIBILITY_FLAGS)
EXTRA_CFLAGS += $(INCLUDE_FLAGS_ALL)
EXTRA_CFLAGS += $(WARNING_FLAGS)
EXTRA_CFLAGS += $(DEBUG_FLAGS)
EXTRA_CFLAGS += $(PROFILING_FLAGS)


.PHONY = clean clean_module

obj-m	+= mtk_aes.o
mtk_aes-objs:= \
	aes_engine.o	\
	mcrypto_aes_api.o

#mtk_aes-objs+= \
	aes128_ciphertext.o	\
	aes128_plaintext.o
	
clean:	clean_module

clean_module:
	# this is run in $PATH_BUILD where the Makefile resides
	find . \( -name "*.o" -o -name ".*.cmd" -o -name "*.bak" \) -type f -print | xargs rm -f
	rm -rf .tmp_versions
	rm -f *.o  .*.cmd  .*.flags  *.mod.c  *.ko  *.symvers  *.markers  *.order
	
# end of file
#openssl enc -e -aes-128-cbc -in padlock-aes.c -out padlock-aes.c.aes -k 123456789 -iv 5a5aa5a5 -nosalt -P
#openssl enc -e -aes-128-cbc -in padlock-aes.c -out padlock-aes.c.aes -k 123456789 -iv 5a5aa5a5 -nosalt
#openssl enc -d -aes-128-cbc -in padlock-aes.c.aes -out padlock-aes.c.p -k 123456789 -iv 5a5aa5a5 -nosalt
#xxd -i padlock-aes.c >aes128_plaintext.c
#xxd -i padlock-aes.c.aes >aes128_ciphertext.c