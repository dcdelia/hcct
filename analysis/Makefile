CC	= gcc
#CFLAGS	= -O3 -Wall
CFLAGS  = -g -Wall

INCLUDE_DIR = include
SRC_DIR = src
OBJ_DIR = build

GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
GLIB_LDFLAGS = $(shell pkg-config --libs glib-2.0)

ANALYSIS_SRC_FILES = analysis.c metrics.c output.c symbols.c tree.c
ANALYSIS_DEP_FILES = common.h config.h metrics.h output.h symbols.h tree.h
ANALYSIS_SRC = $(patsubst %.c,$(OBJ_DIR)/%.o,$(ANALYSIS_SRC_FILES))
ANALYSIS_DEP = $(patsubst %.h,$(INCLUDE_DIR)/%.h,$(ANALYSIS_DEP_FILES))
ANALYSIS_OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(ANALYSIS_SRC_FILES))

COMPARE_SRC_FILES = compare.c metrics.c output.c symbols.c tree.c compare_metrics.c comb.c
COMPARE_DEP_FILES = common.h config.h metrics.h output.h symbols.h tree.h compare_metrics.h comb.h compare.h
COMPARE_SRC = $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMPARE_SRC_FILES))
COMPARE_DEP = $(patsubst %.h,$(INCLUDE_DIR)/%.h,$(COMPARE_DEP_FILES))
COMPARE_OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMPARE_SRC_FILES))

SOLVE_SRC_FILES = solve.c tree.c symbols.c
SOLVE_DEP_FILES = common.h config.h symbols.h tree.h
SOLVE_SRC = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOLVE_SRC_FILES))
SOLVE_DEP = $(patsubst %.h,$(INCLUDE_DIR)/%.h,$(SOLVE_DEP_FILES))
SOLVE_OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOLVE_SRC_FILES))

SRC = $(sort $(ANALYSIS_SRC) $(COMPARE_SRC) $(SOLVE_SRC))
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP = $(sort $(ANALYSIS_DEP) $(COMPARE_DEP) $(SOLVE_DEP))

# TODO: better handling of dep

all:	analysis compare solve

analysis:   $(OBJ_DIR) $(ANALYSIS_SRC) $(ANALYSIS_OBJ)
	    $(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o analysis $(ANALYSIS_OBJ) -lm $(GLIB_LDFLAGS)

compare:    $(OBJ_DIR) $(COMPARE_SRC) $(COMPARE_OBJ)
	    $(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o compare $(COMPARE_OBJ) -lm $(GLIB_LDFLAGS)

solve:	    $(OBJ_DIR) $(SOLVE_SRC) $(SOLVE_OBJ)
	    $(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o solve $(SOLVE_OBJ) -lm $(GLIB_LDFLAGS)

$(OBJ_DIR):
	@mkdir -p $@

$(OBJ_DIR)/%.o:  src/%.c $(DEP)
	    $(CC) $(CFLAGS) $(GLIB_CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

.PHONY:	clean
clean:
	rm -rf $(OBJ_DIR) analysis compare solve
