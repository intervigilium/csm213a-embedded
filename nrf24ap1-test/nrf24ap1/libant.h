#ifndef LIBANT_H_
#define LIBANT_H_

#define EVENT_RX_ACKNOWLEDGED           0x9b
#define EVENT_RX_BROADCAST              0x9a
#define EVENT_RX_BURST_PACKET           0x9c
#define EVENT_RX_EXT_ACKNOWLEDGED       0x9e
#define EVENT_RX_EXT_BROADCAST          0x9d
#define EVENT_RX_EXT_BURST_PACKET       0x9f
#define EVENT_RX_FAKE_BURST             0xdd
#define EVENT_RX_FAIL                   0x02
#define EVENT_TX                        0x03
#define EVENT_TRANSFER_TX_COMPLETED     0x05
#define INVALID_MESSAGE                 0x28
#define MESG_ACKNOWLEDGED_DATA_ID       0x4f
#define MESG_ASSIGN_CHANNEL_ID          0x42
#define MESG_BROADCAST_DATA_ID          0x4e
#define MESG_BURST_DATA_ID              0x50
#define MESG_CAPABILITIES_ID            0x54
#define MESG_CHANNEL_ID_ID              0x51
#define MESG_CHANNEL_MESG_PERIOD_ID     0x43
#define MESG_CHANNEL_RADIO_FREQ_ID      0x45
#define MESG_CHANNEL_SEARCH_TIMEOUT_ID  0x44
#define MESG_CHANNEL_STATUS_ID          0x52
#define MESG_CLOSE_CHANNEL_ID           0x4c
#define MESG_EXT_ACKNOWLEDGED_DATA_ID   0x5e
#define MESG_EXT_BROADCAST_DATA_ID      0x5d
#define MESG_EXT_BURST_DATA_ID          0x5f
#define MESG_NETWORK_KEY_ID             0x46
#define MESG_RADIO_TX_POWER_ID          0x47
#define MESG_OPEN_CHANNEL_ID            0x4b
#define MESG_OPEN_RX_SCAN_ID            0x5b
#define MESG_REQUEST_ID                 0x4d
#define MESG_RESPONSE_EVENT_ID          0x40
#define MESG_RESPONSE_EVENT_SIZE        0x03
#define MESG_SEARCH_WAVEFORM_ID         0x49
#define MESG_SYSTEM_RESET_ID            0x4a
#define MESG_TX_SYNC                    0xa4
#define MESG_UNASSIGN_CHANNEL_ID        0x41
#define RESPONSE_NO_ERROR               0x00
#define MESG_RADIO_CW_MODE_ID           0x48
#define MESG_RADIO_CW_INIT_ID           0x53
#define MESG_ID_LIST_ADD_ID             0x59
#define MESG_ID_LIST_CONFIG_ID          0x5a
#define MESG_SET_LP_SEARCH_TIMEOUT_ID   0x63
#define MESG_SERIAL_NUM_SET_CHANNEL_ID_ID 0x65
#define MESG_RX_EXT_MESGS_ENABLE_ID     0x66
#define MESG_ENABLE_LED_FLASH_ID        0x68

#define DEFAULT_NETWORK_NUMBER          0x00
#define DEFAULT_TRANSMISSION_TYPE       0x00
#define DEFAULT_DEVICE_TYPE_ID          0xFD
#define DEFAULT_CHANNEL_FREQ            0x66
#define DEFAULT_CHANNEL_PERIOD          0x1f86
#define RX_DUPLEX_CHANNEL_TYPE          0x00
#define TX_DUPLEX_CHANNEL_TYPE          0x10
#define RX_SHARED_CHANNEL_TYPE          0x20
#define TX_SHARED_CHANNEL_TYPE          0x30
#define RX_NO_CLEAR_WILDCARD_CHANNEL_TYPE 0x40
#define TX_ONLY_CHANNEL_TYPE            0x50

struct ant_packet {
  uint8_t sync;
  uint8_t length;
  uint8_t id;
  uint8_t *data;
  uint8_t checksum;
};

#endif
