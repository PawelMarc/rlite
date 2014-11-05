#include <math.h>
#include <stdlib.h>
#include "rlite.h"
#include "page_key.h"
#include "page_multi_string.h"
#include "type_zset.h"
#include "page_btree.h"
#include "page_list.h"
#include "page_skiplist.h"
#include "util.h"

static int update_zset(rlite *db, rl_btree *scores, long scores_page, rl_skiplist *skiplist, long skiplist_page)
{
	int retval = rl_write(db, &rl_data_type_btree_hash_md5_double, scores_page, scores);
	if (retval != RL_OK) {
		goto cleanup;
	}
	retval = rl_write(db, &rl_data_type_skiplist, skiplist_page, skiplist);
	if (retval != RL_OK) {
		goto cleanup;
	}
cleanup:
	return retval;
}

static int rl_zset_create(rlite *db, long levels_page_number, rl_btree **btree, long *btree_page, rl_skiplist **_skiplist, long *skiplist_page)
{
	rl_list *levels;
	rl_btree *scores;
	rl_skiplist *skiplist;
	long scores_page_number;
	long skiplist_page_number;

	long max_node_size = (db->page_size - 8) / 24;
	int retval = rl_btree_create(db, &scores, &rl_btree_type_hash_md5_double, max_node_size);
	if (retval != RL_OK) {
		goto cleanup;
	}
	scores_page_number = db->next_empty_page;
	retval = rl_write(db, &rl_data_type_btree_hash_md5_double, scores_page_number, scores);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = rl_skiplist_create(db, &skiplist);
	if (retval != RL_OK) {
		goto cleanup;
	}
	skiplist_page_number = db->next_empty_page;
	retval = rl_write(db, &rl_data_type_skiplist, skiplist_page_number, skiplist);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = rl_list_create(db, &levels, &list_long);
	if (retval != RL_OK) {
		goto cleanup;
	}
	retval = rl_write(db, &rl_data_type_list_long, levels_page_number, levels);
	if (retval != RL_OK) {
		goto cleanup;
	}

	long *scores_element = malloc(sizeof(long));
	*scores_element = scores_page_number;
	retval = rl_list_add_element(db, levels, scores_element, 0);
	if (retval != RL_OK) {
		goto cleanup;
	}

	long *skiplist_element = malloc(sizeof(long));
	*skiplist_element = skiplist_page_number;
	retval = rl_list_add_element(db, levels, skiplist_element, 1);
	if (retval != RL_OK) {
		goto cleanup;
	}

	if (btree) {
		*btree = scores;
	}
	if (btree_page) {
		*btree_page = scores_page_number;
	}
	if (_skiplist) {
		*_skiplist = skiplist;
	}
	if (skiplist_page) {
		*skiplist_page = skiplist_page_number;
	}
cleanup:
	return retval;
}

static int rl_zset_read(rlite *db, long levels_page_number, rl_btree **btree, long *btree_page, rl_skiplist **skiplist, long *skiplist_page)
{
	void *tmp;
	long scores_page_number, skiplist_page_number;
	rl_list *levels;
	int retval = rl_read(db, &rl_data_type_list_long, levels_page_number, &list_long, &tmp, 1);
	if (retval != RL_FOUND) {
		goto cleanup;
	}
	levels = tmp;
	if (btree || btree_page) {
		retval = rl_list_get_element(db, levels, &tmp, 0);
		if (retval != RL_FOUND) {
			goto cleanup;
		}
		scores_page_number = *(long *)tmp;
		if (btree) {
			retval = rl_read(db, &rl_data_type_btree_hash_md5_double, scores_page_number, &rl_btree_type_hash_md5_double, &tmp, 1);
			if (retval != RL_FOUND) {
				goto cleanup;
			}
			*btree = tmp;
		}
		if (btree_page) {
			*btree_page = scores_page_number;
		}
	}
	if (skiplist || skiplist_page) {
		retval = rl_list_get_element(db, levels, &tmp, 1);
		if (retval != RL_FOUND) {
			goto cleanup;
		}
		skiplist_page_number = *(long *)tmp;
		if (skiplist) {
			retval = rl_read(db, &rl_data_type_skiplist, skiplist_page_number, NULL, &tmp, 1);
			if (retval != RL_FOUND) {
				goto cleanup;
			}
			*skiplist = tmp;
		}
		if (skiplist_page) {
			*skiplist_page = skiplist_page_number;
		}
	}
	retval = RL_OK;
cleanup:
	return retval;
}

