/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 * Virtual machine abstraction.
 */
 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <assert.h>


#include "device.h"
#include "cpu.h"
#include "vm.h"
#include "dev_vtty.h"
#include "debug.h"

/* Get VM type */
char *vm_get_type(vm_instance_t * vm)
{
   char *machine;

   switch (vm->type)
   {
   case VM_TYPE_PAVO:
      machine = "PAVO";
      break;
   default:
      machine = "unknown";
      break;
   }
   return machine;
}

/* Get platform type */
char *vm_get_platform_type(vm_instance_t * vm)
{
   char *machine;

   switch (vm->type)
   {
   case VM_TYPE_PAVO:
      machine = "PAVO";
      break;
   default:
      machine = "VM";
      break;
   }

   return machine;
}


/* Generate a filename for use by the instance */
static char *vm_build_filename(vm_instance_t * vm, char *name)
{
   char *filename, *machine;

   machine = vm_get_type(vm);

   /*switch(vm_file_naming_type) {
      case 1:
      filename = dyn_sprintf("%s_i%u_%s",machine,vm->instance_id,name);
      break; */
   //    case 0:
   //    default:
   filename = dyn_sprintf("%s_%s_%s", machine, vm->name, name);
   //      break;
   //}

   assert(filename != NULL);
   return filename;
}


/* Log a message */
void vm_flog(vm_instance_t * vm, char *module, char *format, va_list ap)
{
   if (vm->log_fd)
      m_flog(vm->log_fd, module, format, ap);
}

/* Log a message */
void vm_log(vm_instance_t * vm, char *module, char *format, ...)
{
   va_list ap;

   if (vm->log_fd)
   {
      va_start(ap, format);
      vm_flog(vm, module, format, ap);
      va_end(ap);
   }
}

/* Close the log file */
int vm_close_log(vm_instance_t * vm)
{
   if (vm->log_fd)
      fclose(vm->log_fd);

   free(vm->log_file);

   vm->log_file = NULL;
   vm->log_fd = NULL;
   return (0);
}

/* Create the log file */
int vm_create_log(vm_instance_t * vm)
{
   if (vm->log_file_enabled)
   {
      vm_close_log(vm);

      if (!(vm->log_file = vm_build_filename(vm, "log.txt")))
         return (-1);

      if (!(vm->log_fd = fopen(vm->log_file, "w")))
      {
         fprintf(stderr, "VM %s: unable to create log file '%s'\n", vm->name, vm->log_file);
         free(vm->log_file);
         vm->log_file = NULL;
         return (-1);
      }
   }

   return (0);
}

/* Error message */
void vm_error(vm_instance_t * vm, char *format, ...)
{
   char buffer[2048];
   va_list ap;

   va_start(ap, format);
   vsnprintf(buffer, sizeof(buffer), format, ap);
   va_end(ap);

   fprintf(stderr, "%s '%s': %s", vm_get_platform_type(vm), vm->name, buffer);
}

/* Create a new VM instance */
vm_instance_t *vm_create(char *name, int machine_type)
{
   vm_instance_t *vm;

   if (!(vm = malloc(sizeof(*vm))))
   {
      fprintf(stderr, "VM %s: unable to create new instance!\n", name);
      return NULL;
   }

   memset(vm, 0, sizeof(*vm));
   vm->type = machine_type;
   vm->status = VM_STATUS_HALTED;
   vm->vtty_con1_type = VTTY_TYPE_TERM;
   vm->vtty_con2_type = VTTY_TYPE_NONE;
   vm->log_file_enabled = TRUE;

   if (!(vm->name = strdup(name)))
   {
      fprintf(stderr, "VM %s: unable to store instance name!\n", name);
      goto err_name;
   }


   /* create log file */
   if (vm_create_log(vm) == -1)
      goto err_log;

   /*create configure*/
   if (!(vm->configure= malloc(sizeof(*vm->configure))))
		goto err_configure;
   memset(vm->configure,0,sizeof(*vm->configure));

   return vm;
err_configure:
	vm_close_log(vm);
 err_log:
   free(vm->name);
 err_name:
   free(vm);
   return NULL;
}

/* 
 * Shutdown hardware resources used by a VM.
 * The CPU must have been stopped.
 */
static int vm_hardware_shutdown(vm_instance_t * vm)
{
   //int i;

   if ((vm->status == VM_STATUS_HALTED) )
   {
      vm_log(vm, "VM", "trying to shutdown an inactive VM.\n");
      return (-1);
   }

   vm_log(vm, "VM", "shutdown procedure engaged.\n");

   /* Mark the VM as halted */
   vm->status = VM_STATUS_HALTED;

   /* Disable NVRAM operations */
   // vm->nvram_extract_config = NULL;
   //  vm->nvram_push_config = NULL;

   /* Free the object list */
   // vm_object_free_list(vm);

   /* Free resources used by PCI busses */
   //  vm_log(vm,"VM","removing PCI busses.\n");
   //  pci_io_data_remove(vm,vm->pci_io_space);
   //  pci_bus_remove(vm->pci_bus[0]);
   //  pci_bus_remove(vm->pci_bus[1]);
   //  vm->pci_bus[0] = vm->pci_bus[1] = NULL;

   /* Free the PCI bus pool */
   /* for(i=0;i<VM_PCI_POOL_SIZE;i++) {
      if (vm->pci_bus_pool[i] != NULL) {
      pci_bus_remove(vm->pci_bus_pool[i]);
      vm->pci_bus_pool[i] = NULL;
      }
      }     */

   /* Remove the IRQ routing vectors */
   vm->set_irq = NULL;
   vm->clear_irq = NULL;

   /* Delete the VTTY for Console and AUX ports */
   vm_log(vm, "VM", "deleting VTTY.\n");
   vm_delete_vtty(vm);

   /* Delete system CPU group */
   vm_log(vm, "VM", "deleting system CPUs.\n");
   vm->cpu = NULL;

   vm_log(vm, "VM", "shutdown procedure completed.\n");
   return (0);
}

