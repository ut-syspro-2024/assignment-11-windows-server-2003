#include "virtio.h"
#include "port_io.h"
#include "common.h"
#include "util.h"

#define CONFIG_ADDRESS_ADDR 0xCF8
#define CONFIG_DATA_ADDR 0xCFC

#define VIRTIO_NET_VENDOR_ID 0x1AF4
#define VIRTIO_NET_DEVICE_ID 0x1000


u32 pci_config_read32(u8 bus_num, u8 dev_num, u8 func_num, u8 offset) {
	io_write32(CONFIG_ADDRESS_ADDR, 1ull << 31 | (u32) bus_num << 16 | (u32) (dev_num & 0x1F) << 11 | (u32) (func_num & 0x7) << 8 | offset);
	return io_read32(CONFIG_DATA_ADDR);
}


u32 read_bar0(u8 bus_num, u8 dev_num, u8 func_num, int *is_io_space) {
	u32 t = pci_config_read32(bus_num, dev_num, func_num, 0x10);
	*is_io_space = t & 1;
	return (t & 1) ? (t & ~3) : (t & ~15);
}


static int virtio_net_bus_num;
static int virtio_net_dev_num;
static int virtio_net_func_num;

#define BUS_MAX 255
#define DEV_MAX 31
#define FUNC_MAX 0

void find_virtio_net() {
	for (int bus = 0; bus <= BUS_MAX; bus++) for (int dev_num = 0; dev_num <= DEV_MAX; dev_num++) for (int func = 0; func <= FUNC_MAX; func++) {
		u32 t = pci_config_read32(bus, dev_num, func, 0);
		u16 dev_id = t >> 16;
		u16 vendor_id = t & 0xFFFF;
		if (dev_id == VIRTIO_NET_DEVICE_ID && vendor_id == VIRTIO_NET_VENDOR_ID) {
			puts("found virtio\n");
			virtio_net_bus_num = bus;
			virtio_net_dev_num = dev_num;
			virtio_net_func_num = func;
			
			puts("bus:");
			puth(virtio_net_bus_num, 2);
			puts("\n");
			puts("dev:");
			puth(virtio_net_dev_num, 2);
			puts("\n");
			puts("func:");
			puth(virtio_net_func_num, 2);
			puts("\n");
		}
	}
}

// https://www.redhat.com/en/blog/virtqueues-and-virtio-ring-how-data-travels
typedef struct { 
	u64 addr;
	u32 len;
	u16 flags;
	u16 next;
} __attribute__((packed)) virtq_desc;


#define VIRTQ_AVAIL_USED_EVENT(virtq_avail) (*(u16 *) ((char *) &(virtq_avail) + sizeof(u16) * (2 + QUEUE_SIZE)))
typedef struct {
	u16 flags;
	u16 idx;
	u16 ring[0]; // QUEUE_SIZE
	// u16 used_event;
} __attribute__ ((packed)) virtq_avail;

typedef struct {
	u32 id; /* Index of start of used descriptor chain. */
	u32 len;/* Total length of the descriptor chain which was used (written to) */
}  __attribute__ ((packed)) virtq_used_elem;
#define VIRTQ_USED_USED_EVENT(virtq_avail) (*(u16 *) ((char *) &(virtq_avail) + sizeof(u16) * 2 + sizeof(virtq_used_elem) * QUEUE_SIZE))
typedef struct {
	u16 flags;
	u16 idx;
	virtq_used_elem ring[0]; // QUEUE_SIZE
	// u16 used_event;
} __attribute__ ((packed)) virtq_used;

u16 QUEUE_SIZE;

#define ALIGN(x) (((x) + 0x1000) & ~(0x1000 - 1)) 
#define VIRTQ_DESC(virtq_ptr) ((volatile virtq_desc *) (virtq_ptr))
#define VIRTQ_AVAIL(virtq_ptr) (*(volatile virtq_avail *) ((char *) (virtq_ptr) + QUEUE_SIZE * sizeof(virtq_desc)))
#define VIRTQ_USED(virtq_ptr) (*(volatile virtq_used *) ((char *) (virtq_ptr) + ALIGN(QUEUE_SIZE * sizeof(virtq_desc) + sizeof(u16) * (3 + QUEUE_SIZE))))
#define VIRTQ_SIZE (ALIGN(QUEUE_SIZE * sizeof(virtq_desc) + sizeof(u16) * (3 + QUEUE_SIZE)) + \
					ALIGN(sizeof(u16) * 3 + sizeof(virtq_used_elem) * QUEUE_SIZE))

