#ifndef ETHNETIF_H
#define ETHNETIF_H

#ifdef __cplusplus
extern "C" {
#endif

void device_poll();
err_t device_init(struct netif *netif);
void device_address(char *mac);


#ifdef __cplusplus
};
#endif

#endif
