########################################################################
oltp.lua + PostgreSQL tests 
########################################################################

  $ if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"

  $ function db_show_table() {
  >  pg_dump -t "$1" --schema-only sbtest
  > }

  $ . $SBTEST_INCDIR/script_oltp_common.sh
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating secondary indexes on 'sbtest1'...
  Creating table 'sbtest2'...
  Inserting 10000 records into 'sbtest2'
  Creating secondary indexes on 'sbtest2'...
  Creating table 'sbtest3'...
  Inserting 10000 records into 'sbtest3'
  Creating secondary indexes on 'sbtest3'...
  Creating table 'sbtest4'...
  Inserting 10000 records into 'sbtest4'
  Creating secondary indexes on 'sbtest4'...
  Creating table 'sbtest5'...
  Inserting 10000 records into 'sbtest5'
  Creating secondary indexes on 'sbtest5'...
  Creating table 'sbtest6'...
  Inserting 10000 records into 'sbtest6'
  Creating secondary indexes on 'sbtest6'...
  Creating table 'sbtest7'...
  Inserting 10000 records into 'sbtest7'
  Creating secondary indexes on 'sbtest7'...
  Creating table 'sbtest8'...
  Inserting 10000 records into 'sbtest8'
  Creating secondary indexes on 'sbtest8'...
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest1; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest1 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest1 OWNER TO postgres;
  
  --
  -- Name: sbtest1_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest1_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest1_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest1_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest1_id_seq OWNED BY sbtest1.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest1 ALTER COLUMN id SET DEFAULT nextval('sbtest1_id_seq'::regclass);
  
  
  --
  -- Name: sbtest1_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest1
      ADD CONSTRAINT sbtest1_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_1; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_1 ON sbtest1 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest2; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest2 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest2 OWNER TO postgres;
  
  --
  -- Name: sbtest2_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest2_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest2_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest2_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest2_id_seq OWNED BY sbtest2.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest2 ALTER COLUMN id SET DEFAULT nextval('sbtest2_id_seq'::regclass);
  
  
  --
  -- Name: sbtest2_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest2
      ADD CONSTRAINT sbtest2_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_2; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_2 ON sbtest2 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest3; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest3 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest3 OWNER TO postgres;
  
  --
  -- Name: sbtest3_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest3_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest3_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest3_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest3_id_seq OWNED BY sbtest3.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest3 ALTER COLUMN id SET DEFAULT nextval('sbtest3_id_seq'::regclass);
  
  
  --
  -- Name: sbtest3_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest3
      ADD CONSTRAINT sbtest3_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_3; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_3 ON sbtest3 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest4; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest4 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest4 OWNER TO postgres;
  
  --
  -- Name: sbtest4_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest4_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest4_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest4_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest4_id_seq OWNED BY sbtest4.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest4 ALTER COLUMN id SET DEFAULT nextval('sbtest4_id_seq'::regclass);
  
  
  --
  -- Name: sbtest4_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest4
      ADD CONSTRAINT sbtest4_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_4; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_4 ON sbtest4 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest5; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest5 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest5 OWNER TO postgres;
  
  --
  -- Name: sbtest5_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest5_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest5_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest5_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest5_id_seq OWNED BY sbtest5.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest5 ALTER COLUMN id SET DEFAULT nextval('sbtest5_id_seq'::regclass);
  
  
  --
  -- Name: sbtest5_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest5
      ADD CONSTRAINT sbtest5_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_5; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_5 ON sbtest5 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest6; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest6 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest6 OWNER TO postgres;
  
  --
  -- Name: sbtest6_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest6_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest6_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest6_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest6_id_seq OWNED BY sbtest6.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest6 ALTER COLUMN id SET DEFAULT nextval('sbtest6_id_seq'::regclass);
  
  
  --
  -- Name: sbtest6_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest6
      ADD CONSTRAINT sbtest6_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_6; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_6 ON sbtest6 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest7; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest7 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest7 OWNER TO postgres;
  
  --
  -- Name: sbtest7_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest7_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest7_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest7_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest7_id_seq OWNED BY sbtest7.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest7 ALTER COLUMN id SET DEFAULT nextval('sbtest7_id_seq'::regclass);
  
  
  --
  -- Name: sbtest7_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest7
      ADD CONSTRAINT sbtest7_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_7; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_7 ON sbtest7 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest8; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest8 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest8 OWNER TO postgres;
  
  --
  -- Name: sbtest8_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest8_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest8_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest8_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest8_id_seq OWNED BY sbtest8.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest8 ALTER COLUMN id SET DEFAULT nextval('sbtest8_id_seq'::regclass);
  
  
  --
  -- Name: sbtest8_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest8
      ADD CONSTRAINT sbtest8_pkey PRIMARY KEY (id);
  
  
  --
  -- Name: k_8; Type: INDEX; Schema: public; Owner: postgres
  --
  
  CREATE INDEX k_8 ON sbtest8 USING btree (k);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  pg_dump: No matching tables were found
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
      queries performed:
          read:                            1400
          write:                           400
          other:                           200
          total:                           2000
      transactions:                        100    (* per sec.) (glob)
      read/write requests:                 1800   (* per sec.) (glob)
      other operations:                    200    (* per sec.) (glob)
      ignored errors:                      0      (* per sec.) (glob)
      reconnects:                          0      (* per sec.) (glob)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
      total time taken by event execution: *s (glob)
      response time:
           min:                               *ms (glob)
           avg:                               *ms (glob)
           max:                               *ms (glob)
           approx.  95 percentile:            *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  pg_dump: No matching tables were found
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  --
  -- PostgreSQL database dump
  --
  
  -- Dumped from database version 9.5.4
  -- Dumped by pg_dump version 9.5.4
  
  SET statement_timeout = 0;
  SET lock_timeout = 0;
  SET client_encoding = 'UTF8';
  SET standard_conforming_strings = on;
  SET check_function_bodies = false;
  SET client_min_messages = warning;
  SET row_security = off;
  
  SET search_path = public, pg_catalog;
  
  SET default_tablespace = '';
  
  SET default_with_oids = false;
  
  --
  -- Name: sbtest1; Type: TABLE; Schema: public; Owner: postgres
  --
  
  CREATE TABLE sbtest1 (
      id integer NOT NULL,
      k integer DEFAULT 0 NOT NULL,
      c character(120) DEFAULT ''::bpchar NOT NULL,
      pad character(60) DEFAULT ''::bpchar NOT NULL
  );
  
  
  ALTER TABLE sbtest1 OWNER TO postgres;
  
  --
  -- Name: sbtest1_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
  --
  
  CREATE SEQUENCE sbtest1_id_seq
      START WITH 1
      INCREMENT BY 1
      NO MINVALUE
      NO MAXVALUE
      CACHE 1;
  
  
  ALTER TABLE sbtest1_id_seq OWNER TO postgres;
  
  --
  -- Name: sbtest1_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
  --
  
  ALTER SEQUENCE sbtest1_id_seq OWNED BY sbtest1.id;
  
  
  --
  -- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest1 ALTER COLUMN id SET DEFAULT nextval('sbtest1_id_seq'::regclass);
  
  
  --
  -- Name: sbtest1_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
  --
  
  ALTER TABLE ONLY sbtest1
      ADD CONSTRAINT sbtest1_pkey PRIMARY KEY (id);
  
  
  --
  -- PostgreSQL database dump complete
  --
  
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Dropping table 'sbtest1'...
