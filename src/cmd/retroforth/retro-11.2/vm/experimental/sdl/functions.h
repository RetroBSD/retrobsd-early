#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

/* FILE: devices.c */
void draw_character(int x, int y, int character);
int handle_devices(void *unused);
void init_devices();
void cleanup_devices();

/* FILE: initial_image.c */
void initial_image();

/* FILE: loader.c */
int vm_load_image(char *image);
int vm_save_image(char *image);

/* FILE: ngaro.c */
int main(int argc, char **argv);

/* FILE: video.c */
void drawpixel(int x, int color);
int update_display(void *unused);
void clear_display();

/* FILE: vm.c */
void init_vm();
void vm_process();

#endif
