 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

 /*configure template */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define COMMON_CONFIG_INFO_ARRAY   static char *flash_type_string[2]={"NOR FLASH","NAND FLASH"};


#define COMMON_CONFIG_OPTION  \
           CFG_SIMPLE_INT("ram_size", &(vm->configure->ram_size)),   \
			CFG_SIMPLE_INT("gdb_debug", &(vm->gdb_debug)),  \
			CFG_SIMPLE_INT("gdb_port", &(vm->gdb_port)),  \
 			CFG_SIMPLE_INT("flash_size", &(vm->configure->flash_size)),   \
			CFG_SIMPLE_INT("flash_type", &(vm->configure->flash_type)),   \
			CFG_SIMPLE_STR("flash_file_name", &(vm->configure->flash_filename)),   \
			CFG_SIMPLE_INT("flash_phy_address", &(vm->configure->flash_address)),   \

#define  PRINT_COMMON_COFING_OPTION    \
    printf("Using configure file: %s\n",vm->configure_filename);  \
	printf("ram_size: %dM bytes \n",vm->configure->ram_size);  \
	if (vm->configure->flash_size!=0)   \
	{   \
	    printf("flash_type: %s \n",flash_type_string[vm->configure->flash_type-1]);   \
		printf("flash_size: %dM bytes \n",vm->configure->flash_size);   \
		if (vm->configure->flash_type==FLASH_TYPE_NOR_FLASH)   \
		  {  \
		  printf("flash_file_name: %s \n",vm->configure->flash_filename);   \
		  printf("flash_phy_address: 0x%x \n",vm->configure->flash_address);   \
		  } \
	}   \
	if (vm->gdb_debug!=0)   \
	{   \
		printf("GDB debug enable\n");   \
		printf("GDB port: %d \n",vm->gdb_port);    \
	}   \

#define VALID_COMMON_CONFIG_OPTION    \
   ASSERT(vm->configure->ram_size!=0,"ram_size can not be 0\n");   \
   if (vm->configure->flash_type==FLASH_TYPE_NOR_FLASH) \
    {  \
	    if (vm->configure->flash_size!=0)   \
	    {   \
		    ASSERT(vm->configure->flash_size==4,"flash_size should be 4.\n We only support 4MB NOR flash emulation\n");   \
		    /*flash_filename can be null. virtualmips will create it.*/   \
		    ASSERT(vm->configure->flash_address!=0,"flash_address can not be 0\n");   \
	      }   \
    } \
    else  if (vm->configure->flash_type==FLASH_TYPE_NAND_FLASH) \
      { \
          ASSERT(vm->configure->flash_size==0x400,"flash_size should be 0x400.\n We only support 1G byte NAND flash emulation\n");   \
          assert(1); \
      } \
    else  \
        ASSERT(0,"error flash_type. valid value: 1:NOR FLASH 2:NAND FLASH\n");   \

#endif
