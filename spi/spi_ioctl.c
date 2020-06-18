#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>
