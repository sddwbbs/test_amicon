#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

#include <rte_memory.h>
#include <rte_malloc.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <sys/types.h>

#define ull unsigned long long int

const size_t NUM_OF_ELEMENTS = 1000000;
const int NUM_OF_PAIRS = 16000000;

typedef struct {
	ull key;
	ull value;
} Pair;

typedef struct {
	Pair pair;
	bool empty;
} TableElem;

typedef struct {
	TableElem* table1;
	TableElem* table2;
	int totalElements;
} HashTables;

ull generateRandomNumber(void);
bool initialize_hash_tables(HashTables* hashTables);
size_t hash1(ull key);
size_t hash2(ull key);
bool belong(const HashTables* hashTables, ull key);
int insert(HashTables* hashTables, Pair elem);
ull getDataFromKey(const HashTables* hashTables, ull key);
size_t getHashIdx(const HashTables* hashTables, ull key);
void printTables(const HashTables *hashTables);

ull generateRandomNumber(void) {
	ull lower = (ull)rand();
	ull upper = (ull)rand();

	return (upper << 32) | lower;
}

bool initialize_hash_tables(HashTables* hashTables) {
	hashTables->totalElements = 0;
    hashTables->table1 = (TableElem*)rte_malloc(NULL, sizeof(TableElem) * NUM_OF_ELEMENTS, 0);
    hashTables->table2 = (TableElem*)rte_malloc(NULL, sizeof(TableElem) * NUM_OF_ELEMENTS, 0);

	if (hashTables->table1 == NULL || hashTables->table2 == NULL)
		return false;

	for (size_t i = 0; i < NUM_OF_ELEMENTS; ++i) {
		hashTables->table1[i].pair.key = 1;
		hashTables->table1[i].pair.value = 1;
		hashTables->table1[i].empty = true;
		hashTables->table2[i].pair.key = 1;
		hashTables->table2[i].pair.value = 1;
		hashTables->table2[i].empty = true;
	}

	return true;
}

size_t hash1(const ull key) {
	return key % NUM_OF_ELEMENTS;
}

size_t hash2(const ull key) {
	return (key / NUM_OF_ELEMENTS) % NUM_OF_ELEMENTS;
}

bool belong(const HashTables* hashTables, const ull key) {
	const size_t index1 = hash1(key);
	const size_t index2 = hash2(key);

	return ((!hashTables->table1[index1].empty && hashTables->table1[index1].pair.key == key) ||
			(!hashTables->table2[index2].empty && hashTables->table2[index2].pair.key == key));
}

int insert(HashTables* hashTables, const Pair elem) {
	int i = 1, pass = 1;
	Pair curElem = elem, temp;

	if (belong(hashTables, elem.key)) return 0;

	while (i < 2 * (int)NUM_OF_ELEMENTS) {
		if (pass == 1) {
			if (hashTables->table1[hash1(curElem.key)].empty) {
				hashTables->table1[hash1(curElem.key)].pair.key = curElem.key;
				hashTables->table1[hash1(curElem.key)].pair.value = curElem.value;
				hashTables->table1[hash1(curElem.key)].empty = false;
				++hashTables->totalElements;
				return 1;
			}

			temp = hashTables->table1[hash1(curElem.key)].pair;
			hashTables->table1[hash1(curElem.key)].pair.key = curElem.key;
			hashTables->table1[hash1(curElem.key)].pair.value = curElem.value;
			curElem = temp;
			pass = 2;
		} else {
			if (hashTables->table2[hash2(curElem.key)].empty) {
				hashTables->table2[hash2(curElem.key)].pair.key = curElem.key;
				hashTables->table2[hash2(curElem.key)].pair.value = curElem.value;
				hashTables->table2[hash2(curElem.key)].empty = false;
				++hashTables->totalElements;
				return 1;
			}

			temp = hashTables->table2[hash2(curElem.key)].pair;
			hashTables->table2[hash2(curElem.key)].pair.key = curElem.key;
			hashTables->table2[hash2(curElem.key)].pair.value = curElem.value;
			curElem = temp;
			pass = 1;
		}
		++i;
	}

	return 0;
}

