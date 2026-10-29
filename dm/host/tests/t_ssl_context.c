// SSL contexts are local resources even when sockets cannot connect. Exercise the real exported hooks, including lifecycle, ownership and concurrent contexts.
#include "../hle_net.c"
#include <mach-o/getsect.h>

void dm_guest_sleep_ns(int64_t ns) { (void)ns; abort(); }
extern const struct mach_header_64 _mh_execute_header;
static int checks, fails;
#define CHECK(c) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static void *hook(const char *name) {
    unsigned long bytes = 0;
    const hle_force_ent *e = (const hle_force_ent *)getsectiondata(&_mh_execute_header, "__DATA", "swforce", &bytes);
    void *fn = NULL; unsigned count = 0;
    for (size_t i = 0; e && i < bytes / sizeof *e; i++) if (!strcmp(e[i].name, name)) { count++; fn = (void *)e[i].fn; }
    CHECK(count == 1 && fn);
    return fn;
}
static Result (*create)(void *, int), (*destroy)(void *), (*get_id)(void *, uint64_t *);
static void *worker(void *arg) {
    uint64_t *id = arg, object = 0;
    if (create(&object, 1) || get_id(&object, id) || !*id || destroy(&object) || object) return (void *)1;
    return NULL;
}
int main(void) {
    create = hook("_ZN2nn3ssl7Context6CreateENS1_10SslVersionE");
    destroy = hook("_ZN2nn3ssl7Context7DestroyEv");
    get_id = hook("_ZN2nn3ssl7Context12GetContextIdEPm");
    Result (*set)(void *, int, int) = hook("_ZN2nn3ssl7Context9SetOptionENS1_13ContextOptionEi");
    Result (*get)(void *, int *, int) = hook("_ZN2nn3ssl7Context9GetOptionEPiNS1_13ContextOptionE");
    Result (*connect)(void *, void *) = hook("_ZN2nn3ssl10Connection6CreateEPNS0_7ContextE");
    unsigned (*available)(void) = hook("_ZN2nn4nifm18IsNetworkAvailableEv");
    if (fails) return 1;
    struct { uint64_t before, object, after; } a = {0x1122334455667788, 0, 0x8877665544332211};
    uint64_t b = 0, id_a = 0, id_b = 0;
    int option = -1;
    CHECK(create(NULL, 1) != RES_OK);
    CHECK(create(&a.object, 0) != RES_OK);
    CHECK(create(&a.object, 2) != RES_OK);
    CHECK(get_id(&a.object, &id_a) != RES_OK && id_a == 0);
    CHECK(create(&a.object, 1) == RES_OK);
    CHECK(create(&b, 0x38) == RES_OK); // TLS 1.0..1.2, a valid explicit version range
    CHECK(get_id(&a.object, &id_a) == RES_OK && id_a);
    CHECK(get_id(&b, &id_b) == RES_OK && id_b && id_b != id_a);
    CHECK(create(&a.object, 1) != RES_OK);
    CHECK(get(&a.object, &option, 1) == RES_OK && option == 1);
    CHECK(set(&a.object, 1, 0) == RES_OK);
    CHECK(get(&a.object, &option, 1) == RES_OK && option == 0);
    CHECK(get(&b, &option, 1) == RES_OK && option == 1);
    CHECK(set(&a.object, 1, 2) != RES_OK);
    CHECK(set(&a.object, 99, 0) != RES_OK);
    uint64_t copy = a.object;
    CHECK(destroy(&copy) != RES_OK); // copying the opaque word cannot destroy another object's context
    CHECK(connect(&copy, &a.object) != RES_OK && available() == 0);
    CHECK(destroy(&a.object) == RES_OK && a.object == 0);
    CHECK(destroy(&a.object) != RES_OK);
    CHECK(get_id(&a.object, &id_b) != RES_OK && id_b == 0);
    CHECK(create(&a.object, 1) == RES_OK);
    CHECK(get_id(&a.object, &id_b) == RES_OK && id_b != id_a);
    CHECK(destroy(&a.object) == RES_OK && destroy(&b) == RES_OK);
    CHECK(a.before == 0x1122334455667788 && a.after == 0x8877665544332211);
    enum { N = 16 };
    pthread_t threads[N]; uint64_t ids[N];
    for (int i = 0; i < N; i++) CHECK(pthread_create(&threads[i], NULL, worker, &ids[i]) == 0);
    for (int i = 0; i < N; i++) { void *ret; CHECK(pthread_join(threads[i], &ret) == 0 && !ret); }
    for (int i = 0; i < N; i++) for (int j = 0; j < i; j++) CHECK(ids[i] != ids[j]);
    CHECK(g_ssl_contexts == NULL);
    printf("ssl contexts: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
