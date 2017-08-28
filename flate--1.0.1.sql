CREATE OR REPLACE FUNCTION deflate(bytea, bytea DEFAULT NULL, integer DEFAULT NULL) RETURNS bytea
    AS '$libdir/flate.so', 'flate_deflate'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION inflate(bytea, bytea DEFAULT NULL) RETURNS bytea
    AS '$libdir/flate.so', 'flate_inflate'
    LANGUAGE C IMMUTABLE;
