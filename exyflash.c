// exyflash
// Copyright (c) 2023: Christopher Lentocha
// SPDX-License-Identifier: AGPL-3.0-only


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define DEBUG	1
#define BLOCK_SIZE		512
#define CHUNK_SIZE	(uint32_t)0xfffe00

#if DEBUG
#define dprint(args...) printf(args)
#else
#define dprint(args...)
#endif

static char *target_names[] = {
	"Exynos8890",//TARGET_8890
	"Exynos8895",//TARGET_8895
};

libusb_device_handle *handle = NULL;

#define MAX_PAYLOAD_SIZE	(BLOCK_SIZE - 10)
typedef struct __attribute__ ((__packed__)) dldata_s {
	uint32_t unk0;
	uint32_t size;// header(8) + data(n) + footer(2)
	uint8_t data[];
	//uint16_t footer;
} dldata_t;

static int dnes(dldata_t *payload) {
	int rc;
	int transferred;
	u_int total_size = payload->size;
	uint8_t *payload_ptr = (uint8_t *)payload;

	do {
		rc = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_OUT | 2, payload_ptr, (total_size < BLOCK_SIZE ? total_size : BLOCK_SIZE), &transferred, 0);
		if(rc) {
			fprintf(stderr, "Error libusb_bulk_transfer: %s\n", libusb_error_name(rc));
			return rc;
		}
		dprint("libusb_bulk_transfer: transferred=%d\n", transferred);
		payload_ptr += transferred;
		assert(total_size>=transferred);
		total_size -= transferred;
	} while(total_size > 0);

	return rc;
}

static int identify_target()
{
	struct libusb_device_descriptor desc;
	int rc, i;
	unsigned char product[256];

	if(!handle)
		return -1;

	rc = libusb_get_device_descriptor(libusb_get_device(handle), &desc);
	if (LIBUSB_SUCCESS != rc) {
		fprintf(stderr, "Error getting device descriptor\n");
		return -1;
	}

	printf("Device detected: %04x:%04x\n", desc.idVendor, desc.idProduct);

	if (desc.iProduct) {
		rc = libusb_get_string_descriptor_ascii(handle, desc.iProduct, product, sizeof(product));
		if (rc > 0){
			for(i = 0; i < sizeof(target_names)/sizeof(target_names[0]); i++){
				if(!strcmp(target_names[i], (char *)product)){
					printf("Target: %s\n", target_names[i]);
					return i;
				}
			}
		}
	}

	return -1;
}

static int save_received_data(const char *filename){
	FILE *fd;
	int transferred = 0;
	int total_transferred = 0;
	uint8_t buf[BLOCK_SIZE];

	fd = fopen(filename,"wb");
	if (fd == NULL) {
		fprintf(stderr, "Error: Can't open output file!\n");
		return -1;
	}

	do {
		libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_IN | 1, buf, sizeof(buf), &transferred, 10);// no error handling because device-side is a mess anyway
		fwrite(buf, 1, transferred, fd);
		total_transferred += transferred;
	} while(transferred);

	fclose(fd);

	return total_transferred;
}

int main(int argc, char *argv[])
{
	libusb_context *ctx;
	FILE *fd;
	dldata_t *payload;
	size_t payload_size, fd_size;
	int target_id = -1;
	uint8_t mode;
	int rc;

	if (!(argc == 4)) {
		printf("Usage: %s <USB_BIN> <USB_VID> <USB_DEV>\n", argv[0]);
		printf("\tUSB_BIN: Send BIN file over USB\n");
		printf("\tUSB_VID: USB Vendor ID with 0x prefix \n");
		printf("\tUSB_DEV: USB Device ID with 0x prefix \n");
		return EXIT_SUCCESS;
	}

	fd = fopen(argv[1],"rb");
	if (fd == NULL) {
		fprintf(stderr, "Can't open input file %s !\n", argv[1]);
		return EXIT_FAILURE;
	}

	fseek(fd, 0, SEEK_END);
	fd_size = ftell(fd);

	payload_size = sizeof(dldata_t) + fd_size + 2;// +2 bytes for footer

	payload = (dldata_t*)calloc(1, payload_size);
	payload->size = payload_size;

	fseek(fd, 0, SEEK_SET);
	payload_size = fread(&payload->data, 1, fd_size, fd);
	if (payload_size != fd_size) {
		fprintf(stderr, "Error: cannot read entire file !\n");
		return EXIT_FAILURE;
	}

	rc = libusb_init (&ctx);
	if (rc < 0)
	{
		fprintf(stderr, "Error: failed to initialise libusb: %s\n", libusb_error_name(rc));
		return EXIT_FAILURE;
	}

	#if DEBUG
	libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
	#endif

	handle = libusb_open_device_with_vid_pid(NULL, (strtol(argv[2], NULL, 0)), (strtol(argv[3], NULL, 0)));
	if (!handle) {
		fprintf(stderr, "Error: cannot open device %04x:%04x\n", (strtol(argv[2], NULL, 0)), (strtol(argv[3], NULL, 0)));
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	libusb_set_auto_detach_kernel_driver(handle, 1);
	rc = libusb_claim_interface(handle, 0);
	if(rc) {
		fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
		return EXIT_FAILURE;
	}
	printf("Sending file %s (0x%lx)...\n", argv[1], fd_size);
	rc = dnes(payload);
	if(!rc)
		printf("File %s sent !\n", argv[1]);

	#if DEBUG
	sleep(5);
	#endif
	libusb_release_interface(handle, 0);

	if (handle) {
		libusb_close (handle);
	}

	libusb_exit (NULL);

	return EXIT_SUCCESS;
}