# pgflate

## Introduction

pgflate is a PostgreSQL extension that provides functions for deflating and
inflating raw DEFLATE streams, with support for custom dictionaries.

## Prerequisites

* PostgreSQL headers, libraries and PGXS build infrastructure
* `pg_config` must be in your `PATH`
* zlib

## Building

Run `make` to build the extension.

## Installation

Run `make install` as `root` (e.g. with `sudo`) to install the extension.

## Debian package

The repository also contains the files for building a Debian package, which can
be done by running `pg_buildext updatecontrol` followed by `dpkg-buildpackage`.
I distribute pre-built versions for stable amd64 Debian using the
[apt.postgresql.org][pgapt] repository in my [personal APT repository][apt]. Run
`apt-get install postgresql-PGVERSION-flate` as root after setting up the
repository.

## Usage

Run `CREATE EXTENSION flate` to install the extension in the current database.
Two functions are provided:

| Function                                                                       | Return Type |
|--------------------------------------------------------------------------------|-------------|
| <code>deflate(*data* bytea [, *dictionary* bytea [, *level* integer ]])</code> | `bytea`     |
| <code>inflate(*data* bytea [, *dictionary* bytea ])</code>                     | `bytea`     |

`deflate` compresses the provided `data` and returns a raw DEFLATE stream. A
preset `dictionary` may also be provided. The default compression `level` may
also be overriden, valid values range from `0` (no compression), `1` (best
speed) to `9` (best compression).

If you want to override the compression level without using a dictionary, set
`dictionary` to `NULL`.

`inflate` decompresses the provided DEFLATE stream in `data` and returns the
uncompressed data. A preset `dictionary`, matching the dictionary used to
deflate the data, may also be provided.

## Example

    gpe=# CREATE EXTENSION flate;
    CREATE EXTENSION
    gpe=# SELECT deflate('hello hello hello hello', 'hello', 9);
         deflate
    ------------------
     \xcb00110a182400
    (1 row)

    gpe=# SELECT convert_from(inflate('\xcb00110a182400', 'hello'), 'utf-8');
          convert_from
    -------------------------
     hello hello hello hello
    (1 row)

    gpe=#

## License

This project is available under the terms of the ISC license, which is similar
to the 2-clause BSD license. See the `LICENSE` file for the copyright
information and licensing terms.

[pgapt]: https://wiki.postgresql.org/wiki/Apt
[apt]: https://www.grahamedgecombe.com/apt-repository