static int rl_zset_get_objects(rlite *db, unsigned char *key, long keylen, rl_btree **btree, long *btree_page, rl_skiplist **skiplist, long *skiplist_page, int create)
{
	long levels_page_number;
	int retval;
	if (create) {
		retval = rl_key_get_or_create(db, key, keylen, RL_TYPE_ZSET, &levels_page_number);
		if (retval != RL_FOUND && retval != RL_NOT_FOUND) {
			goto cleanup;
		}
		else if (retval == RL_NOT_FOUND) {
			retval = rl_zset_create(db, levels_page_number, btree, btree_page, skiplist, skiplist_page);
		}
		else {
			retval = rl_zset_read(db, levels_page_number, btree, btree_page, skiplist, skiplist_page);
		}
	}
	else {
		unsigned char type;
		retval = rl_key_get(db, key, keylen, &type, &levels_page_number);
		if (retval != RL_FOUND) {
			goto cleanup;
		}
		if (type != RL_TYPE_ZSET) {
			retval = RL_WRONG_TYPE;
			goto cleanup;
		}
		retval = rl_zset_read(db, levels_page_number, btree, btree_page, skiplist, skiplist_page);
	}
cleanup:
	return retval;
}

static int add_member(rlite *db, rl_btree *scores, rl_skiplist *skiplist, double score, unsigned char *data, long datalen)
{
	void *value_ptr = malloc(sizeof(double));
	*(double *)value_ptr = score;
	unsigned char *digest = malloc(sizeof(unsigned char) * 16);
	int retval = md5(data, datalen, digest);
	if (retval != RL_OK) {
		if (retval == RL_FOUND) {
			retval = RL_UNEXPECTED;
		}
		goto cleanup;
	}
	retval = rl_btree_add_element(db, scores, digest, value_ptr);
	if (retval != RL_OK) {
		goto cleanup;
	}
	retval = rl_skiplist_add(db, skiplist, score, data, datalen);
	if (retval != RL_OK) {
		goto cleanup;
	}
cleanup:
	return retval;
}

int rl_zadd(rlite *db, unsigned char *key, long keylen, double score, unsigned char *data, long datalen)
{
	rl_btree *scores;
	rl_skiplist *skiplist;
	long scores_page, skiplist_page;
	int retval = rl_zset_get_objects(db, key, keylen, &scores, &scores_page, &skiplist, &skiplist_page, 1);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = add_member(db, scores, skiplist, score, data, datalen);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = update_zset(db, scores, scores_page, skiplist, skiplist_page);
	if (retval != RL_OK) {
		goto cleanup;
	}
cleanup:
	return retval;
}

static int rl_get_zscore(rlite *db, rl_btree *scores, unsigned char *data, long datalen, double *score)
{
	unsigned char *digest = NULL;
	int retval;
	digest = malloc(sizeof(unsigned char) * 16);
	if (digest == NULL) {
		retval = RL_OUT_OF_MEMORY;
		goto cleanup;
	}
	retval = md5(data, datalen, digest);
	if (retval != RL_OK) {
		goto cleanup;
	}
	void *value;
	retval = rl_btree_find_score(db, scores, digest, &value, NULL, NULL);
	if (retval != RL_FOUND) {
		goto cleanup;
	}
	*score = *(double *)value;
	retval = RL_FOUND;
cleanup:
	free(digest);
	return retval;
}

int rl_zscore(rlite *db, unsigned char *key, long keylen, unsigned char *data, long datalen, double *score)
{
	rl_btree *scores;
	int retval = rl_zset_get_objects(db, key, keylen, &scores, NULL, NULL, NULL, 0);
	if (retval != RL_OK) {
		return retval;
	}
	return rl_get_zscore(db, scores, data, datalen, score);
}

