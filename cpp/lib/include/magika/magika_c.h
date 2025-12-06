#ifdef __cplusplus
extern "C" {
#endif

typedef struct magika_t magika_t;

magika_t *magika_new(const char *model_dir);
void magika_free(magika_t *m);
const char *magika_identify_path(magika_t *m, const char *path);

#ifdef __cplusplus
}
#endif