# List of all the board related files.
ifeq ($(TARGET_PLATFORM),H7)
  BOARDSRC = ./board/board_h7.c
else
  BOARDSRC = ./board/board_l4.c
endif

# Required include directories
ifeq ($(TARGET_PLATFORM),H7)
  BOARDINC = ./board/h7
else
  BOARDINC = ./board/l4
endif

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
