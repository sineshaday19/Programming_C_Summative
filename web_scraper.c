// Compile with: gcc -pthread -o web_scraper web_scraper.c

/*
 * web_scraper.c — fetch several URLs in parallel with pthreads + curl.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_CMD  512
#define URL_COUNT 3

typedef struct {
    int         index;
    const char *url;
} ScrapeArgs;

static void *fetch_url(void *arg)
{
    ScrapeArgs *fa = (ScrapeArgs *)arg;

    char cmd[MAX_CMD];
    snprintf(cmd, sizeof cmd,
             "curl -s --max-time 15 \"%s\" -o output_%d.txt",
             fa->url, fa->index);

    printf("thread %d: %s\n", fa->index, fa->url);

    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "thread %d: failed (curl exit %d) %s\n",
                fa->index, ret, fa->url);
    } else {
        printf("thread %d: wrote output_%d.txt\n", fa->index, fa->index);
    }

    return NULL;
}

int main(void)
{
    const char *urls[URL_COUNT] = {
        "http://example.com",
        "http://example.org",
        "http://example.net"
    };

    pthread_t threads[URL_COUNT];
    ScrapeArgs args[URL_COUNT];
    int       started[URL_COUNT];

    if (system("curl --version >nul 2>&1") != 0) {
        fprintf(stderr, "curl not found (install it or add to PATH).\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < URL_COUNT; i++) {
        args[i].index = i + 1;
        args[i].url   = urls[i];

        int rc = pthread_create(&threads[i], NULL, fetch_url, &args[i]);
        started[i] = (rc == 0);
        if (rc != 0)
            fprintf(stderr, "pthread_create failed (%d) for job %d\n", rc, i + 1);
    }

    for (int i = 0; i < URL_COUNT; i++) {
        if (started[i])
            pthread_join(threads[i], NULL);
    }

    printf("finished.\n");
    return EXIT_SUCCESS;
}