ull getDataFromKey(const HashTables* hashTables, const ull key) {
	const size_t index1 = hash1(key);
	const size_t index2 = hash2(key);
	ull data = 0;

	if (!hashTables->table1[index1].empty && hashTables->table1[index1].pair.key == key) {
		data = hashTables->table1[index1].pair.value;
		return data;
	}

	if (!hashTables->table2[index2].empty && hashTables->table2[index2].pair.key == key) {
		data = hashTables->table2[index2].pair.value;
		return data;
	}

	return data;
}

size_t getHashIdx(const HashTables* hashTables, const ull key) {
	const size_t index1 = hash1(key);
	const size_t index2 = hash2(key);
	size_t idx = 0;

	if (!hashTables->table1[index1].empty && hashTables->table1[index1].pair.key == key) {
		idx = index1;
		return idx;
	}

	if (!hashTables->table2[index2].empty && hashTables->table2[index2].pair.key == key) {
		idx = index2;
		return idx;
	}

	return idx;
}

void printTables(const HashTables *hashTables) {
	char isEmpty1;
	char isEmpty2;

	printf("Table1\t|\tTable2\n");
	printf("---------------------------------\n");
	for (size_t i = 0; i < NUM_OF_ELEMENTS; ++i) {
		isEmpty1 = hashTables->table1[i].empty ? 'e' : ' ';
		isEmpty2 = hashTables->table2[i].empty ? 'e' : ' ';

		printf("%c%llu\t|\t%c%llu\n",
			isEmpty1,
			hashTables->table1[i].empty ? 0 : hashTables->table1[i].pair.value,
			isEmpty2,
			hashTables->table2[i].empty ? 0 : hashTables->table2[i].pair.value);
	}
}

int main(int argc, char **argv) {
	int ret = rte_eal_init(argc, argv);
	HashTables hashTables;
	struct timespec res1,res2;
	int curKeys = 0;
	int hashSize;
	size_t hashIdx;
	ull data;

	srand (time(NULL));

	printf("cuchoo test\n");

	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	if (!initialize_hash_tables(&hashTables)) {
		printf("Failed to allocate memory");
		return -1;
	}

	printf("Time 0 ns\n");
	clock_gettime(CLOCK_REALTIME,&res1);

	clock_gettime(CLOCK_REALTIME, &res2);
	printf("%d k / %d k, time: %lu ns\n", curKeys, NUM_OF_PAIRS, res2.tv_nsec - res1.tv_nsec);

	for (int i = 0, j = 0; i < NUM_OF_PAIRS; ++i) {
		Pair elem;

		if (i - j == 1000000) {
			j = i;
			clock_gettime(CLOCK_REALTIME, &res2);
			printf("%d k / %d k, time: %lu ns\n", curKeys, NUM_OF_PAIRS, res2.tv_nsec - res1.tv_nsec);
		}

		if (i == 123456) {
			elem.key = 42;
			elem.value = 123456;
			if (insert(&hashTables, elem) == 1) ++curKeys;
			continue;
		}

		elem.key = generateRandomNumber();
		elem.value = generateRandomNumber();
		if (insert(&hashTables, elem) == 1) ++curKeys;
	}

	hashSize = sizeof(TableElem) * hashTables.totalElements;
	hashIdx = getHashIdx(&hashTables, 42);

	clock_gettime(CLOCK_REALTIME,&res1);
	data = getDataFromKey(&hashTables, 42);
	clock_gettime(CLOCK_REALTIME, &res2);

	printf("Hash size: %d, key: 42, hash idx: %lu, data: %llu, lookup time: %lu ns",
		hashSize,
		hashIdx,
		data,
		res2.tv_nsec - res1.tv_nsec);

	rte_eal_cleanup();

	return 0;
}