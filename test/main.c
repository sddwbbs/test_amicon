#include <stdio.h>
#include <stdint.h>

#include <rte_memory.h>
#include <rte_malloc.h>
#include <rte_eal.h>
#include <rte_debug.h>

// const size_t NUM_OF_ELEMENTS = 1000000;
const size_t NUM_OF_ELEMENTS = 11;

typedef struct {
	uint64_t key;
	uint64_t value;
} Pair;

typedef struct {
	uint64_t value;
	int sign;
} BigNumber;

typedef struct {
    BigNumber* table1;
    BigNumber* table2;
} HashTable;

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

size_t hash1(const uint64_t key) {
	return key % NUM_OF_ELEMENTS;
}

size_t hash2(const uint64_t key) {
	return (key / NUM_OF_ELEMENTS) % NUM_OF_ELEMENTS;
}

bool belong(const HashTable* hashTable, const Pair elem) {
	const size_t index1 = hash1(elem.key);
	const size_t index2 = hash2(elem.key);

	return (hashTable->table1[index1].sign >= 0 && hashTable->table1[index1].value == elem.value ||
			hashTable->table2[index2].sign >= 0 && hashTable->table2[index2].value == elem.value);
}

int insert(HashTable* hashTable, const Pair elem) {
	int i = 1, pass = 1;
	Pair elemCopy = elem;
	uint64_t temp;

	if (belong(hashTable, elem)) return 0;

	while (i < 2 * NUM_OF_ELEMENTS) {
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
	printf("cuchoo hash test\n");

	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	uint64_t mem = rte_eal_get_physmem_size();
	printf("mem: %llu\n", mem);

	HashTable hashTable;
	if (!initialize_hash_table(&hashTable)) {
		return -1;
	}

	Pair arr[10] = {{23, 124}
					, {50, 55}
					, {53, 104004}
					, {75, 1414}
					, {100, 9890}
					, {67, 0}
					, {105, 9909}
					, {3, 8184}
					, {36, 89041}
					, {39, 99009}};

	for (int i = 0; i < 10; ++i) {
		insert(&hashTable, arr[i]);
	}

	printTables(&hashTable);

	rte_eal_cleanup();

	return 0;
}