#ifndef _OC_H
#define _OC_H

#include <sys/ioctl.h>

struct oc_state {
    int mode;
    int pwm_duty;
};

#define OC_MAX_DEV 5

#define OC_MODE_PWM     0x0001

#define OC_SET_MODE _IOW('i',1,int)
#define OC_PWM_DUTY _IOW('i',2,int)

#ifdef KERNEL
extern int oc_open (dev_t dev, int flag, int mode);
extern int oc_close (dev_t dev, int flag, int mode);
extern int oc_read (dev_t dev, struct uio *uio, int flag);
extern int oc_write (dev_t dev, struct uio *uio, int flag);
extern int oc_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#define T2CON           PIC32_R (0x00800)
#define PR2             PIC32_R (0x00820)

#define OC1CON          PIC32_R (0x03000)
#define OC2CON          PIC32_R (0x03200)
#define OC3CON          PIC32_R (0x03400)
#define OC4CON          PIC32_R (0x03600)
#define OC5CON          PIC32_R (0x03800)

#define OC1RS           PIC32_R (0x03020)
#define OC2RS           PIC32_R (0x03220)
#define OC3RS           PIC32_R (0x03420)
#define OC4RS           PIC32_R (0x03620)
#define OC5RS           PIC32_R (0x03820)

#endif
