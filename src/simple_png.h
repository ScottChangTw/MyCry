#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

#ifdef USE_FPNG
int simple_read_argb_from_fpng(const char *filename, uint8_t **out, uint32_t *w, uint32_t *h, char *title);

int simple_write_argb_to_fpng(const void* pImage, unsigned int width, unsigned int height, const char * format, ...);
#endif

int simple_set_text_libpng(const char * format, ...);
int simple_write_argb_to_libpng(const void* pImage, uint32_t width, uint32_t height, const char * format, ...);
int simple_read_argb_from_libpng(const char *filename, uint8_t **out, uint32_t *w, uint32_t *h, char **title);


#if defined (__cplusplus)
}
#endif