int rl_zrank(rlite *db, unsigned char *key, long keylen, unsigned char *data, long datalen, long *rank)
{
	double score;
	rl_btree *scores;
	rl_skiplist *skiplist;
	int retval = rl_zset_get_objects(db, key, keylen, &scores, NULL, &skiplist, NULL, 0);
	if (retval != RL_OK) {
		return retval;
	}
	retval = rl_get_zscore(db, scores, data, datalen, &score);
	if (retval != RL_FOUND) {
		return retval;
	}
	retval = rl_skiplist_first_node(db, skiplist, score, 0, data, datalen, NULL, rank);
	if (retval != RL_FOUND) {
		return retval;
	}
	return retval;
}

int rl_zcard(rlite *db, unsigned char *key, long keylen, long *card)
{
	rl_skiplist *skiplist;
	int retval = rl_zset_get_objects(db, key, keylen, NULL, NULL, &skiplist, NULL, 0);
	if (retval != RL_OK) {
		return retval;
	}
	*card = skiplist->size;
	return RL_OK;
}

int rl_zcount(rlite *db, unsigned char *key, long keylen, rl_zrangespec *range, long *count)
{
	rl_skiplist *skiplist;
	long maxrank, minrank;
	int retval;
	if (range->max < range->min) {
		*count = 0;
		retval = RL_OK;
		goto cleanup;
	}

	retval = rl_zset_get_objects(db, key, keylen, NULL, NULL, &skiplist, NULL, 0);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = rl_skiplist_first_node(db, skiplist, range->max, !range->maxex, NULL, 0, NULL, &maxrank);
	if (retval == RL_NOT_FOUND) {
		maxrank = skiplist->size;
	}
	else if (retval != RL_FOUND) {
		goto cleanup;
	}
	retval = rl_skiplist_first_node(db, skiplist, range->min, range->minex, NULL, 0, NULL, &minrank);
	if (retval == RL_NOT_FOUND) {
		minrank = skiplist->size;
	}
	else if (retval != RL_FOUND) {
		goto cleanup;
	}

	*count = maxrank - minrank;
	retval = RL_OK;
cleanup:
	return retval;
}

int rl_zrange(rlite *db, unsigned char *key, long keylen, long start, long end, rl_zset_iterator **iterator)
{
	long card, size, node_page;
	rl_skiplist *skiplist;

	int retval = rl_zcard(db, key, keylen, &card);
	if (retval != RL_OK) {
		return retval;
	}

	retval = rl_zset_get_objects(db, key, keylen, NULL, NULL, &skiplist, NULL, 0);
	if (start < 0) {
		start += card;
		if (start < 0) {
			start = 0;
		}
	}
	if (end < 0) {
		end += card;
	}
	if (start > end || start >= card) {
		size = 0;
		goto cleanup;
	}
	if (end >= card) {
		end = card - 1;
	}

	size = end - start + 1;

	retval = rl_skiplist_node_by_rank(db, skiplist, start, NULL, &node_page);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = rl_skiplist_iterator_create(db, iterator, skiplist, node_page, 1, size);
cleanup:
	return retval;
}

int rl_zset_iterator_next(rl_zset_iterator *iterator, double *score, unsigned char **data, long *datalen)
{
	if ((!data && datalen) || (data && !datalen)) {
		fprintf(stderr, "Expected to receive either data and datalen or neither\n");
		return RL_UNEXPECTED;
	}

	rl_skiplist_node *node;
	int retval = rl_skiplist_iterator_next(iterator, &node);
	if (retval != RL_OK) {
		goto cleanup;
	}

	if (data) {
		retval = rl_multi_string_get(iterator->db, node->value, data, datalen);
		if (retval != RL_OK) {
			goto cleanup;
		}
	}

	if (score) {
		*score = node->score;
	}
cleanup:
	return retval;
}

int rl_zset_iterator_destroy(rl_zset_iterator *iterator)
{
	return rl_skiplist_iterator_destroy(iterator->db, iterator);
}

