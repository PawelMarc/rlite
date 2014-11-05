#include <stdlib.h>
#include <string.h>
#include "../rlite.h"
#include "../type_zset.h"
#include "../page_key.h"
#include "test_util.h"

int basic_test_zadd_zscore(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zscore %d\n", _commit);

	rlite *db = setup_db(_commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	double score = 1.41, score2;
	unsigned char *data = (unsigned char *)"my data";
	long datalen = strlen((char *)data);

	retval = rl_zadd(db, key, keylen, score, data, datalen);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd %d\n", retval);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	retval = rl_zscore(db, key, keylen, data, datalen, &score2);
	if (retval != RL_FOUND) {
		fprintf(stderr, "Unable to zscore %d\n", retval);
		return 1;
	}

	if (score != score2) {
		fprintf(stderr, "Expected score %lf to match score2 %lf\n", score, score2);
		return 1;
	}

	fprintf(stderr, "End basic_test_zadd_zscore\n");
	rl_close(db);
	return 0;
}

int basic_test_zadd_zscore2(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zscore2 %d\n", _commit);

	rlite *db = setup_db(_commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	double score = 8913.109, score2;
	unsigned char *data = (unsigned char *)"my data";
	long datalen = strlen((char *)data);
	unsigned char *data2 = (unsigned char *)"my data2";
	long datalen2 = strlen((char *)data2);
	long card;

	retval = rl_zadd(db, key, keylen, score, data, datalen);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd %d\n", retval);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	retval = rl_zcard(db, key, keylen, &card);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zcard %d\n", retval);
		return 1;
	}
	if (card != 1) {
		fprintf(stderr, "Expected zcard to be 1, got %ld instead\n", card);
		return 1;
	}

	retval = rl_zadd(db, key, keylen, score, data2, datalen2);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd a second time %d\n", retval);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	retval = rl_zcard(db, key, keylen, &card);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zcard a second time %d\n", retval);
		return 1;
	}
	if (card != 2) {
		fprintf(stderr, "Expected zcard to be 2, got %ld instead\n", card);
		return 1;
	}

	retval = rl_zscore(db, key, keylen, data, datalen, &score2);
	if (retval != RL_FOUND) {
		fprintf(stderr, "Unable to zscore %d\n", retval);
		return 1;
	}

	if (score != score2) {
		fprintf(stderr, "Expected score %lf to match score2 %lf\n", score, score2);
		return 1;
	}

	retval = rl_zscore(db, key, keylen, data2, datalen2, &score2);
	if (retval != RL_FOUND) {
		fprintf(stderr, "Unable to zscore %d\n", retval);
		return 1;
	}

	if (score != score2) {
		fprintf(stderr, "Expected score %lf to match score2 %lf\n", score, score2);
		return 1;
	}

	fprintf(stderr, "End basic_test_zadd_zscore2\n");
	rl_close(db);
	return 0;
}

int basic_test_zadd_zrank(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zrank %d\n", _commit);

	rlite *db = setup_db(_commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	double score = 8913.109;
	long rank;
	unsigned char *data = (unsigned char *)"my data";
	long datalen = strlen((char *)data);
	unsigned char *data2 = (unsigned char *)"my data2";
	long datalen2 = strlen((char *)data2);

	retval = rl_zadd(db, key, keylen, score, data, datalen);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd %d\n", retval);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	retval = rl_zadd(db, key, keylen, score, data2, datalen2);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd a second time %d\n", retval);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	retval = rl_zrank(db, key, keylen, data, datalen, &rank);
	if (retval != RL_FOUND) {
		fprintf(stderr, "Unable to zrank %d\n", retval);
		return 1;
	}

	if (0 != rank) {
		fprintf(stderr, "Expected rank %d to be %ld\n", 0, rank);
		return 1;
	}

	retval = rl_zrank(db, key, keylen, data2, datalen2, &rank);
	if (retval != RL_FOUND) {
		fprintf(stderr, "Unable to zrank %d\n", retval);
		return 1;
	}

	if (1 != rank) {
		fprintf(stderr, "Expected rank %d to be %ld\n", 1, rank);
		return 1;
	}

	fprintf(stderr, "End basic_test_zadd_zrank\n");
	rl_close(db);
	return 0;
}

