#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define GPIO_90 18
#define DEV_NAME "servo"
#define SERVO_MAJOR_NUMBER 501
#define SERVO_DEV_NAME    "servo"

static int major;

int servo90_open(struct inode *pinode, struct file *pfile){
  printk("[SERVO90] Open servo90_dev\n");
  if(gpio_request(GPIO_90,"servo90")!=0){
    printk("[SERVO90] Already being used");
    return -1;
  }
  gpio_direction_output(GPIO_90,0);
  return 0;
}

int servo90_close(struct inode *pinode, struct file *pfile){
  printk("[SERVO90] Close servo90_dev\n");
  gpio_free(GPIO_90); // Release the pin
  return 0;
}

void turn90_servo(int mode){
  int i;

  if(mode ==0){  
    for(i=0;i<50;i++){
      gpio_set_value(GPIO_90,1);
      mdelay(1.5);
      gpio_set_value(GPIO_90,0);
      mdelay(18.5);
    }
  }
  else if(mode==1){ 
    for(i=0; i<50; i++){
      gpio_set_value(GPIO_90,1);
      udelay(500);
      gpio_set_value(GPIO_90,0);
      mdelay(19);
      udelay(500);
    }
  }
}

ssize_t servo90_write(struct file *pfile, const char *buffer, size_t length, loff_t *offset){ 
  int get_msg;
   
  if(copy_from_user(&get_msg,buffer,4)<0){
    printk(KERN_ALERT "[SERVO90] Write error\n");
    return -1;
  }
  
  if(get_msg==0){          //Turn 90 degrees
    turn90_servo(0);
    printk("[SERVO90] Turn 0 degrees\n");
  }
  else if(get_msg==1){      //Turn 0 degrees
    turn90_servo(1);
    printk("[SERVO90] Turn 90 degrees\n");
  }
  return 0;
}

struct file_operations fop = {
  .owner = THIS_MODULE,
  .open = servo90_open,
  .write = servo90_write,
  .release = servo90_close,
};

int __init servo90_init(void){
  major = register_chrdev(SERVO_MAJOR_NUMBER,DEV_NAME,&fop);
  printk("[SERVO90] Initialize SERVO90_dev major number = %d\n",major);
  return 0;
}

void __exit servo90_exit(void){
  printk("[SERVO90] Exit SERVO90_dev\n");
  unregister_chrdev(SERVO_MAJOR_NUMBER,DEV_NAME);
}

module_init(servo90_init);
module_exit(servo90_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mingyu");
MODULE_DESCRIPTION("servo motor device driver");