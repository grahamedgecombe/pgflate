MODULE_big = flate
EXTENSION = flate
OBJS = flate.o
DATA = flate--1.0.0--1.0.1.sql flate--1.0.1.sql

SHLIB_LINK += -lz

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