/* Free resources used by a VM */
void vm_free(vm_instance_t * vm)
{
   if (vm != NULL)
   {
      /* Free hardware resources */
      vm_hardware_shutdown(vm);

      /* Close log file */
      vm_close_log(vm);

      /* Remove the lock file */
      // vm_release_lock(vm,TRUE);

      /* Free all chunks */
      // vm_chunk_free_all(vm);

      /* Free various elements */
      // free(vm->ghost_ram_filename);
      //   free(vm->sym_filename);
      //free(vm->ios_image);
      // free(vm->ios_config);
      //free(vm->rom_filename);
      free(vm->name);
      free(vm);
   }
}


int dev_ram_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
/* Initialize RAM */
int vm_ram_init(vm_instance_t * vm, m_pa_t paddr)
{
   m_uint32_t len;

   len = vm->configure->ram_size * 1048576;
#ifdef SIM_PAVO
/*
Why plus 0x2000 (8k) for PAVO??
It seems that jz4740 has an extra 8k boot memory.
I am not sure. But uboot use the extra memory space beyond 64M memory space.
So I just add 0x2000 to ram.
*/
   len += 0x2000;
#endif
   return (dev_ram_init(vm, "ram", paddr, len));
}

/* Initialize VTTY */
int vm_init_vtty(vm_instance_t * vm)
{
   /* Create Console and AUX ports */
   vm->vtty_con1 = vtty_create(vm, "Console port",
                               vm->vtty_con1_type, vm->vtty_con1_tcp_port, &vm->vtty_con1_serial_option);

   vm->vtty_con2 = vtty_create(vm, "AUX port",
                               vm->vtty_con2_type, vm->vtty_con2_tcp_port, &vm->vtty_con2_serial_option);
   return (0);
}

/* Delete VTTY */
void vm_delete_vtty(vm_instance_t * vm)
{
   vtty_delete(vm->vtty_con1);
   vtty_delete(vm->vtty_con2);
   vm->vtty_con1 = vm->vtty_con2 = NULL;
}


/* Bind a device to a virtual machine */
int vm_bind_device(vm_instance_t * vm, struct vdevice *dev)
{
   struct vdevice **cur;
   u_int i;

   /* 
    * Add this device to the device array. The index in the device array
    * is used by the MTS subsystem.
    */
   for (i = 0; i < VM_DEVICE_MAX; i++)
      if (!vm->dev_array[i])
         break;

   if (i == VM_DEVICE_MAX)
   {
      fprintf(stderr, "VM: vm_bind_device: device table full.\n");
      return (-1);
   }

   vm->dev_array[i] = dev;
   dev->id = i;

   /*
    * Add it to the linked-list (devices are ordered by physical addresses).
    */
   for (cur = &vm->dev_list; *cur; cur = &(*cur)->next)
      if ((*cur)->phys_addr > dev->phys_addr)
         break;

   dev->next = *cur;
   if (*cur)
      (*cur)->pprev = &dev->next;
   dev->pprev = cur;
   *cur = dev;
   return (0);
}

/* Unbind a device from a virtual machine */
int vm_unbind_device(vm_instance_t * vm, struct vdevice *dev)
{
   u_int i;

   if (!dev || !dev->pprev)
      return (-1);

   /* Remove the device from the linked list */
   if (dev->next)
      dev->next->pprev = dev->pprev;

   *(dev->pprev) = dev->next;

   /* Remove the device from the device array */
   for (i = 0; i < VM_DEVICE_MAX; i++)
      if (vm->dev_array[i] == dev)
      {
         vm->dev_array[i] = NULL;
         break;
      }

   /* Clear device list info */
   dev->next = NULL;
   dev->pprev = NULL;
   return (0);
}

/* Map a device at the specified physical address */
int vm_map_device(vm_instance_t * vm, struct vdevice *dev, m_pa_t base_addr)
{

   /* Unbind the device if it was already active */
   vm_unbind_device(vm, dev);

   /* Map the device at the new base address and rebuild MTS */
   dev->phys_addr = base_addr;
   vm_bind_device(vm, dev);
   return (0);
}

/* Suspend a VM instance */
int vm_suspend(vm_instance_t * vm)
{
   if (vm->status == VM_STATUS_RUNNING)
   {
      vm->status = VM_STATUS_SUSPENDED;
   }
   return (0);
}

/* Resume a VM instance */
int vm_resume(vm_instance_t * vm)
{
   if (vm->status == VM_STATUS_SUSPENDED)
   {
      vm->status = VM_STATUS_RUNNING;
   }
   return (0);
}

/* Stop an instance */
int vm_stop(vm_instance_t * vm)
{
   vm->status = VM_STATUS_SHUTDOWN;
   return (0);
}


/* Monitor an instance periodically */
void vm_monitor(vm_instance_t * vm)
{
   while (vm->status != VM_STATUS_SHUTDOWN)
      usleep(1000);
}