static int delete_zset(rlite *db, unsigned char *key, long keylen, rl_skiplist *skiplist, long skiplist_page, rl_btree *scores, long scores_page)
{
	int retval;
	if (skiplist->size == 0) {
		retval = rl_delete(db, skiplist_page);
		if (retval != RL_OK) {
			goto cleanup;
		}
		retval = rl_skiplist_destroy(db, skiplist);
		if (retval != RL_OK) {
			goto cleanup;
		}
		retval = rl_delete(db, scores_page);
		if (retval != RL_OK) {
			goto cleanup;
		}
		retval = rl_btree_destroy(db, scores);
		if (retval != RL_OK) {
			goto cleanup;
		}
		retval = rl_key_delete(db, key, keylen);
		if (retval != RL_OK) {
			goto cleanup;
		}
	}
	else {
		retval = RL_NOT_IMPLEMENTED;
	}
cleanup:
	return retval;
}

static int remove_member(rlite *db, rl_btree *scores, rl_skiplist *skiplist, unsigned char *member, long member_len)
{
	double score;
	void *tmp;
	unsigned char digest[16];
	int retval = md5(member, member_len, digest);
	if (retval != RL_OK) {
		goto cleanup;
	}
	retval = rl_btree_find_score(db, scores, digest, &tmp, NULL, NULL);
	if (retval != RL_FOUND && retval != RL_NOT_FOUND) {
		goto cleanup;
	}
	if (retval == RL_FOUND) {
		retval = rl_btree_remove_element(db, scores, digest);
		if (retval != RL_OK) {
			goto cleanup;
		}
		score = *(double *)tmp;
		retval = rl_skiplist_delete(db, skiplist, score, member, member_len);
		if (retval != RL_OK) {
			goto cleanup;
		}
		free(tmp);
	}
cleanup:
	return retval;
}

int rl_zrem(rlite *db, unsigned char *key, long keylen, long members_size, unsigned char **members, long *members_len, long *changed)
{
	rl_btree *scores;
	rl_skiplist *skiplist;
	long scores_page, skiplist_page;
	int retval = rl_zset_get_objects(db, key, keylen, &scores, &scores_page, &skiplist, &skiplist_page, 1);
	if (retval != RL_OK) {
		goto cleanup;
	}
	long i;
	long _changed = 0;
	for (i = 0; i < members_size; i++) {
		retval = remove_member(db, scores, skiplist, members[i], members_len[i]);
		if (retval != RL_OK && retval != RL_NOT_FOUND) {
			goto cleanup;
		}
		else if (retval == RL_OK) {
			_changed++;
		}
	}

	if (skiplist->size == 0) {
		retval = delete_zset(db, key, keylen, skiplist, skiplist_page, scores, scores_page);
		if (retval != RL_OK) {
			goto cleanup;
		}
	}
	else if (_changed) {
		retval = update_zset(db, scores, scores_page, skiplist, skiplist_page);
		if (retval != RL_OK) {
			goto cleanup;
		}
	}

	*changed = _changed;
	retval = RL_OK;
cleanup:
	return retval;
}

int rl_zincrby(rlite *db, unsigned char *key, long keylen, double score, unsigned char *data, long datalen, double *newscore)
{
	double existing_score;
	rl_btree *scores;
	rl_skiplist *skiplist;
	long scores_page, skiplist_page;
	int retval = rl_zset_get_objects(db, key, keylen, &scores, &scores_page, &skiplist, &skiplist_page, 1);
	if (retval != RL_OK) {
		return retval;
	}
	retval = rl_get_zscore(db, scores, data, datalen, &existing_score);
	if (retval != RL_FOUND && retval != RL_NOT_FOUND) {
		goto cleanup;
	}
	else if (retval == RL_FOUND) {
		score += existing_score;
		retval = remove_member(db, scores, skiplist, data, datalen);
		if (retval != RL_OK) {
			goto cleanup;
		}
	}

	retval = add_member(db, scores, skiplist, score, data, datalen);
	if (retval != RL_OK) {
		goto cleanup;
	}

	retval = update_zset(db, scores, scores_page, skiplist, skiplist_page);
	if (retval != RL_OK) {
		goto cleanup;
	}

	if (newscore) {
		*newscore = score;
	}
cleanup:
	return retval;
}