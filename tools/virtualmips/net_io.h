/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 * Network I/O Layer.
 */

#ifndef __NET_IO_H__
#define __NET_IO_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include "utils.h"

/* Maximum packet size */
#define NETIO_MAX_PKT_SIZE  32768

/* Maximum device length */
#define NETIO_DEV_MAXLEN    64

enum {
    NETIO_TYPE_UNIX = 0,
    NETIO_TYPE_VDE,
    NETIO_TYPE_TAP,
    NETIO_TYPE_UDP,
    NETIO_TYPE_TCP_CLI,
    NETIO_TYPE_TCP_SER,
#ifdef LINUX_ETH
    NETIO_TYPE_LINUX_ETH,
#endif
#ifdef GEN_ETH
    NETIO_TYPE_GEN_ETH,
#endif
    NETIO_TYPE_FIFO,
    NETIO_TYPE_NULL,
    NETIO_TYPE_MAX,
};

enum {
    NETIO_FILTER_ACTION_DROP = 0,
    NETIO_FILTER_ACTION_PASS,
    NETIO_FILTER_ACTION_ALTER,
    NETIO_FILTER_ACTION_DUPLICATE,
};

typedef struct netio_desc netio_desc_t;

/* VDE switch definitions */
enum vde_request_type { VDE_REQ_NEW_CONTROL };

#define VDE_SWITCH_MAGIC   0xfeedface
#define VDE_SWITCH_VERSION 3

struct vde_request_v3 {
    m_uint32_t magic;
    m_uint32_t version;
    enum vde_request_type type;
    struct sockaddr_un sock;
};

/* netio unix descriptor */
typedef struct netio_unix_desc netio_unix_desc_t;
struct netio_unix_desc {
    char *local_filename;
    struct sockaddr_un remote_sock;
    int fd;
};

/* netio vde descriptor */
typedef struct netio_vde_desc netio_vde_desc_t;
struct netio_vde_desc {
    char *local_filename;
    struct sockaddr_un remote_sock;
    int ctrl_fd, data_fd;
};

/* netio tap descriptor */
typedef struct netio_tap_desc netio_tap_desc_t;
struct netio_tap_desc {
    char filename[NETIO_DEV_MAXLEN];
    int fd;
};

/* netio udp/tcp descriptor */
typedef struct netio_inet_desc netio_inet_desc_t;
struct netio_inet_desc {
    int local_port, remote_port;
    char *remote_host;
    int fd;
};

#ifdef LINUX_ETH
/* netio linux raw ethernet descriptor */
typedef struct netio_lnxeth_desc netio_lnxeth_desc_t;
struct netio_lnxeth_desc {
    char dev_name[NETIO_DEV_MAXLEN];
    int dev_id, fd;
};
#endif

#ifdef GEN_ETH
/* netio generic raw ethernet descriptor */
typedef struct netio_geneth_desc netio_geneth_desc_t;
struct netio_geneth_desc {
    char dev_name[NETIO_DEV_MAXLEN];
    pcap_t *pcap_dev;
};
#endif

/* FIFO packet */
typedef struct netio_fifo_pkt netio_fifo_pkt_t;
struct netio_fifo_pkt {
    netio_fifo_pkt_t *next;
    size_t pkt_len;
    char pkt[0];
};

/* Netio FIFO */
typedef struct netio_fifo_desc netio_fifo_desc_t;
struct netio_fifo_desc {
    pthread_cond_t cond;
    pthread_mutex_t lock, endpoint_lock;
    netio_fifo_desc_t *endpoint;
    netio_fifo_pkt_t *head, *last;
    u_int pkt_count;
};

/* Packet filter */
typedef struct netio_pktfilter netio_pktfilter_t;
struct netio_pktfilter {
    char *name;
    int (*setup) (netio_desc_t * nio, void **opt, int argc, char *argv[]);
    void (*free) (netio_desc_t * nio, void **opt);
    int (*pkt_handler) (netio_desc_t * nio, void *pkt, size_t len, void *opt);
    netio_pktfilter_t *next;
};

/* Generic netio descriptor */
struct netio_desc {
    u_int type;
    void *dptr;
    char *name;
    int debug;

    /*can nio recv packet now? */
    //u_int can_recv;

    /* Frame Relay specific information */
    //m_uint8_t fr_lmi_seq;
    //void *fr_conn_list;

    /* Ethernet specific information */
    //u_int vlan_port_type;
    //m_uint16_t vlan_id;
    //void *vlan_input_vector;

