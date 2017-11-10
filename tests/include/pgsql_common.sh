########################################################################
# Common code for PostgreSQL-specific tests
########################################################################
set -eu

if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
then
  exit 80
fi

# Emulate "\d+" output since it is not portable across PostgreSQL major versions
function db_show_table() {
  if ! psql -c "\d+ $1" sbtest > /dev/null
  then
      return
  fi

  echo "                            Table \"public.$1\""
  psql -q sbtest <<EOF
\pset footer off
 SELECT
    f.attname AS "Column",
    pg_catalog.format_type(f.atttypid,f.atttypmod) AS "Type",
    CASE
        WHEN f.attnotnull = 't' OR p.contype = 'p' THEN 'not null '
        ELSE ''
    END ||
    CASE
        WHEN f.atthasdef = 't' THEN 'default ' || d.adsrc
        ELSE ''
    END AS "Modifiers",
    CASE
        WHEN f.attstorage = 'p' THEN 'plain'
        ELSE 'extended'
    END as "Storage",
    CASE
        WHEN f.attstattarget < 0 THEN ''
        ELSE f.attstattarget::text
    END as "Stats target",
    pg_catalog.col_description(f.attrelid, f.attnum) as "Description"
FROM pg_attribute f
    JOIN pg_class c ON c.oid = f.attrelid
    JOIN pg_type t ON t.oid = f.atttypid
    LEFT JOIN pg_attrdef d ON d.adrelid = c.oid AND d.adnum = f.attnum
    LEFT JOIN pg_namespace n ON n.oid = c.relnamespace
    LEFT JOIN pg_constraint p ON p.conrelid = c.oid AND f.attnum = ANY (p.conkey)
    LEFT JOIN pg_class AS g ON p.confrelid = g.oid
WHERE c.relkind = 'r'::char
    AND n.nspname = 'public'
    AND c.relname = '$1'
    AND f.attnum > 0
EOF
  echo "Indexes:"
  psql -qt sbtest <<EOF
 SELECT indexdef
FROM pg_catalog.pg_indexes
WHERE schemaname = 'public'
    AND tablename = '$1'
ORDER BY 1
EOF
}

DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"