typedef struct { 
#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1 
#define VIRTIO_NET_HDR_F_DATA_VALID    2 
#define VIRTIO_NET_HDR_F_RSC_INFO      4 
	u8 flags; 
#define VIRTIO_NET_HDR_GSO_NONE        0 
#define VIRTIO_NET_HDR_GSO_TCPV4       1 
#define VIRTIO_NET_HDR_GSO_UDP         3 
#define VIRTIO_NET_HDR_GSO_TCPV6       4 
#define VIRTIO_NET_HDR_GSO_ECN      0x80 
	u8 gso_type; 
	u16 hdr_len; 
	u16 gso_size; 
	u16 csum_start; 
	u16 csum_offset; 
	// u16 num_buffers; 
} __attribute__ ((packed)) virtio_net_hdr;


typedef struct {
	u8 dst_mac[6];
	u8 src_mac[6];
	u16 type;
	u8 data[0];
} __attribute__ ((packed)) ethernet_frame;

#define BUFFER_LEN 0x1000
#define N_RX_BUFFERS 0x10
char tx_buffer[BUFFER_LEN] __attribute__((aligned(0x1000)));
char rx_buffer[N_RX_BUFFERS][BUFFER_LEN] __attribute__((aligned(0x1000)));

#define QUEUE_ID_RX 0
#define QUEUE_ID_TX 1
#define virtq_rx ((void *) 0x70ff0000)
#define virtq_tx ((void *) 0x71ff0000)


#define OFFSET_DEV_FEATS 0
#define OFFSET_DRIVER_FEATS 4
#define OFFSET_QUEUE_ADDR 8
#define OFFSET_QUEUE_SIZE 12
#define OFFSET_QUEUE_SEL 14
#define OFFSET_QUEUE_NOTIFY 16
#define OFFSET_STATUS 18

#define STATUS_ACKNOWLEDGE 1
#define STATUS_DRIVER 2
#define STATUS_FAILED 128
#define STATUS_FEATURES_OK 8
#define STATUS_DRIVER_OK 4
#define STATUS_NEEDS_RESET 64

#define VIRTIO_NET_F_CSUM (1 << 0)
#define VIRTIO_NET_F_MAC (1 << 5)

void init_virtio_net(u32 bar0) {
	io_write8(bar0 + OFFSET_STATUS, io_read8(bar0 + OFFSET_STATUS) | STATUS_ACKNOWLEDGE);
	io_write8(bar0 + OFFSET_STATUS, io_read8(bar0 + OFFSET_STATUS) | STATUS_DRIVER);
	
	u32 features = io_read32(bar0 + OFFSET_DEV_FEATS);
	puts("features:");
	puth(features, 8);
	puts("\n");
	ASSERT(features & VIRTIO_NET_F_MAC);
	// ASSERT(features & VIRTIO_NET_F_CSUM); // not set ???
	
	io_write32(bar0 + OFFSET_DRIVER_FEATS, VIRTIO_NET_F_MAC | VIRTIO_NET_F_CSUM); // write features we want to use
	io_write8(bar0 + OFFSET_STATUS, io_read8(bar0 + OFFSET_STATUS) | STATUS_FEATURES_OK); // set FEATURE_OK
	// check for FEATURE_OK still set
	u8 status = io_read8(bar0 + OFFSET_STATUS);
	puts("status:");
	puth(status, 2);
	puts("\n");
	ASSERT(status & STATUS_FEATURES_OK);
	
	
	// register tx que
	{
		io_write16(bar0 + OFFSET_QUEUE_SEL, QUEUE_ID_TX);
		QUEUE_SIZE = io_read16(bar0 + OFFSET_QUEUE_SIZE);
		puts("queue size:");
		puth(QUEUE_SIZE, 2);
		puts("\n");
		io_write32(bar0 + OFFSET_QUEUE_ADDR, (u64) virtq_tx >> 12);
	}
	
	// register rx que
	{
		io_write16(bar0 + OFFSET_QUEUE_SEL, QUEUE_ID_RX);
		u16 dev_queue_size = io_read16(bar0 + OFFSET_QUEUE_SIZE);
		ASSERT(dev_queue_size == QUEUE_SIZE);
		io_write32(bar0 + OFFSET_QUEUE_ADDR, (u64) virtq_rx >> 12);
	}
	
	for (u32 i = 0; i < VIRTQ_SIZE; i++) ((char *) virtq_rx)[i] = 0;
	for (u32 i = 0; i < VIRTQ_SIZE; i++) ((char *) virtq_tx)[i] = 0;
	
	// register rx descs
	for (int i = 0; i < N_RX_BUFFERS; i++) {
		VIRTQ_AVAIL(virtq_rx).ring[VIRTQ_AVAIL(virtq_rx).idx] = i;
		VIRTQ_AVAIL(virtq_rx).idx++;
	}
	io_write16(bar0 + OFFSET_QUEUE_NOTIFY, QUEUE_ID_RX);
	
	// register
	for (int i = 0; i < N_RX_BUFFERS; i++) {
		virtq_desc desc;
		desc.addr = (u64) (rx_buffer + i);
		desc.len = BUFFER_LEN;
		desc.flags = 2; // ?
		desc.next = 0;
		VIRTQ_DESC(virtq_rx)[i] = desc;
	}
	
	io_write8(bar0 + OFFSET_STATUS, io_read8(bar0 + OFFSET_STATUS) | STATUS_DRIVER_OK); // set DRIVER_ok
	puts("ok!!!!\n");
}