    union {
        //netio_unix_desc_t nud;
        //netio_vde_desc_t nvd;
        netio_tap_desc_t ntd;
        //netio_inet_desc_t nid;
#ifdef LINUX_ETH
        //netio_lnxeth_desc_t nled;
#endif
#ifdef GEN_ETH
        //netio_geneth_desc_t nged;
#endif
        //netio_fifo_desc_t nfd;
    } u;

    /* Send and receive prototypes */
          ssize_t (*send) (void *desc, void *pkt, size_t len);
          ssize_t (*recv) (void *desc, void *pkt, size_t len);

    /* Configuration saving */
    //void (*save_cfg)(netio_desc_t *nio,FILE *fd);

    /* Packet filters */
    //netio_pktfilter_t *rx_filter,*tx_filter,*both_filter;
    //void *rx_filter_data,*tx_filter_data,*both_filter_data;

    /* Next pointer (for RX listener) */
    netio_desc_t *rxl_next;

    /* Packet data */
    u_char rx_pkt[NETIO_MAX_PKT_SIZE];
};

/* RX listener */
typedef int (*netio_rx_handler_t) (netio_desc_t * nio, u_char * pkt,
    ssize_t pkt_len, void *arg1, void *arg2);

struct netio_rx_listener {
    netio_desc_t *nio;
    u_int ref_count;
    volatile int running;
    netio_rx_handler_t rx_handler;
    void *arg1, *arg2;
    pthread_t spec_thread;
    struct netio_rx_listener *prev, *next;

};

/* Get NETIO type given a description */
int netio_get_type (char *type);

/* Show the NETIO types */
void netio_show_types (void);

/* Create a new NetIO descriptor */
netio_desc_t *netio_desc_create_unix (char *nio_name, char *local,
    char *remote);

/* Create a new NetIO descriptor with VDE method */
netio_desc_t *netio_desc_create_vde (char *nio_name, char *control,
    char *local);

/* Create a new NetIO descriptor with TAP method */
netio_desc_t *netio_desc_create_tap (char *nio_name, char *tap_name);

/* Create a new NetIO descriptor with TCP_CLI method */
netio_desc_t *netio_desc_create_tcp_cli (char *nio_name, char *addr,
    char *port);

/* Create a new NetIO descriptor with TCP_SER method */
netio_desc_t *netio_desc_create_tcp_ser (char *nio_name, char *port);

/* Create a new NetIO descriptor with UDP method */
netio_desc_t *netio_desc_create_udp (char *nio_name, int local_port,
    char *remote_host, int remote_port);

#ifdef LINUX_ETH
/* Create a new NetIO descriptor with raw Ethernet method */
netio_desc_t *netio_desc_create_lnxeth (char *nio_name, char *dev_name);
#endif

#ifdef GEN_ETH
/* Create a new NetIO descriptor with generic raw Ethernet method */
netio_desc_t *netio_desc_create_geneth (char *nio_name, char *dev_name);
#endif

/* Establish a cross-connect between two FIFO NetIO */
int netio_fifo_crossconnect (netio_desc_t * a, netio_desc_t * b);

/* Create a new NetIO descriptor with FIFO method */
netio_desc_t *netio_desc_create_fifo (char *nio_name);

/* Create a new NetIO descriptor with NULL method */
netio_desc_t *netio_desc_create_null (char *nio_name);

/* Acquire a reference to NIO from registry (increment reference count) */
netio_desc_t *netio_acquire (char *name);

/* Release an NIO (decrement reference count) */
int netio_release (char *name);

/* Delete a NetIO descriptor */
int netio_delete (char *name);

/* Delete all NetIO descriptors */
int netio_delete_all (void);

/* Save the configuration of a NetIO descriptor */
void netio_save_config (netio_desc_t * nio, FILE * fd);

/* Save configurations of all NetIO descriptors */
void netio_save_config_all (FILE * fd);

/* Send a packet through a NetIO descriptor */
ssize_t netio_send (netio_desc_t * nio, void *pkt, size_t len);

/* Receive a packet through a NetIO descriptor */
ssize_t netio_recv (netio_desc_t * nio, void *pkt, size_t max_len);

/* Get a NetIO FD */
int netio_get_fd (netio_desc_t * nio);

/* Enable a RX listener */
int netio_rxl_enable (netio_desc_t * nio);

/* Add an RX listener in the listener list */
int netio_rxl_add (netio_desc_t * nio, netio_rx_handler_t rx_handler,
    void *arg1, void *arg2);

/* Remove a NIO from the listener list */
int netio_rxl_remove (netio_desc_t * nio);

/* Initialize the RXL thread */
int netio_rxl_init (void);

#endif
