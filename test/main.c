#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <rte_memory.h>
#include <rte_malloc.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <sys/types.h>

#define ull unsigned long long int

const size_t NUM_OF_ELEMENTS = 1000000;

typedef struct {
	ull key;
	ull value;
} Pair;

typedef struct {
	ull value;
	int sign;
} BigNumber;

typedef struct {
	BigNumber* table1;
	BigNumber* table2;
} HashTable;

ull generateRandomNumber(void);
bool initialize_hash_table(HashTable* hashTable);
size_t hash1(ull key);
size_t hash2(ull key);
bool belong(const HashTable* hashTable, Pair elem);
int insert(HashTable* hashTable, Pair elem);
void printTables(HashTable *hashTable);

ull generateRandomNumber(void) {
	ull lower = (ull)rand();
	ull upper = (ull)rand();

	return (upper << 32) | lower;
}

bool initialize_hash_table(HashTable* hashTable) {
    hashTable->table1 = (BigNumber*)rte_malloc(NULL, sizeof(BigNumber) * NUM_OF_ELEMENTS, 0);
    hashTable->table2 = (BigNumber*)rte_malloc(NULL, sizeof(BigNumber) * NUM_OF_ELEMENTS, 0);

	if (hashTable->table1 == NULL || hashTable->table2 == NULL) {
		printf("Failed to allocate memory");
		return false;
	}

	for (size_t i = 0; i < NUM_OF_ELEMENTS; ++i) {
		hashTable->table1[i].value = 1;
		hashTable->table1[i].sign = -1;
		hashTable->table2[i].value = 1;
		hashTable->table2[i].sign = -1;
	}

	return true;
}

size_t hash1(const ull key) {
	return key % NUM_OF_ELEMENTS;
}

size_t hash2(const ull key) {
	return (key / NUM_OF_ELEMENTS) % NUM_OF_ELEMENTS;
}

bool belong(const HashTable* hashTable, const Pair elem) {
	const size_t index1 = hash1(elem.key);
	const size_t index2 = hash2(elem.key);

	return ((hashTable->table1[index1].sign >= 0 && hashTable->table1[index1].value == elem.value) ||
			(hashTable->table2[index2].sign >= 0 && hashTable->table2[index2].value == elem.value));
}

int insert(HashTable* hashTable, const Pair elem) {
	int i = 1, pass = 1;
	Pair elemCopy = elem;
	ull temp;

	if (belong(hashTable, elem)) return 0;

	while (i < 2 * (int)NUM_OF_ELEMENTS) {
		if (pass == 1) {
			if (hashTable->table1[hash1(elemCopy.key)].sign < 0) {
				hashTable->table1[hash1(elemCopy.key)].value = elemCopy.value;
				hashTable->table1[hash1(elemCopy.key)].sign = 1;
				return 1;
			}

			temp = hashTable->table1[hash1(elemCopy.key)].value;
			hashTable->table1[hash1(elemCopy.key)].value = elemCopy.value;
			elemCopy.value = temp;
			pass = 2;
		} else {
			if (hashTable->table2[hash2(elemCopy.key)].sign < 0) {
				hashTable->table2[hash2(elemCopy.key)].value = elemCopy.value;
				hashTable->table2[hash2(elemCopy.key)].sign = 1;
				return 1;
			}

			temp = hashTable->table2[hash2(elemCopy.key)].value;
			hashTable->table2[hash2(elemCopy.key)].value = elemCopy.value;
			elemCopy.value = temp;
			pass = 1;
		}
		++i;
	}

	return 0;
}

void printTables(HashTable *hashTable) {
	const char negativeNumSign = '-';
	const char positiveNumSign = ' ';
	char sign1;
	char sign2;

	printf("Table1\t|\tTable2\n");
	printf("---------------------------------\n");
	for (size_t i = 0; i < NUM_OF_ELEMENTS; ++i) {
		sign1 = (hashTable->table1[i].sign >= 0) ? positiveNumSign : negativeNumSign;
		sign2 = (hashTable->table2[i].sign >= 0) ? positiveNumSign : negativeNumSign;

		printf("%c%llu\t|\t%c%llu\n", sign1, hashTable->table1[i].value, sign2, hashTable->table2[i].value);
	}
}

int main(int argc, char **argv) {
	int ret = rte_eal_init(argc, argv);
	HashTable hashTable;
	struct timespec res1,res2;
	int curKeys = 0;
	int totalKeys = 0;

	printf("cuchoo test\n");

	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	if (!initialize_hash_table(&hashTable)) {
		return -1;
	}

	printf("Time 0 ns\n");
	clock_gettime(CLOCK_REALTIME,&res1);

	clock_gettime(CLOCK_REALTIME, &res2);
	printf("%d k / %d k, time: %lu ns\n", curKeys, totalKeys, res2.tv_nsec - res1.tv_nsec);

	for (int i = 0, j = 0; i < 160000; ++i) {
		Pair elem;

		if (i - j == 10000) {
			j = i;
			clock_gettime(CLOCK_REALTIME, &res2);
			printf("%d k / %d k, time: %lu ns\n", curKeys, totalKeys, res2.tv_nsec - res1.tv_nsec);
		}

		if (i == 1234) {
			elem.key = 42;
			elem.value = 123456;
			if (insert(&hashTable, elem) == 1) ++curKeys;
			continue;
		}

		elem.key = generateRandomNumber();
		elem.value = generateRandomNumber();
		if (insert(&hashTable, elem) == 1) ++curKeys;
	}

	rte_eal_cleanup();

	return 0;
}