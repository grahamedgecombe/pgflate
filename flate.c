#include <postgres.h>
#include <fmgr.h>
#include <zlib.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define BUFSZ 16384
#define DEFAULT_MEM_LEVEL 8

static z_stream deflate_strm, inflate_strm;
static unsigned char buf[BUFSZ];

void _PG_init(void);
void _PG_free(void);
Datum flate_deflate(PG_FUNCTION_ARGS);
Datum flate_inflate(PG_FUNCTION_ARGS);

void _PG_init(void)
{
    int ret;

    deflate_strm.zalloc = Z_NULL;
    deflate_strm.zfree  = Z_NULL;
    deflate_strm.opaque = Z_NULL;

    ret = deflateInit2(&deflate_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEFAULT_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        elog(FATAL, "deflateInit2 failed: %d", ret);

    inflate_strm.zalloc = Z_NULL;
    inflate_strm.zfree  = Z_NULL;
    inflate_strm.opaque = Z_NULL;

    ret = inflateInit2(&inflate_strm, -MAX_WBITS);
    if (ret != Z_OK)
        elog(FATAL, "inflateInit failed: %d", ret);
}

void _PG_free(void)
{
    deflateEnd(&deflate_strm);
    inflateEnd(&inflate_strm);
}

PG_FUNCTION_INFO_V1(flate_deflate);
Datum flate_deflate(PG_FUNCTION_ARGS)
{
    bytea *in, *out;
    int32 level;
    int ret;
    size_t out_pos, out_len;

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    in = PG_GETARG_BYTEA_P(0);
    level = PG_ARGISNULL(2) ? Z_DEFAULT_COMPRESSION : PG_GETARG_INT32(2);

    ret = deflateParams(&deflate_strm, level, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        elog(ERROR, "deflateParams failed: %d", ret);

    if (!PG_ARGISNULL(1))
    {
        bytea *dict = PG_GETARG_BYTEA_P(1);
        ret = deflateSetDictionary(&deflate_strm, (unsigned char *) VARDATA(dict), VARSIZE(dict) - VARHDRSZ);
        if (ret != Z_OK)
        {
            int reset_ret = deflateReset(&deflate_strm);
            if (reset_ret != Z_OK)
                elog(FATAL, "deflateReset failed: %d", reset_ret);

            elog(ERROR, "deflateSetDictionary failed: %d", ret);
        }
    }

    deflate_strm.avail_in = VARSIZE(in) - VARHDRSZ;
    deflate_strm.next_in  = (unsigned char *) VARDATA(in);

    out = palloc(VARHDRSZ);
    out_pos = 0;
    out_len = 0;

    do
    {
        size_t n;

        deflate_strm.avail_out = BUFSZ;
        deflate_strm.next_out  = buf;

        ret = deflate(&deflate_strm, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            int reset_ret;

            pfree(out);

            reset_ret = deflateReset(&deflate_strm);
            if (reset_ret != Z_OK)
                elog(FATAL, "deflateReset failed: %d", reset_ret);

            elog(ERROR, "deflate failed: %d", ret);
        }

        n = BUFSZ - deflate_strm.avail_out;
        if (n)
        {
            out_len += n;

            out = repalloc(out, out_len + VARHDRSZ);
            memcpy(VARDATA(out) + out_pos, buf, n);

            out_pos += n;
        }
    } while (ret != Z_STREAM_END);

    ret = deflateReset(&deflate_strm);
    if (ret != Z_OK)
        elog(FATAL, "deflateReset failed: %d", ret);

    SET_VARSIZE(out, out_len + VARHDRSZ);
    PG_RETURN_BYTEA_P(out);
}

PG_FUNCTION_INFO_V1(flate_inflate);
Datum flate_inflate(PG_FUNCTION_ARGS)
{
    bytea *in, *out;
    int ret;
    size_t out_pos, out_len;

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    in = PG_GETARG_BYTEA_P(0);

    inflate_strm.avail_in = 0;
    inflate_strm.next_in  = NULL;

    if (!PG_ARGISNULL(1))
    {
        bytea *dict = PG_GETARG_BYTEA_P(1);
        ret = inflateSetDictionary(&inflate_strm, (unsigned char *) VARDATA(dict), VARSIZE(dict) - VARHDRSZ);
        if (ret != Z_OK)
        {
            int reset_ret = inflateReset(&inflate_strm);
            if (reset_ret != Z_OK)
                elog(FATAL, "inflateReset failed: %d", reset_ret);

            elog(ERROR, "inflateSetDictionary failed: %d", ret);
        }
    }

    inflate_strm.avail_in = VARSIZE(in) - VARHDRSZ;
    inflate_strm.next_in  = (unsigned char *) VARDATA(in);

    out = palloc(VARHDRSZ);
    out_pos = 0;
    out_len = 0;

    do
    {
        size_t n;

        inflate_strm.avail_out = BUFSZ;
        inflate_strm.next_out  = buf;

        ret = inflate(&inflate_strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            int reset_ret;

            pfree(out);

            reset_ret = inflateReset(&inflate_strm);
            if (reset_ret != Z_OK)
                elog(FATAL, "inflateReset failed: %d", reset_ret);

            elog(ERROR, "inflate failed: %d", ret);
        }

        n = BUFSZ - inflate_strm.avail_out;
        if (n)
        {
            out_len += n;

            out = repalloc(out, out_len + VARHDRSZ);
            memcpy(VARDATA(out) + out_pos, buf, n);

            out_pos += n;
        }
    } while (ret != Z_STREAM_END);

    ret = inflateReset(&inflate_strm);
    if (ret != Z_OK)
        elog(FATAL, "inflateReset failed: %d", ret);

    SET_VARSIZE(out, out_len + VARHDRSZ);
    PG_RETURN_BYTEA_P(out);
}
