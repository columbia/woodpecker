#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <db.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

const char *ENV_DIRECTORY = "TXNAPP";

#ifdef TEST_TXN
int env_open(DB_ENV **dbenvp) {
    DB_ENV *dbenv;
    int ret;

    if ((ret = db_env_create(&dbenv, 0)) != 0) {
        fprintf(stderr,
                "txnapp: db_env_create: %s\n", db_strerror(ret));
        return 1;
    }

    dbenv->set_errpfx(dbenv, "txnapp");

    if ((ret = dbenv->open(dbenv, ENV_DIRECTORY,
                    DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL
                    | DB_INIT_TXN | DB_RECOVER | DB_THREAD,
                    S_IRUSR | S_IWUSR)) != 0) {
        dbenv->err(dbenv, ret, "dbenv->open: %s", ENV_DIRECTORY);
        return 1;
    }

    *dbenvp = dbenv;
    return 0;
}

int env_dir_create() {
    struct stat sb;

    if (stat(ENV_DIRECTORY, &sb) == 0)
        return 0;

    if (mkdir(ENV_DIRECTORY, S_IRWXU) != 0) {
        fprintf(stderr, "txnapp: mkdir: %s: %s\n", ENV_DIRECTORY, strerror(errno));
        return 1;
    }
    return 0;
}

int db_open(DB_ENV *dbenv, DB **dbp, char *name, int dups) {
    DB *db;
    int ret;

    if ((ret = db_create(&db, dbenv, 0)) != 0) {
        dbenv->err(dbenv, ret, "db_create");
        return ret;
    }

    if (dups && (ret = db->set_flags(db, DB_DUP)) != 0) {
        dbenv->err(dbenv, ret, "db->set_flags: DB_DUP");
        return ret;
    }

    if ((ret = db->open(db, NULL, name, NULL,
                    DB_BTREE, DB_CREATE | DB_THREAD, S_IRUSR | S_IWUSR)) != 0) {
        dbenv->err(dbenv, ret, "db->open: %s", name);
        return ret;
    }

    *dbp = db;
    return 0;
}

int add_fruit(DB_ENV *dbenv, DB *db, char *fruit, char *name) {
    DBT key, data;
    DB_TXN *tid;
    int fail, ret, t_ret;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = fruit;
    key.size = strlen(fruit);
    data.data = name;
    data.size = strlen(name);

    for (fail = 0;;) {
        if ((ret = txn_begin(dbenv, NULL, &tid, )) != 0) {
            dbenv->err(dbenv, ret, "txn_begin");
            return 1;
        }

        switch (ret = db->put(db, tid, &key, &data, 0)) {
            case 0:
                if ((ret = txn_commit(tid, 0)) != 0) {
                    dbenv->err(dbenv, ret, "txn_commit");
                    return 1;
                }
                return 0;
            case DB_LOCK_DEADLOCK:
            default:
                if ((t_ret = txn_abort(tid)) != 0) {
                    dbenv->err(dbenv, t_ret, "txn_abort");
                    return 1;
                }
                if (++fail == MAXIMUM_RETRY)
                    return ret;
                break;
        }
    }
}

int test_txn() {
    DB_ENV *dbenv;
    DB *db_cats, *db_color, *db_fruit;

    int ret = env_dir_create();
    if (ret) return ret;
    ret = env_open(&dbenv);
    if (ret) return ret;

    ret = db_open(dbenv, &db_fruit, "fruit", 0);
    if (ret) return ret;
    ret = db_open(dbenv, &db_color, "color", 0);
    if (ret) return ret;
    ret = db_open(dbenv, &db_cats, "cats", 1);
    if (ret) return ret;

    ret = add_fruit(dbenv, db_fruit, "apple", "yellow delicious");

    return 0;
}
#endif

int test_db(const char *dbfile, int db_type) {
    DB *dbp;
    int ret, t_ret;

    if ((ret = db_create(&dbp, NULL, 0)) != 0) {
        fprintf(stderr, "db_create: %s\n", db_strerror(ret));
        return 1;
    }
/*
    if ((ret = dbp->open(dbp, NULL, dbfile, NULL, db_type, DB_CREATE, 0664)) != 0) {
        dbp->err(dbp, ret, "%s", dbfile);
        goto err;
    }

    DBT key, data;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = "fruit";
    key.size = sizeof("fruit");
    data.data = "apple";
    data.size = sizeof("apple");

    if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) == 0)
        printf("db: %s: key stored.\n", (char *)key.data);
    else {
        dbp->err(dbp, ret, "DB->put");
        goto err;
    }

    if ((ret = dbp->get(dbp, NULL, &key, &data, 0)) == 0)
        printf("db: %s: key retrieved: data was %s.\n",
                (char *)key.data, (char *)data.data);
    else {
        dbp->err(dbp, ret, "DB->get");
        goto err;
    }

    if ((ret = dbp->del(dbp, NULL, &key, 0)) == 0)
        printf("db: %s: key was deleted.\n", (char *)key.data);
    else {
        dbp->err(dbp, ret, "DB->del");
        goto err;
    }
err:
    if ((t_ret = dbp->close(dbp, 0)) != 0 && ret == 0)
        ret = t_ret;
*/

    return ret;
}

int main(int argc, char **argv) {
    int ret;
    int types[] = {DB_BTREE, DB_HASH};
    char *files[] = {"btree.db", "hash.db"};
    char *names[] = {"BTree", "Hash"};
/*
    int i;
    for (i=0; i<sizeof(types) / sizeof(int); i++) {
        printf("Testing format %s, file %s\n", names[i], files[i]);
        ret = test_db(files[i], types[i]);
        if (ret)
        {
            printf("test type %d failed: %d\n", i, ret);
            return ret;
        }
    }
*/
    DB *dbp;

    db_create(&dbp, NULL, 0);

#ifdef TEST_TXN
    printf("Testing TXN:\n");
    ret = test_txn();
    if (ret)
        printf("Test FAIL: %d\n", ret);
#endif
    return 0;
}
