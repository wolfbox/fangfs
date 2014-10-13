#include <string.h>
#include <sodium.h>
#include "test.h"
#include "../src/metafile.h"

void test_field(void) {
	do_test();

	metafield_t field;

	char nonce[sizeof(field.nonce)];
	memset(nonce, 0, sizeof(nonce));
	char encrypted_key[sizeof(field.encrypted_key)];
	memset(encrypted_key, 1, sizeof(encrypted_key));

	field.opslimit = 513;
	field.memlimit = 513;
	memcpy(field.nonce, nonce, sizeof(nonce));
	memcpy(field.encrypted_key, encrypted_key, sizeof(encrypted_key));

	uint8_t buf[META_FIELD_LEN];

	// Test serialize
	metafield_serialize(&field, buf);

	const uint8_t rightbuf[META_FIELD_LEN] = {
	    0x01, 0x02, 0x00, 0x00,
	    0x01, 0x02, 0x00, 0x00,
	    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	verify(memcmp(buf, rightbuf, sizeof(buf)) == 0);

	// Test parse
	{
		metafield_t field2;
		metafield_parse(&field2, buf);
		verify(memcmp(&field, &field2, sizeof(field)) == 0);
	}
}

int main(void) {
	test_field();
	return 0;
}