void send_packet(u32 bar0, char *data, int len, u8 src_mac[6], u8 dst_mac[6]) {
	ASSERT(len + sizeof(ethernet_frame) + sizeof(virtio_net_hdr) <= BUFFER_LEN);
	// prepare buffer
	virtio_net_hdr *header = (virtio_net_hdr *) tx_buffer;
	header->flags = 0;
	header->gso_type = VIRTIO_NET_HDR_GSO_NONE;
	header->hdr_len = sizeof(virtio_net_hdr) + sizeof(ethernet_frame) + len;
	header->gso_size = 0;
	header->csum_start = 0;
	header->csum_offset = 0;
	//header->num_buffers = 0;
	ethernet_frame *eth_frame = (ethernet_frame *) (tx_buffer + sizeof(virtio_net_hdr));
	for (int i = 0; i < 6; i++) eth_frame->src_mac[i] = src_mac[i];
	for (int i = 0; i < 6; i++) eth_frame->dst_mac[i] = dst_mac[i];
	eth_frame->type = len;
	for (int i = 0; i < len; i++) eth_frame->data[i] = data[i];
	
	// register to available queue
	virtq_desc desc;
	desc.addr = (u64) tx_buffer;
	desc.len = len + sizeof(ethernet_frame) + sizeof(virtio_net_hdr);
	desc.flags = 0;
	desc.next = 0;
	VIRTQ_DESC(virtq_tx)[0] = desc;
	
	VIRTQ_AVAIL(virtq_tx).ring[VIRTQ_AVAIL(virtq_tx).idx++] = 0;
	io_write16(bar0 + OFFSET_QUEUE_NOTIFY, QUEUE_ID_TX);
}

int receive_head = 0;
void receive_packet(u32 bar0) {
	(void) bar0;
	
	// wait for the used queue to update
	while (VIRTQ_USED(virtq_rx).idx == receive_head);
	int idx = VIRTQ_USED(virtq_rx).idx;
	for (int i = receive_head; i < idx; i++) {
		virtq_used_elem used = VIRTQ_USED(virtq_rx).ring[i % QUEUE_SIZE];
		// ASSERT(used.len == 1);
		virtq_desc desc = VIRTQ_DESC(virtq_rx)[used.id];
		puts("length of ethernet frame: ");
		puth(used.len - sizeof(virtio_net_hdr), 4);
		puts("\n");
		puts("content:\n");
		for (u32 i = 0; i + sizeof(virtio_net_hdr) < used.len && i < 0x100; i++) {
			puth(((u8 *) desc.addr)[i + sizeof(virtio_net_hdr)], 2);
			if (i % 0x10 == 0xF) puts("\n");
			else puts(" ");
		}
		if (used.len > 0x100) puts("...(more)\n");
		puts("\n");
	}
	receive_head = idx;
}

void get_mac_address(u32 bar0, u8 mac[6]) {
	for (int i = 0; i < 6; i++) mac[i] = io_read8(bar0 + 20 + i);
}

void init_virtio() {
	find_virtio_net();
	
	int is_io_space;
	u32 bar0 = read_bar0(virtio_net_bus_num, virtio_net_dev_num, virtio_net_func_num, &is_io_space);
	ASSERT(is_io_space);
	
	init_virtio_net(bar0);
	
	u8 mac[6];
	get_mac_address(bar0, mac);
	
	puts("------------------\n");
	puts("MAC Address:");
	for (int i = 0; i < 6; i++) {
		if (i) puts(":");
		puth(mac[i], 2);
	}
	puts("\n");
	puts("------------------\n");
	
	// send a broadcast packet
	puts("Sending packet... ");
	u8 dst_mac[6];
	for (int i = 0; i < 6; i++) dst_mac[i] = 0xFF;
	char buf[] = "This document describes the specifications of the \"virtio\" family of devices. These devices are found in virtual environments, yet by design they look like physical devices to the guest within the virtual machine - and this document treats them as such.";
	send_packet(bar0, buf, 128, mac, dst_mac);
	puts("ok\n");
	
	// receive a packet
	puts("Receiving packet... \n");
	receive_packet(bar0);
}

