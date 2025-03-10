CREATE DATABASE links_db;

\c links_db

CREATE ROLE admin WITH LOGIN PASSWORD 'admin123';
CREATE ROLE guest WITH LOGIN PASSWORD 'guest123';

CREATE TABLE links
(
    id         BIGSERIAL PRIMARY KEY,
    original   TEXT                                NOT NULL,
    alias      VARCHAR(255)                        NOT NULL UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
);

ALTER TABLE links
    OWNER TO admin;
ALTER SEQUENCE links_id_seq OWNER TO admin;

CREATE OR REPLACE PROCEDURE drop_table()
    LANGUAGE plpgsql
AS
$$
BEGIN
    DROP TABLE IF EXISTS links CASCADE;
END;
$$;


CREATE OR REPLACE PROCEDURE create_user(
    username VARCHAR(255),
    password VARCHAR(255),
    is_admin BOOLEAN
)
    LANGUAGE plpgsql
    SECURITY DEFINER
AS
$$
BEGIN
    IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = username) THEN
        RAISE EXCEPTION 'User % already exists', username;
    END IF;

    EXECUTE format(
            'CREATE ROLE %I WITH LOGIN PASSWORD %L',
            username,
            password
            );

    IF is_admin THEN
        EXECUTE format('GRANT admin TO %I', username);
    ELSE
        EXECUTE format('GRANT guest TO %I', username);
    END IF;
END;
$$;

CREATE OR REPLACE FUNCTION is_admin()
    RETURNS BOOLEAN
    LANGUAGE plpgsql
    SECURITY DEFINER
AS
$$
BEGIN
    RETURN EXISTS (SELECT 1
                   FROM pg_roles
                   WHERE rolname = 'admin'
                     AND pg_has_role(CURRENT_USER, oid, 'member'));
END;
$$;

ALTER PROCEDURE create_user OWNER TO postgres;

CREATE OR REPLACE PROCEDURE clear_table()
    LANGUAGE plpgsql
AS
$$
BEGIN
    TRUNCATE TABLE links RESTART IDENTITY CASCADE;
END;
$$;

CREATE OR REPLACE PROCEDURE add_link(
    original_text TEXT,
    alias_text VARCHAR(255))
    LANGUAGE plpgsql
AS
$$
BEGIN
    INSERT INTO links (original, alias) VALUES (original_text, alias_text);
END;
$$;

CREATE OR REPLACE FUNCTION search_by_original(search_text TEXT)
    RETURNS TABLE
            (
                id         BIGINT,
                original   TEXT,
                alias      VARCHAR(255),
                created_at TIMESTAMP
            )
    LANGUAGE plpgsql
    SECURITY DEFINER
AS
$$
BEGIN
    RETURN QUERY
        SELECT id, original, alias, created_at
        FROM links
        WHERE original ILIKE '%' || search_text || '%';
END;
$$;

CREATE OR REPLACE PROCEDURE update_link(
    link_id BIGINT,
    new_original TEXT,
    new_alias VARCHAR(255))
    LANGUAGE plpgsql
AS
$$
BEGIN
    UPDATE links
    SET original = new_original,
        alias    = new_alias
    WHERE id = link_id;
END;
$$;

CREATE OR REPLACE PROCEDURE delete_by_alias(alias_text VARCHAR(255))
    LANGUAGE plpgsql
AS
$$
BEGIN
    DELETE FROM links WHERE alias = alias_text;
END;
$$;

REVOKE ALL ON TABLE links FROM PUBLIC;
REVOKE ALL ON SEQUENCE links_id_seq FROM PUBLIC;

GRANT USAGE ON SCHEMA public TO admin, guest;
GRANT EXECUTE ON PROCEDURE drop_table, clear_table, add_link, update_link, delete_by_alias TO admin;
GRANT EXECUTE ON FUNCTION search_by_original TO guest;

ALTER DEFAULT PRIVILEGES IN SCHEMA public REVOKE ALL ON TABLES FROM PUBLIC;