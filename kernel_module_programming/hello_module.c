#include <linux/init.h>
#include <linux/module.h> 
#include <linux/kernel.h> 

MODULE_LICENSE("Dual BSD/GPL"); //라이센스

/* 모듈의 초기화 부분 */
static int initModule(void){
    printk(KERN_INFO "Hello Module!\n");
    return 0;
}

/* 모듈 내려질 때 정리 부분*/
static void cleanupModule(void){
    printk(KERN_INFO "Good-bye moduel!\n");
}

module_init(initModule);
module_exit(cleanupModule);
