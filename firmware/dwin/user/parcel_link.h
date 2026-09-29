#ifndef __PARCEL_LINK_H__
#define __PARCEL_LINK_H__

#include "sys.h"

void parcel_link_init(void);
void parcel_link_poll(void);
void parcel_link_1s(void);
void parcel_link_retry(void);

#endif