int basic_test_zadd_zrange()
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zrange\n");

	rlite *db = setup_db(0, 1);

	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);

	long i, setdatalen = 1;
	unsigned char setdata[1];
	for (i = 0; i < 200; i++) {
		setdata[0] = i;

		retval = rl_zadd(db, key, keylen, i * 10.5, setdata, setdatalen);
		if (retval != RL_OK) {
			fprintf(stderr, "Unable to zadd %d\n", retval);
			return 1;
		}
	}

	unsigned char *data;
	long datalen;
	rl_zset_iterator* iterator;
	retval = rl_zrange(db, key, keylen, 0, -1, &iterator);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zrange %d\n", retval);
		return 1;
	}
	if (iterator->size != 200) {
		fprintf(stderr, "Expected size to be 200, got %ld instead\n", iterator->size);
		return 1;
	}
	rl_zset_iterator_destroy(iterator);

	retval = rl_zrange(db, key, keylen, 0, 0, &iterator);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zrange %d\n", retval);
		return 1;
	}
	if (iterator->size != 1) {
		fprintf(stderr, "Expected size to be 1, got %ld instead\n", iterator->size);
		return 1;
	}
	retval = rl_zset_iterator_next(iterator, NULL, &data, &datalen);
	if (retval != RL_OK) {
		fprintf(stderr, "Failed to fetch next element in zset iterator\n");
		return 1;
	}
	if (data[0] != 0) {
		fprintf(stderr, "Expected data to be 0, got %d instead\n", data[0]);
		return 1;
	}
	if (datalen != 1) {
		fprintf(stderr, "Expected data to be 1, got %ld instead\n", datalen);
		return 1;
	}
	free(data);
	rl_zset_iterator_destroy(iterator);

	rl_close(db);
	fprintf(stderr, "End basic_test_zadd_zrange\n");
	return 0;
}

int basic_test_zadd_zrem(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zrem %d\n", _commit);

	rlite *db = setup_db(_commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	double score = 8913.109;
	long rank;
	unsigned char *data = (unsigned char *)"my data";
	long datalen = strlen((char *)data);
	unsigned char *data2 = (unsigned char *)"my data2";
	long datalen2 = strlen((char *)data2);

	retval = rl_zadd(db, key, keylen, score, data, datalen);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd %d\n", retval);
		return 1;
	}

	retval = rl_zadd(db, key, keylen, score, data2, datalen2);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zadd a second time %d\n", retval);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	unsigned char *members[1] = {data};
	long members_len[1] = {datalen};
	long changed;
	retval = rl_zrem(db, key, keylen, 1, members, members_len, &changed);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zrem %d\n", retval);
		return 1;
	}
	if (changed != 1) {
		fprintf(stderr, "Expected to have removed 1 element, got %ld\n", changed);
		return 1;
	}

	retval = rl_zrank(db, key, keylen, data, datalen, &rank);
	if (retval != RL_NOT_FOUND) {
		fprintf(stderr, "Unable to zrank %d\n", retval);
		return 1;
	}

	retval = rl_zscore(db, key, keylen, data, datalen, NULL);
	if (retval != RL_NOT_FOUND) {
		fprintf(stderr, "Unable to zscore %d\n", retval);
		return 1;
	}

	retval = rl_zrank(db, key, keylen, data2, datalen2, &rank);
	if (retval != RL_FOUND) {
		fprintf(stderr, "Unable to zrank %d\n", retval);
		return 1;
	}

	if (0 != rank) {
		fprintf(stderr, "Expected rank %d to be %ld\n", 0, rank);
		return 1;
	}

	unsigned char *members2[2] = {data, data2};
	long members_len2[2] = {datalen, datalen2};
	retval = rl_zrem(db, key, keylen, 2, members2, members_len2, &changed);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zrem %d\n", retval);
		return 1;
	}
	if (changed != 1) {
		fprintf(stderr, "Expected to have removed 1 element, got %ld\n", changed);
		return 1;
	}

	retval = rl_key_get(db, key, keylen, NULL, NULL);
	if (retval != RL_NOT_FOUND) {
		fprintf(stderr, "Expected not to find key after removing all zset elements, got %ld\n", changed);
		return 1;
	}

	fprintf(stderr, "End basic_test_zadd_zrem\n");
	rl_close(db);
	return 0;
}

