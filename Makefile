#
# Makefile for Verse core; API and reference server.
# This pretty much requires GNU Make, I think.
#
# This build is slightly complicated that part of the C code that
# needs to go into the API implementation is generated by building
# and running other C files (this is the protocol definition).
#

CC	?= gcc
CFLAGS	?= -I$(shell pwd) -Wall -ansi -g #-pg -O2 -finline-functions
LDFLAGS	?= -pg

AR	?= ar
ARFLAGS	?= rus
RANLIB	?= ranlib

TARGETS = libverse.a verse

.PHONY:	all clean cleanprot

# Automatically generated protocol things.
PROT_DEF  = $(wildcard v_cmd_def_*.c)
PROT_TOOL = v_cmd_gen.c $(PROT_DEF)
PROT_OUT  = v_gen_pack_init.c v_gen_unpack_func.h verse.h \
		$(patsubst v_cmd_def_%.c,v_gen_pack_%_node.c, $(PROT_DEF))

# The API implementation is the protocol code plus a few bits.
LIBVERSE_SRC =  $(PROT_OUT) v_bignum.c v_cmd_buf.c v_connect.c v_connection.c v_connection.h \
		v_encryption.c \
		v_func_storage.c v_internal_verse.h v_man_pack_node.c \
		v_network.c v_network.h v_network_in_que.c v_network_out_que.c \
		v_pack.c v_pack.h v_pack_method.c v_prime.c v_util.c

LIBVERSE_OBJ = $(patsubst %h,, $(LIBVERSE_SRC:%.c=%.o))

# The server is a simple 1:1 mapping, so just use wildcards.
VERSE_SRC = $(wildcard vs_*.c)
VERSE_OBJ = $(VERSE_SRC:%.c=%.o)

# -----------------------------------------------------

all:		$(TARGETS)

verse:		$(VERSE_OBJ) libverse.a
		$(CC) $(LDFLAGS) -o $@ $^

libverse.a:	libverse.a($(LIBVERSE_OBJ))
		rm -f $@
		$(AR) $(ARFLAGS) $@ $(LIBVERSE_OBJ)
		$(RANLIB) $@

# -----------------------------------------------------

# Here are the automatically generated pieces of the puzzle.
# Basically, we generate v_gen_pack_X_node.c files by compiling
# the v_cmd_def_X.c files together with some driver glue and
# running the result.
#

# The autogen outputs all depend on the tool.
$(PROT_OUT):	mkprot
		./mkprot

# Build the protocol maker, from the definitions themselves.
mkprot:		$(PROT_TOOL)
		$(CC) -DV_GENERATE_FUNC_MODE -o $@ $^

# Clean away all the generated parts of the protocol implementation.
cleanprot:	clean
		rm -f mkprot $(PROT_OUT)

# -----------------------------------------------------

clean:
	rm -f *.o $(TARGETS)

# -----------------------------------------------------

# Utter ugliness to create release archives. Needs to improve, but should work for a while.
dist:
	RELEASE=$$( \
	R=`grep V_RELEASE_NUMBER verse.h | tr -s ' \t' | tr -d '"\r' | cut -d'	' -f3` ; \
	P=`grep V_RELEASE_PATCH verse.h | tr -s ' \t' | tr -d '"\r' | cut -d'	' -f3` ; \
	L=`grep V_RELEASE_LABEL verse.h | tr -s ' \t' | tr -d '"\r' | cut -d'	' -f3` ; echo r$${R}p$$P$$L ) ; \
	if [ $$RELEASE ]; then ( \
	 rm -rf  /tmp/verse; \
	 mkdir -p /tmp/verse; \
	 cp -a * /tmp/verse; \
	 cd /tmp && zip verse-$$RELEASE.zip -r verse -x 'verse/*CVS*' -x 'verse/.*' ; \
	 ); mv /tmp/verse-$$RELEASE.zip . \
	;else \
	  echo "Couldn't auto-set RELEASE from verse.h, something is fishy" \
	;fi
