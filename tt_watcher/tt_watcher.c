#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/completion.h>
#include <linux/interrupt.h> 
#include <linux/sched.h> //wait queue

#include <mach/cp_wakeup_ap.h>

#define TT_WATCHER_NAME "tt_watcher"

struct tt_watcher_data {
    struct device       	*dev;	
	struct miscdevice 		misc_dev;
	struct file_operations 	fops;	
	int 					irq_no;	
	int 					tt_sleep_status;
	int 					tt_status_changed;
	wait_queue_head_t		tt_st_waitq;
	struct comip_ap_wakeup_cp_pdata *pin_data;
};

static struct tt_watcher_data *pdata = NULL;
//DECLARE_WAIT_QUEUE_HEAD(tt_st_waitq);//wait queue init


static int tt_watcher_open(struct inode *ind, struct file *fp)
{
	if(!pdata) {
		printk("tt watcher open error !! \n");
		return -ENOMEM;
	}
		
	printk("%s\n", __func__);

	return 0;
}

static ssize_t tt_watcher_read(struct file *fp, char __user *userbuf, size_t count, loff_t *off)
{
	int ret = -EINVAL;
	
	printk("%s\n", __func__);
	ret = copy_to_user(userbuf, &pdata->tt_sleep_status, 1);
    pdata->tt_status_changed = 0;
	
	return ret;
}

static unsigned int tt_watcher_poll(struct file *fp, struct poll_table_struct *wait)
{
	unsigned int ret = 0;
	printk("%s\n", __func__);

    //poll_wait(fp, &pdata->tt_st_waitq, wait);//

    if(pdata->tt_status_changed) {		
	    ret |= POLLIN;
	}    
	
	printk("%s\n", __func__);
	
	return ret;
}

static int tt_watcher_release(struct inode *ind, struct file *fp)
{
	printk("%s\n", __func__);
	return 0;
}

static const struct file_operations tt_watcher_fops = {
	.owner		= THIS_MODULE,
	.open		= tt_watcher_open,
	.read		= tt_watcher_read,
	.poll		= tt_watcher_poll,	
	.release	= tt_watcher_release,
};

static irqreturn_t tt_status_irq(int irq, void *di)
{
	
	int val = gpio_get_value(pdata->pin_data->mdm2ap_sleep);

	printk("%s,b2a_sleep val = %d\n",__func__, val);
	if(pdata->tt_sleep_status != val)
	{
		pdata->tt_sleep_status = val;
		pdata->tt_status_changed = 1;
		wake_up_interruptible(&pdata->tt_st_waitq);   /*wake up thread*/
		
		printk("wake_up_interruptible\n");
	}
	return IRQ_HANDLED;
	
}

static int init_tt_status_irq(void)
{
	int ret = 0;
	int irqno = 0;

	if(pdata->pin_data == NULL) {		
		printk("tt watcher error:pin data not got! \n");
		return -1;
	}
	irqno  = gpio_to_irq(pdata->pin_data->mdm2ap_sleep);

	pdata->irq_no = irqno;
    ret = request_irq(irqno, 
		tt_status_irq,
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT ,
		"tt_status_irq", 
		pdata);

	if(ret){
		printk("tt watcher:request irq error ! \n");
		free_irq(irqno, pdata);
		return -1;
	}
	return 0;
	
}

static int __init tt_watcher_init(void)
{
	int ret;
	struct comip_ap_wakeup_cp_pdata *board_data = NULL;

	pdata = (struct tt_watcher_data *)kzalloc(sizeof(struct tt_watcher_data), GFP_KERNEL);
	if(!pdata) {
		printk("tt watcher mem failed!\n");
		goto m_error;
	}
	
    //pdata->dev = &pdev->dev;
    //platform_set_drvdata(pdev, pdata);	
	board_data = (struct comip_ap_wakeup_cp_pdata *)get_tt_wakeup_data();

	pdata->pin_data = board_data;
	init_waitqueue_head(&pdata->tt_st_waitq);
	if(init_tt_status_irq()) {
		printk("tt watcher init_tt_status_irq failed!!\n");
		goto error;
	}

	pdata->misc_dev.minor	= MISC_DYNAMIC_MINOR;
	pdata->misc_dev.name		= "ttw";
	pdata->misc_dev.fops		= &tt_watcher_fops;
	ret = misc_register(&pdata->misc_dev);
	if (ret)
	{
		printk("tt watcher register misc failed!!\n");
		goto error;
	}
	printk("tt_watcher probe successful!\n");
	return 0;
	
error:
	kfree(pdata);
m_error:	
	return -ENOMEM;	
}

static void __exit tt_watcher_exit(void)
{
	return;
}

/*static struct platform_driver tt_watcher_driver = {
    .remove = __exit_p(tt_watcher_remove),
    .driver = {
        .name  = TT_WATCHER_NAME,
        .owner = THIS_MODULE,        
    },
    
};

static int __init tt_watcher_init(void)
{
	return platform_driver_probe(&tt_watcher_driver, tt_watcher_probe);
}

static void __exit tt_watcher_exit(void)
{
	platform_driver_unregister(&tt_watcher_driver);
}
*/


late_initcall(tt_watcher_init);
module_exit(tt_watcher_exit);