#define ZCOUNT_SIZE 100
int basic_test_zadd_zcount(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zcount %d\n", _commit);

	rlite *db = setup_db(_commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	long datalen = 20;
	unsigned char *data = malloc(sizeof(unsigned char) * datalen);
	long i, count;

	for (i = 0; i < datalen; i++) {
		data[i] = i;
	}

	for (i = 0; i < ZCOUNT_SIZE; i++) {
		data[0] = i;
		retval = rl_zadd(db, key, keylen, i * 1.0, data, datalen);
		if (retval != RL_OK) {
			fprintf(stderr, "Unable to zadd %d\n", retval);
			return 1;
		}
	}
	free(data);

	if (_commit) {
		rl_commit(db);
	}

	rl_zrangespec range;
	range.min = -1000;
	range.minex = 0;
	range.max = 1000;
	range.maxex = 0;
	retval = rl_zcount(db, key, keylen, &range, &count);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zcount %d\n", retval);
		return 1;
	}
	if (count != ZCOUNT_SIZE) {
		fprintf(stderr, "Expected zcount to be %d, got %ld\n", ZCOUNT_SIZE, count);
		return 1;
	}

	range.min = 0;
	range.minex = 0;
	range.max = ZCOUNT_SIZE - 1;
	range.maxex = 0;
	retval = rl_zcount(db, key, keylen, &range, &count);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zcount %d\n", retval);
		return 1;
	}
	if (count != ZCOUNT_SIZE) {
		fprintf(stderr, "Expected zcount to be %d, got %ld\n", ZCOUNT_SIZE, count);
		return 1;
	}

	range.min = 0;
	range.minex = 1;
	range.max = ZCOUNT_SIZE - 1;
	range.maxex = 1;
	retval = rl_zcount(db, key, keylen, &range, &count);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zcount %d\n", retval);
		return 1;
	}
	if (count != ZCOUNT_SIZE - 2) {
		fprintf(stderr, "Expected zcount to be %d, got %ld\n", ZCOUNT_SIZE - 2, count);
		return 1;
	}

	range.min = 1;
	range.minex = 1;
	range.max = 2;
	range.maxex = 0;
	retval = rl_zcount(db, key, keylen, &range, &count);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zcount %d\n", retval);
		return 1;
	}
	if (count != 1) {
		fprintf(stderr, "Expected zcount to be %d, got %ld\n", 1, count);
		return 1;
	}

	fprintf(stderr, "End basic_test_zadd_zcount\n");
	rl_close(db);
	return 0;
}

int basic_test_zadd_zincrby(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_zadd_zincrby %d\n", _commit);

	rlite *db = setup_db(_commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	double score = 4.2;
	unsigned char *data = (unsigned char *)"my data";
	long datalen = strlen((char *)data);
	double newscore;

	retval = rl_zincrby(db, key, keylen, score, data, datalen, &newscore);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zincrby %d\n", retval);
		return 1;
	}
	if (newscore != score) {
		fprintf(stderr, "Expected new score %lf to match incremented score %lf\n", newscore, score);
		return 1;
	}

	if (_commit) {
		rl_commit(db);
	}

	retval = rl_zincrby(db, key, keylen, score, data, datalen, &newscore);
	if (retval != RL_OK) {
		fprintf(stderr, "Unable to zincrby %d\n", retval);
		return 1;
	}
	if (newscore != score * 2) {
		fprintf(stderr, "Expected new score %lf to match incremented twice score %lf\n", newscore, 2 * score);
		return 1;
	}

	fprintf(stderr, "End basic_test_zadd_zincrby\n");
	rl_close(db);
	return 0;
}

int main()
{
	int retval, i;
	for (i = 0; i < 2; i++) {
		retval = basic_test_zadd_zscore(i);
		if (retval != 0) {
			goto cleanup;
		}
		retval = basic_test_zadd_zscore2(i);
		if (retval != 0) {
			goto cleanup;
		}
		retval = basic_test_zadd_zrank(i);
		if (retval != 0) {
			goto cleanup;
		}
		retval = basic_test_zadd_zrem(i);
		if (retval != 0) {
			goto cleanup;
		}
		retval = basic_test_zadd_zcount(i);
		if (retval != 0) {
			goto cleanup;
		}
		retval = basic_test_zadd_zincrby(i);
		if (retval != 0) {
			goto cleanup;
		}
	}
	retval = basic_test_zadd_zrange();
	if (retval != 0) {
		goto cleanup;
	}
cleanup:
	return retval;
}